//
// Created by fionaw on 14/11/2025.
//

#include "System/pch.h"
#include "Render/EnvironmentMap.h"
#include "Utils/Helper.h"
#include "Utils/SharedUtils.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/Heap.h"
#include "../../../Assets/Shaders/Utils/CBVs.h"
#include "System/FileHelper.h"
#include "System/TextureLoader.h"
#include "Utils/D3DUtils.h"

void EnvironmentMap::CreateCubemapResource(ID3D12Device* device)
{
    if (!m_cubemap.IsInitialized())
        m_cubemap.Init_Tex2D("Cubemap", device, 1024, 1024, 6, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void EnvironmentMap::Init(D3D* d3d, Heap* heap, const std::wstring& filePath, const float rotation)
{
    CherryPrint("EnvMap Initializing...");

    UploadHeap uploadHeap = {};
    size_t maxRequiredUploadHeapSize = Align(sizeof(CbvPanoToEA), 256);

    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();

    m_rotation = rotation;

    if (!m_resourcesInitialized)
        initResources(d3d->GetDevice());

    // Initialize Panoramic
    if (m_currentPanoFilepath != filePath)
    {
        GPU_SCOPE(cmdList, "Upload Pano Map");

        const std::wstring fullPath = FileHelper::GetAssetTextureFullPath((L"EnvMaps/" + filePath).c_str());

        ScratchImage scratchImage;
        m_pano = TextureLoader::LoadTexture2DHDR(d3d->GetDevice(), fullPath.c_str(), scratchImage, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        maxRequiredUploadHeapSize += Align(m_pano.GetIntermediateSize(), 512);
        uploadHeap.Init(d3d->GetDevice(), maxRequiredUploadHeapSize);

        TextureLoader::UploadTexture(cmdList, &uploadHeap, scratchImage, &m_pano);

        m_currentPanoFilepath = filePath;
    }
    else
        uploadHeap.Init(d3d->GetDevice(), maxRequiredUploadHeapSize);

    {
        m_setPanoToEA.Init(heap);
        m_setPanoToEA.AddCBV(d3d->GetDevice(), sizeof(CbvPanoToEA), &uploadHeap, "CBV Pano To EA");
        m_setPanoToEA.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_pano, m_pano.GetDesc().Format);
        m_setPanoToEA.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_ea, m_ea.GetDesc().Format);

        CbvPanoToEA cbv = {};
        cbv.OutputDimensions = hlsl::uint2(m_ea.GetDesc().Width, m_ea.GetDesc().Height);
        cbv.InputDimensions = hlsl::uint2(m_pano.GetDesc().Width, m_pano.GetDesc().Height);
        cbv.Rotation = rotation / 360.0f;
        m_setPanoToEA.UpdateCBV(0, &cbv);
    }

    {
        GPU_SCOPE(cmdList, "Compute EA Map");

        m_pano.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_ea.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        heap->Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigPanoToEA.Get());
        cmdList->SetPipelineState(m_pipelinePanoToEA.GetPSO());
        m_setPanoToEA.SetDescriptorTables_Compute(cmdList);

        DispatchOverTexture(cmdList, 16, m_ea.GetDesc().Width, m_ea.GetDesc().Height);

        m_ea.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    }

    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();

    CherryPrint("EnvMap Initialized");
}

