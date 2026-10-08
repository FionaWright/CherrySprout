//
// Created by fionaw on 31/05/2026.
//

#include "System/pch.h"
#include "Scene/SceneManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "HWI/Heap.h"
#include "Scene/InstanceData.h"
#include "System/FileHelper.h"
#include "System/TextureLoader.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

typedef void (*LoadUSDFunc)(const char* usdPath, float sceneScale, SceneCPU* scene);

void SceneManager::AssignScene(const Scene& scene)
{
    m_scene = scene;

    m_gpuDataDirty = true;
}

void SceneManager::LoadScene(const char* filepath, const float sceneScale)
{
    {
        m_scene.GPU.MegaBufferVertex.Reset();
        m_scene.GPU.MegaBufferIndex.Reset();
        m_scene.GPU.MegaBufferMaterials.Reset();
        m_scene.GPU.MegaBufferPunctualLights.Reset();
        m_scene.GPU.MegaBufferInstanceData.Reset();
    }

    m_scene = {};
    m_scene.Filepath = filepath;

    const std::string fullpath = FileHelper::GetAssetFullPath(filepath);
    const bool isUSD = std::filesystem::path(fullpath).extension().string().starts_with(".usd");
    if (!isUSD)
        throw std::runtime_error("Non-USD scenes not supported yet!");

    char exe[MAX_PATH];
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    const std::filesystem::path dllPath = std::filesystem::path(exe).parent_path() / "SceneLoaderUSD.dll";
    const HMODULE dll = LoadLibraryA(dllPath.string().c_str());
    if (!dll)
    {
        LPSTR msg = nullptr;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,
            GetLastError(),
            0,
            (LPSTR)&msg,
            0,
            nullptr);
        std::cerr << "DLL LOAD ERROR: " << msg << std::endl;

        throw std::runtime_error("Failed to load SceneLoaderUSD.dll!");
    }

    const auto LoadUSD =
        reinterpret_cast<LoadUSDFunc>(
            GetProcAddress(dll, "LoadUSD"));

    if (!LoadUSD)
    {
        FreeLibrary(dll);
        throw std::runtime_error("Failed to find SceneLoaderUSD.dll/LoadUSD!");
    }

    CherryPrint("Loading scene...: " << fullpath);
    LoadUSD(fullpath.c_str(), sceneScale, &m_scene.CPU);
    CherryPrint("Loaded scene: " << fullpath);

    CherryPrint("Freeing SceneLoaderUSD DLL...");
    FreeLibrary(dll);
    CherryPrint("Freed SceneLoaderUSD DLL");

    CherryAssert(m_scene.CPU.MegaBufferPunctualLightsCount > 0);
    CherryAssert(m_scene.CPU.MegaBufferMaterialsCount > 0);

    m_gpuDataDirty = true;
}

