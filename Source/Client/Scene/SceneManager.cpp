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
    {
        m_scene.GPU.MegaBufferVertex.Reset();
        m_scene.GPU.MegaBufferIndex.Reset();
        m_scene.GPU.MegaBufferMaterials.Reset();
        m_scene.GPU.MegaBufferPunctualLights.Reset();
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

void SceneManager::UploadScene(D3D* d3d)
{
    CherryPrint("Uploading Scene...");

    CherryAssert(m_gpuDataDirty);

    const size_t megaBufferVertexBytes = m_scene.CPU.MegaBufferVertexCount * sizeof(Vertex);
    const size_t megaBufferIndexBytes = m_scene.CPU.MegaBufferIndexCount * sizeof(uint32_t);
    const size_t megaBufferMaterialsBytes = m_scene.CPU.MegaBufferMaterialsCount * sizeof(Material);
    const size_t megaBufferPunctualBytes = m_scene.CPU.MegaBufferPunctualLightsCount * sizeof(PunctualLight);

    m_scene.GPU.MegaBufferVertex.Init_Buffer("Mega Buffer Vertex", d3d->GetDevice(), megaBufferVertexBytes);
    m_scene.GPU.MegaBufferIndex.Init_Buffer("Mega Buffer Index", d3d->GetDevice(), megaBufferIndexBytes);
    m_scene.GPU.MegaBufferMaterials.Init_Buffer("Mega Buffer Materials", d3d->GetDevice(), megaBufferMaterialsBytes);
    m_scene.GPU.MegaBufferPunctualLights.Init_Buffer("Mega Buffer Punctual Lights", d3d->GetDevice(), megaBufferPunctualBytes);

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
        D12Resource tex = TextureLoader::LoadTexture2DLDR(d3d->GetDevice(), path, scratchImages[i]);

        uploadHeapRequiredSize += Align(tex.GetIntermediateSize(), 512);

        m_scene.GPU.SceneTextures.emplace_back(std::move(tex));
    }

    UploadHeap uploadHeap;
    uploadHeap.Init(d3d->GetDevice(), uploadHeapRequiredSize);

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

    m_gpuDataDirty = false;

    CherryPrint("Scene Uploaded");
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