void EnvironmentMap::InitCubemap(D3D* d3d, Heap* heap)
{
    UploadHeap uploadHeapCBV = {};
    uploadHeapCBV.Init(d3d->GetDevice(), Align(sizeof(CbvPanoToCM), 256));

    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();

    assert(m_pano.IsInitialized());

    if (!m_cubemap.IsInitialized())
        m_cubemap.Init_Tex2D("Cubemap", d3d->GetDevice(), 1024, 1024, 6, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, nullptr, true);

    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Format = m_cubemap.GetDesc().Format;
        uavDesc.Texture2DArray.ArraySize = 6;
        uavDesc.Texture2DArray.MipSlice = 0;
        uavDesc.Texture2DArray.FirstArraySlice = 0;

        m_setPanoToCM.Init(heap);
        m_setPanoToCM.AddCBV(d3d->GetDevice(), sizeof(CbvPanoToCM), &uploadHeapCBV, "CBV Cubemap");
        m_setPanoToCM.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_pano, m_pano.GetDesc().Format);
        m_setPanoToCM.SetUAV(d3d->GetDevice(), 0, &m_cubemap, uavDesc);

        CbvPanoToCM cbv = {};
        cbv.OutputWidth = m_cubemap.GetDesc().Width;
        cbv.InputDimensions = hlsl::uint2(m_pano.GetDesc().Width, m_pano.GetDesc().Height);
        cbv.Rotation = m_rotation / 360.0f;
        m_setPanoToCM.UpdateCBV(0, &cbv);
    }

    {
        GPU_SCOPE(cmdList, "Cubemap Initialization");

        m_pano.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_cubemap.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        heap->Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigPanoToCM.Get());
        cmdList->SetPipelineState(m_pipelinePanoToCM.GetPSO());
        m_setPanoToCM.SetDescriptorTables_Compute(cmdList);
    }

    const uint32_t groupSizeXY = (m_cubemap.GetDesc().Width + 15) / 16;
    cmdList->Dispatch(groupSizeXY, groupSizeXY, 6);

    m_cubemap.UavBarrier(cmdList);

    ComputeMipMapsCubemap(d3d->GetDevice(), cmdList, heap);

    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();
}

void EnvironmentMap::initResources(ID3D12Device* device)
{
    m_ea.Init_Tex2D("Equal-Area Envmap", device, 4096, 4096, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_STATIC_SAMPLER_DESC sampler;
    InitializeSamplerLinearClamp(&sampler);

    m_rootSigPanoToEA.SmartInit(device, 1, 1, 1, false, &sampler, 1);
    V(m_pipelinePanoToEA.InitCompute(device, "Compute/PanoToEaCS.hlsl", m_rootSigPanoToEA.Get()));

    m_rootSigPanoToCM.SmartInit(device, 1, 1, 1, false, &sampler, 1);
    V(m_pipelinePanoToCM.InitCompute(device, "Compute/PanoToCubemapCS.hlsl", m_rootSigPanoToCM.Get()));

    m_resourcesInitialized = true;
}

// TODO: Might have minor issues
void EnvironmentMap::ComputeMipMapsCubemap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Heap* heap)
{
    if (!m_pipelineMipMapCubemap.GetPSO())
    {
        m_rootConstantsMipMapCubemap.Init(0, 0, sizeof(CbvMipMapCubemap));

        D3D12_STATIC_SAMPLER_DESC sampler;
        InitializeSamplerLinearClamp(&sampler, 0);
        m_rootSigMipMapCubemap.SmartInit(device, 0, 1, 1, false, &sampler, 1, &m_rootConstantsMipMapCubemap);

        V(m_pipelineMipMapCubemap.InitCompute(device, "Compute/MipMapsCubemapCS.hlsl", m_rootSigMipMapCubemap.Get()));
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srcSRVDesc = {};
    srcSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srcSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srcSRVDesc.TextureCube.MipLevels = 1;
    srcSRVDesc.Format = m_cubemap.GetDesc().Format;

    D3D12_UNORDERED_ACCESS_VIEW_DESC dstUAVDesc = {};
    dstUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    dstUAVDesc.Texture2DArray.ArraySize = 6;
    dstUAVDesc.Texture2DArray.FirstArraySlice = 0;
    dstUAVDesc.Format = m_cubemap.GetDesc().Format;

    heap->Bind(cmdList);
    cmdList->SetComputeRootSignature(m_rootSigMipMapCubemap.Get());
    cmdList->SetPipelineState(m_pipelineMipMapCubemap.GetPSO());

    int width = static_cast<int>(m_cubemap.GetDesc().Width);
    int height = static_cast<int>(m_cubemap.GetDesc().Height);

    m_cubemap.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

    for (int srcMip = 0; srcMip < m_cubemap.GetDesc().MipLevels - 1; srcMip++)
    {
        int dstMip = srcMip + 1;

        DescriptorSet set;
        set.Init(heap, false, true);

        int dstWidth = std::max<int>(width >> (dstMip), 1);
        int dstHeight = std::max<int>(height >> (dstMip), 1);

        CbvMipMapCubemap cbv;
        cbv.FaceResolution.x = static_cast<uint32_t>(dstWidth);
        cbv.FaceResolution.y = static_cast<uint32_t>(dstHeight);
        cbv.Roughness = static_cast<float>(dstMip) / static_cast<float>(m_cubemap.GetDesc().MipLevels - 1);
        m_rootConstantsMipMapCubemap.Bind_Compute(cmdList, &cbv);

        srcSRVDesc.TextureCube.MostDetailedMip = srcMip;
        dstUAVDesc.Texture2DArray.MipSlice = dstMip;

        set.SetSRV(device, 0, &m_cubemap, srcSRVDesc);
        set.SetUAV(device, 0, &m_cubemap, dstUAVDesc);

        if (srcMip > 0)
        {
            const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_cubemap.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, srcMip);
            cmdList->ResourceBarrier(1, &barrier);
        }

        {
            const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_cubemap.GetResource(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srcMip+1);
            cmdList->ResourceBarrier(1, &barrier);
        }

        set.SetDescriptorTables_Compute(cmdList);

        DispatchOverTexture(cmdList, 8, dstWidth, dstHeight, 1);

        m_cubemap.UavBarrier(cmdList);
    }

    if (m_cubemap.GetDesc().MipLevels > 1)
    {
        const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_cubemap.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, m_cubemap.GetDesc().MipLevels-1);
        cmdList->ResourceBarrier(1, &barrier);
    }
}