void SceneManager::UploadScene(D3D* d3d, const bool tryOverloadDDS)
{
    CherryPrint("Uploading Scene...");

    CherryAssert(m_gpuDataDirty);

    const size_t megaBufferVertexBytes = m_scene.CPU.MegaBufferVertexCount * sizeof(Vertex);
    const size_t megaBufferIndexBytes = m_scene.CPU.MegaBufferIndexCount * sizeof(uint32_t);
    const size_t megaBufferMaterialsBytes = m_scene.CPU.MegaBufferMaterialsCount * sizeof(Material);
    const size_t megaBufferPunctualBytes = m_scene.CPU.MegaBufferPunctualLightsCount * sizeof(PunctualLight);
    const size_t megaBufferInstanceDataBytes = m_scene.CPU.ObjectCount * sizeof(InstanceData);

    m_scene.GPU.MegaBufferVertex.Init_Buffer("Mega Buffer Vertex", d3d->GetDevice(), megaBufferVertexBytes);
    m_scene.GPU.MegaBufferIndex.Init_Buffer("Mega Buffer Index", d3d->GetDevice(), megaBufferIndexBytes);
    m_scene.GPU.MegaBufferMaterials.Init_Buffer("Mega Buffer Materials", d3d->GetDevice(), megaBufferMaterialsBytes);
    m_scene.GPU.MegaBufferPunctualLights.Init_Buffer("Mega Buffer Punctual Lights", d3d->GetDevice(), megaBufferPunctualBytes);
    m_scene.GPU.MegaBufferInstanceData.Init_Buffer("Mega Buffer Instance Data", d3d->GetDevice(), megaBufferInstanceDataBytes);

    size_t uploadHeapRequiredSize = 0;
    uploadHeapRequiredSize += Align(m_scene.GPU.MegaBufferVertex.GetIntermediateSize(), 512);
    uploadHeapRequiredSize += Align(m_scene.GPU.MegaBufferIndex.GetIntermediateSize(), 512);
    uploadHeapRequiredSize += Align(m_scene.GPU.MegaBufferMaterials.GetIntermediateSize(), 512);
    uploadHeapRequiredSize += Align(m_scene.GPU.MegaBufferPunctualLights.GetIntermediateSize(), 512);

    const size_t textureCount = m_scene.CPU.TextureFilepathCount;
    std::vector<ScratchImage> scratchImages(textureCount);

    for (int i = 0; i < textureCount; i++)
    {
        const char* path = m_scene.CPU.TextureFilepaths[i];
        const D3D12_RESOURCE_FLAGS flags = tryOverloadDDS ? D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        D12Resource tex = TextureLoader::LoadTexture2DLDR(d3d->GetDevice(), path, scratchImages[i], flags, tryOverloadDDS);

        uploadHeapRequiredSize += Align(tex.GetIntermediateSize(), 512);

        m_scene.GPU.SceneTextures.emplace_back(std::move(tex));
    }

    UploadHeap uploadHeap;
    uploadHeap.Init(d3d->GetDevice(), uploadHeapRequiredSize);

    {
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_COPY);
        const auto cmdList = cmdListPtr.Get();
        {
            GPU_SCOPE(cmdList, "Upload Scene Data");

            m_scene.GPU.MegaBufferIndex.            UploadBuffer(cmdList, &uploadHeap, m_scene.CPU.MegaBufferIndex, megaBufferIndexBytes);
            m_scene.GPU.MegaBufferMaterials.        UploadBuffer(cmdList, &uploadHeap, m_scene.CPU.MegaBufferMaterials, megaBufferMaterialsBytes);
            m_scene.GPU.MegaBufferVertex.           UploadBuffer(cmdList, &uploadHeap, m_scene.CPU.MegaBufferVertex, megaBufferVertexBytes);
            m_scene.GPU.MegaBufferPunctualLights.   UploadBuffer(cmdList, &uploadHeap, m_scene.CPU.MegaBufferPunctualLights, megaBufferPunctualBytes);

            for (int i = 0; i < textureCount; i++)
            {
                TextureLoader::UploadTexture(cmdList, &uploadHeap, scratchImages[i], &m_scene.GPU.SceneTextures[i]);
            }
        }
        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }

    std::vector<int> normalMapTexIndices;
    for (uint32_t i = 0; i < m_scene.CPU.MegaBufferMaterialsCount; i++)
    {
        const int texIdx = m_scene.CPU.MegaBufferMaterials[i].TexIdxNormal;
        if (texIdx >= 0 && std::ranges::find(normalMapTexIndices, texIdx) == normalMapTexIndices.end())
            normalMapTexIndices.emplace_back(texIdx);
    }

    if (normalMapTexIndices.size() > 0)
    {
        Heap heap;
        heap.Init("Scene Manager Heap", d3d->GetDevice(), normalMapTexIndices.size() * 3, 0, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        m_converter.Init(d3d);

        // Impossible to know if normal maps are stored as 2-channel or 3-channel, so convert all
        // Textures can be BC format so need to blit to new texture
        std::vector<std::pair<uint32_t, D12Resource>> textureUpdates;
        {
            const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
            const auto cmdList = cmdListPtr.Get();
            {
                GPU_SCOPE(cmdList, "Convert Textures");

                for (int texIdx : normalMapTexIndices)
                {
                    D12Resource* normalMap2Channel = &m_scene.GPU.SceneTextures[texIdx];

                    D12Resource normalMap3Channel;
                    m_converter.Convert(d3d, &heap, cmdList, normalMap2Channel, eNormalChannels2To3, &normalMap3Channel);

                    textureUpdates.emplace_back(texIdx, std::move(normalMap3Channel));
                }
            }
            V(cmdList->Close());
            d3d->ExecuteCommandList(cmdList);
            d3d->Flush();
        }

        for (auto& pair : textureUpdates)
        {
            m_scene.GPU.SceneTextures[pair.first] = std::move(pair.second);
        }
    }

    m_gpuDataDirty = false;

    CherryPrint("Scene Uploaded");
}

