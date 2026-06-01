//
// Created by fionaw on 31/05/2026.
//

#include "System/pch.h"
#include "Scene/SceneManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "Utils/Helper.h"

typedef void (*LoadUSDFunc)(const char* usdPath, SceneCPU* scene);

void SceneManager::LoadScene(const char* filepath)
{
    const bool isUSD = std::filesystem::path(filepath).extension().string().starts_with(".usd");
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

    LoadUSD(filepath, &m_scene.CPU);

    FreeLibrary(dll);

    m_gpuDataDirty = true;
}

void SceneManager::UploadScene(const D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    GPU_SCOPE(cmdList, "Upload Scene");

    assert(m_gpuDataDirty);

    const size_t megaBufferVertexBytes = m_scene.CPU.MegaBufferVertex.size() * sizeof(Vertex);
    const size_t megaBufferIndexBytes = m_scene.CPU.MegaBufferIndex.size() * sizeof(uint32_t);
    const size_t megaBufferMaterialsBytes = m_scene.CPU.MegaBufferMaterials.size() * sizeof(Material);

    m_scene.GPU.MegaBufferVertex.InitBuffer("Mega Buffer Vertex", d3d->GetDevice(), megaBufferVertexBytes);
    m_scene.GPU.MegaBufferIndex.InitBuffer("Mega Buffer Index", d3d->GetDevice(), megaBufferIndexBytes);
    m_scene.GPU.MegaBufferMaterials.InitBuffer("Mega Buffer Materials", d3d->GetDevice(), megaBufferMaterialsBytes);

    size_t uploadHeapRequiredSize = 0;
    uploadHeapRequiredSize += m_scene.GPU.MegaBufferVertex.GetIntermediateSize();
    uploadHeapRequiredSize += m_scene.GPU.MegaBufferIndex.GetIntermediateSize();
    uploadHeapRequiredSize += m_scene.GPU.MegaBufferMaterials.GetIntermediateSize();
    uploadHeapRequiredSize += 512 * 3; // For safety
    m_uploadHeap.Init(d3d->GetDevice(), uploadHeapRequiredSize);

    m_scene.GPU.MegaBufferIndex.UploadBuffer(cmdList, &m_uploadHeap, m_scene.CPU.MegaBufferIndex.data(), megaBufferIndexBytes);
    m_scene.GPU.MegaBufferMaterials.UploadBuffer(cmdList, &m_uploadHeap, m_scene.CPU.MegaBufferMaterials.data(), megaBufferMaterialsBytes);
    m_scene.GPU.MegaBufferVertex.UploadBuffer(cmdList, &m_uploadHeap, m_scene.CPU.MegaBufferVertex.data(), megaBufferVertexBytes);

    m_gpuDataDirty = false;
}