// TODO : Should be pre-proc?
XMFLOAT3 EnvironmentMap::GetDirectionOfHighestIntensity(D3D* d3d, Heap* heap)
{
    return XMFLOAT3(0.0f, 0.0f, 0.0f);
    /*
    // 9x9 * 16x16 = 144x144
    constexpr float c_blockSize = 144.0f;

    const float fWidth = static_cast<float>(m_ea.GetDesc().Width);
    const float fHeight = static_cast<float>(m_ea.GetDesc().Height);
    const float maxDim = std::max(fWidth, fHeight);
    const size_t numThreadGroups1D = std::ceil(maxDim / c_blockSize);

    const size_t bufferNumElements = numThreadGroups1D * numThreadGroups1D;
    const size_t bufferSize = bufferNumElements * sizeof(MaxLumRedSearchStruct);

    // Initialize Resources
    if (!m_shaderMaxLumRedSearch.GetPSO())
    {
        D3D12_STATIC_SAMPLER_DESC samplers[1];
        samplers[0] = {};
        samplers[0].Filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        samplers[0].ShaderRegister = 0;

        m_rootSigMaxLumRedSearch.SmartInit(d3d->GetDevice(), 1, 1, 1, false, samplers, _countof(samplers));

        m_shaderMaxLumRedSearch.InitCs(L"Compute/MaxLumReductionSearchCS.hlsl", d3d->GetDevice(), m_rootSigMaxLumRedSearch.Get());

        m_bufferMaxLumRedSearch.InitBuffer(L"MaxLumRedSearch StructuredBuffer", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_readbackBufferMaxLumRedSearch.InitBuffer(L"MaxLumRedSearch ReadbackBuffer", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_NONE, true);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = bufferNumElements;
        uavDesc.Buffer.StructureByteStride = sizeof(MaxLumRedSearchStruct);
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN; // structured buffer
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        m_matMaxLumRedSearch.Init(heap);
        m_matMaxLumRedSearch.AddCBV(d3d->GetDevice(), heap, sizeof(CbvMaxLumRedSearch), "CBV Max Luminance Reduction Search");
        m_matMaxLumRedSearch.AddUAV(d3d->GetDevice(), heap, m_bufferMaxLumRedSearch.GetResource(), uavDesc);
        m_matMaxLumRedSearch.SetTex(d3d->GetDevice(), 0, heap, m_ea.GetD12Resource());
    }

    d3d->Flush();

    auto cmdList = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    // Compute Dispatches
    {
        GPU_SCOPE(cmdList.Get(), "Get Direction of Highest Intensity from EA Map");

        m_bufferMaxLumRedSearch.Transition(cmdList.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        CbvMaxLumRedSearch cbv;
        cbv.TexelSize = XMFLOAT2(1.0f / fWidth, 1.0f / fHeight);
        m_matMaxLumRedSearch.UpdateCBV(0, &cbv);

        heap->SetHeap(cmdList.Get());
        cmdList->SetComputeRootSignature(m_rootSigMaxLumRedSearch.Get());
        cmdList->SetPipelineState(m_shaderMaxLumRedSearch.GetPSO());

        m_matMaxLumRedSearch.TransitionSrvsToPS(cmdList.Get());
        m_matMaxLumRedSearch.SetDescriptorTables(cmdList.Get(), true);

        cmdList->Dispatch(numThreadGroups1D, numThreadGroups1D, 1);
    }

    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList.Get());
    d3d->Flush();

    cmdList = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    // Copy StructuredBuffer -> ReadbackBuffer
    {
        m_bufferMaxLumRedSearch.Transition(cmdList.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_readbackBufferMaxLumRedSearch.Transition(cmdList.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->CopyBufferRegion(m_readbackBufferMaxLumRedSearch.GetResource(), 0, m_bufferMaxLumRedSearch.GetResource(), 0, bufferSize);
    }

    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList.Get());
    d3d->Flush();

    std::vector<MaxLumRedSearchStruct> readbackData;
    readbackData.resize(bufferNumElements);

    // Readback data
    {
        void* mappedData = nullptr;
        const D3D12_RANGE readRange = {0, bufferSize};
        V(m_readbackBufferMaxLumRedSearch.GetResource()->Map(0, &readRange, &mappedData));

        memcpy(readbackData.data(), mappedData, bufferSize);

        constexpr D3D12_RANGE writeRange = {0, 0};
        m_readbackBufferMaxLumRedSearch.GetResource()->Unmap(0, &writeRange);
    }

    float maxLum = 0.0f;
    int maxIdx = 0;

    // Compute max luminance of remaining data
    for (int i = 0; i < bufferNumElements; i++)
    {
        if (readbackData[i].Luminance > maxLum)
        {
            maxLum = readbackData[i].Luminance;
            maxIdx = i;
        }
    }

    // Transform uv to direction
    CherryPrint("MaxLumRedSearch Luminance=" << readbackData[maxIdx].Luminance << ", UV=(" << readbackData[maxIdx].UV.x << "," << readbackData[maxIdx].UV.y << ")");
    XMFLOAT3 dir = glueNormalize(EaSquareToSphere(readbackData[maxIdx].UV));
    dir.x = -dir.x;
    dir.y = -dir.y;
    dir.z = -dir.z;

    if (m_rotation == 0)
        return dir;

    // Rotate direction
    float yawRad = XMConvertToRadians(m_rotation);
    XMMATRIX rotY = XMMatrixRotationY(yawRad);
    XMVECTOR d = XMLoadFloat3(&dir);
    d = XMVector3TransformNormal(d, rotY);
    XMFLOAT3 rotatedDir;
    XMStoreFloat3(&rotatedDir, d);

    return rotatedDir;
    */
}