void SceneManager::GenerateMipMaps(D3D* d3d, Heap* heap)
{
    if (!m_pipelineMipMaps.GetPSO())
    {
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        InitializeSamplerLinearClamp(&sampler);

        m_rootSigMipMaps.SmartInit(d3d->GetDevice(), 0, 1, 1, false, &sampler, 1);

        m_pipelineMipMaps.InitCompute(d3d->GetDevice(), "Compute/MipMapsCS.hlsl", m_rootSigMipMaps.Get());
    }

    m_setMipMaps.Init(heap);

    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();

    for (size_t i = 0; i < m_scene.GPU.SceneTextures.size(); i++)
    {
        D12Resource* resource = &m_scene.GPU.SceneTextures[i];
        const auto desc = resource->GetDesc();

        if (desc.MipLevels <= 1 || desc.DepthOrArraySize != 1)
            continue;

        D3D12_SHADER_RESOURCE_VIEW_DESC srcSRVDesc = {};
        srcSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srcSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srcSRVDesc.Format = desc.Format;

        D3D12_UNORDERED_ACCESS_VIEW_DESC dstUAVDesc = {};
        dstUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        dstUAVDesc.Format = desc.Format;

        heap->Bind(cmdList);

        cmdList->SetComputeRootSignature(m_rootSigMipMaps.Get());
        cmdList->SetPipelineState(m_pipelineMipMaps.GetPSO());

        int width = static_cast<int>(desc.Width);
        int height = static_cast<int>(desc.Height);

        resource->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        for (int mip = 0; mip < desc.MipLevels - 1; mip++)
        {
            int dstWidth = std::max<int>(width >> (mip + 1), 1);
            int dstHeight = std::max<int>(height >> (mip + 1), 1);

            srcSRVDesc.Texture2D.MipLevels = 1;
            srcSRVDesc.Texture2D.MostDetailedMip = mip;

            dstUAVDesc.Texture2D.MipSlice = mip + 1;

            DescriptorSet set;
            set.Init(heap);

            set.SetSRV(d3d->GetDevice(), 0, resource, srcSRVDesc);
            set.SetUAV(d3d->GetDevice(), 0, resource, dstUAVDesc);

            if (mip > 0)
            {
                const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                    resource->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, mip);
                cmdList->ResourceBarrier(1, &barrier);
            }

            {
                const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                    resource->GetResource(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, mip+1);
                cmdList->ResourceBarrier(1, &barrier);
            }

            set.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 16, dstWidth, dstHeight);

            resource->UavBarrier(cmdList);
        }

        if (desc.MipLevels > 1)
        {
            const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                resource->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, desc.MipLevels-1);
            cmdList->ResourceBarrier(1, &barrier);
        }
    }

    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();
}

void SceneManager::AddSceneTexturesToHeap(const D3D* d3d, Heap* heap) const
{
    heap->FreeSceneTextures();

    const size_t textureCount = m_scene.GPU.SceneTextures.size();
    for (int i = 0; i < textureCount; i++)
    {
        const D12Resource& tex = m_scene.GPU.SceneTextures[i];
        heap->AddSRV_SceneTexture(d3d->GetDevice(), &tex);
    }
}
