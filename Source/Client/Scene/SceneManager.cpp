//
// Created by fionaw on 31/05/2026.
//

#include "System/pch.h"
#include "Scene/SceneManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "HWI/Heap.h"
#include "System/FileHelper.h"
#include "System/TextureLoader.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

typedef void (*LoadUSDFunc)(const char* usdPath, float sceneScale, SceneCPU* scene);

void SceneManager::LoadScene(const char* filepath, const float sceneScale)
{
    m_scene = {};
    m_scene.Filepath = filepath;

    const std::string fullpath = FileHelper::GetAssetFullPath(filepath);
    const bool isUSD = std::filesystem::path(fullpath).extension().string().starts_with(".usd");
    if (!isUSD)
        throw std::runtime_error("Non-USD scenes not supported yet!");

    const HMODULE dll = LoadLibraryA("SceneLoaderUSD.dll");
    if (!dll)
    {
        std::cerr << "Failed to load DLL\n";
        return;
    }

    const LoadUSDFunc LoadUSD =
        reinterpret_cast<LoadUSDFunc>(
            GetProcAddress(dll, "LoadUSD"));

    if (!LoadUSD)
    {
        std::cerr << "Failed to find LoadUSD\n";
        FreeLibrary(dll);
        return;
    }

    LoadUSD(fullpath.c_str(), sceneScale, &m_scene.CPU);

    FreeLibrary(dll);

    m_gpuDataDirty = true;
}

void SceneManager::UploadScene(D3D* d3d)
{
    CherryAssert(m_gpuDataDirty);

    const size_t megaBufferVertexBytes = m_scene.CPU.MegaBufferVertexCount * sizeof(Vertex);
    const size_t megaBufferIndexBytes = m_scene.CPU.MegaBufferIndexCount * sizeof(uint32_t);
    const size_t megaBufferMaterialsBytes = m_scene.CPU.MegaBufferMaterialsCount * sizeof(Material);

    m_scene.GPU.MegaBufferVertex.Init_Buffer("Mega Buffer Vertex", d3d->GetDevice(), megaBufferVertexBytes);
    m_scene.GPU.MegaBufferIndex.Init_Buffer("Mega Buffer Index", d3d->GetDevice(), megaBufferIndexBytes);
    m_scene.GPU.MegaBufferMaterials.Init_Buffer("Mega Buffer Materials", d3d->GetDevice(), megaBufferMaterialsBytes);

    size_t uploadHeapRequiredSize = 0;
    uploadHeapRequiredSize += Align(m_scene.GPU.MegaBufferVertex.GetIntermediateSize(), 512);
    uploadHeapRequiredSize += Align(m_scene.GPU.MegaBufferIndex.GetIntermediateSize(), 512);
    uploadHeapRequiredSize += Align(m_scene.GPU.MegaBufferMaterials.GetIntermediateSize(), 512);

    const size_t textureCount = m_scene.CPU.TextureFilepathCount;
    std::vector<ScratchImage> scratchImages(textureCount);

    for (int i = 0; i < textureCount; i++)
    {
        const char* path = m_scene.CPU.TextureFilepaths[i];
        D12Resource tex = TextureLoader::LoadTexture2DLDR(d3d->GetDevice(), path, scratchImages[i]);

        uploadHeapRequiredSize += Align(tex.GetIntermediateSize(), 512);

        m_scene.GPU.SceneTextures.emplace_back(std::move(tex));
    }

    m_uploadHeap = {};
    m_uploadHeap.Init(d3d->GetDevice(), uploadHeapRequiredSize);

    // Upload Data
    {
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_COPY);
        const auto cmdList = cmdListPtr.Get();

        m_scene.GPU.MegaBufferIndex.UploadBuffer(cmdList, &m_uploadHeap, m_scene.CPU.MegaBufferIndex, megaBufferIndexBytes);
        m_scene.GPU.MegaBufferMaterials.UploadBuffer(cmdList, &m_uploadHeap, m_scene.CPU.MegaBufferMaterials, megaBufferMaterialsBytes);
        m_scene.GPU.MegaBufferVertex.UploadBuffer(cmdList, &m_uploadHeap, m_scene.CPU.MegaBufferVertex, megaBufferVertexBytes);

        for (int i = 0; i < textureCount; i++)
        {
            TextureLoader::UploadTexture(cmdList, &m_uploadHeap, scratchImages[i], &m_scene.GPU.SceneTextures[i]);
        }

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }

    m_uploadHeap = {};

    m_gpuDataDirty = false;
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
