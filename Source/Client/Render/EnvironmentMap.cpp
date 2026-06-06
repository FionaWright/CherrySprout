//
// Created by fionaw on 14/11/2025.
//

#include "System/pch.h"
#include "Render/EnvironmentMap.h"
#include "Utils/Helper.h"
#include "Utils/MathUtils.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/Heap.h"
#include "../../../Assets/Shaders/Utils/CBVs.h"
#include "System/FileHelper.h"
#include "Utils/D3DUtils.h"

void EnvironmentMap::CreateCubemapResource(ID3D12Device* device)
{
    if (!m_cubemap.IsInitialized())
        m_cubemap.Init_Tex2D("Cubemap", device, 1024, 1024, 6, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void EnvironmentMap::Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Heap* heap, UploadHeap* uploadHeapCBV, const std::string& filePath, const float rotation)
{
    GPU_SCOPE(cmdList, "Init Pano/EA Map");

    m_rotation = rotation;

    if (!m_resourcesInitialized)
        initResources(device);

    // Initialize Panoramic
    if (m_currentPanoFilepath != filePath)
    {
        const std::string fullPath = FileHelper::GetAssetTextureFullPath(("EnvMaps/" + filePath).c_str()); // TODO
        //const std::string fullPath = R"(C:\Users\fionawright\source\repos\CherrySprout\Assets\Textures\Env Maps\autumn_field_puresky_4k.hdr)";
        const std::wstring fullPathW = stringToWString(fullPath);

        TexMetadata texMetadata;
        ScratchImage scratchImage;
        V(LoadFromHDRFile(fullPathW.c_str(), &texMetadata, scratchImage));

        m_pano.Init_Tex2D("Panoramic Environment Map", device, texMetadata.width, texMetadata.height, 1, texMetadata.format, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
        m_pano.UploadTexture(cmdList, &m_panoUploadHeap, scratchImage.GetPixels(), scratchImage.GetPixelsSize(), scratchImage.GetImage(0, 0, 0)->rowPitch);

        m_currentPanoFilepath = filePath;
    }

    {
        m_dsPanoToEA.Init(heap);
        m_dsPanoToEA.AddCBV(device, sizeof(CbvPanoToEA), uploadHeapCBV, "CBV Pano To EA");
        m_dsPanoToEA.SetSRV_Tex2D(device, 0, &m_pano, m_pano.GetDesc().Format);
        m_dsPanoToEA.SetUAV_Tex2D(device, 0, &m_ea, m_ea.GetDesc().Format);

        CbvPanoToEA cbv = {};
        cbv.OutputDimensions = hlsl::uint2(m_ea.GetDesc().Width, m_ea.GetDesc().Height);
        cbv.InputDimensions = hlsl::uint2(m_pano.GetDesc().Width, m_pano.GetDesc().Height);
        cbv.Rotation = rotation / 360.0f;
        m_dsPanoToEA.UpdateCBV(0, &cbv);
    }

    {
        m_pano.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_ea.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        heap->Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigPanoToEA.Get());
        cmdList->SetPipelineState(m_shaderPanoToEA.GetPSO());
        m_dsPanoToEA.SetDescriptorTables_Compute(cmdList);
    }

    const uint32_t groupSizeX = (m_ea.GetDesc().Width + 15) / 16;
    const uint32_t groupSizeY = (m_ea.GetDesc().Height + 15) / 16;
    cmdList->Dispatch(groupSizeX, groupSizeY, 1);

    m_ea.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void EnvironmentMap::InitCubemap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Heap* heap, UploadHeap* uploadHeap)
{
    GPU_SCOPE(cmdList, "Init CubeMap");

    assert(m_pano.IsInitialized());

    if (!m_cubemap.IsInitialized())
        m_cubemap.Init_Tex2D("Cubemap", device, 1024, 1024, 6, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Format = m_cubemap.GetDesc().Format;
        uavDesc.Texture2DArray.ArraySize = 6;
        uavDesc.Texture2DArray.MipSlice = 0;
        uavDesc.Texture2DArray.FirstArraySlice = 0;

        m_dsPanoToCM.Init(heap);
        m_dsPanoToCM.AddCBV(device, sizeof(CbvPanoToCM), uploadHeap, "CBV Cubemap");
        m_dsPanoToCM.SetSRV_Tex2D(device, 0, &m_pano, m_pano.GetDesc().Format);
        m_dsPanoToCM.SetUAV(device, 0, m_cubemap.GetResource(), uavDesc);

        CbvPanoToCM cbv = {};
        cbv.OutputWidth = m_cubemap.GetDesc().Width;
        cbv.InputDimensions = hlsl::uint2(m_pano.GetDesc().Width, m_pano.GetDesc().Height);
        cbv.Rotation = m_rotation / 360.0f;
        m_dsPanoToCM.UpdateCBV(0, &cbv);
    }

    {
        m_pano.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_cubemap.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        heap->Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigPanoToCM.Get());
        cmdList->SetPipelineState(m_shaderPanoToCM.GetPSO());
        m_dsPanoToCM.SetDescriptorTables_Compute(cmdList);
    }

    const uint32_t groupSizeXY = (m_cubemap.GetDesc().Width + 15) / 16;
    cmdList->Dispatch(groupSizeXY, groupSizeXY, 6);

    //TextureLoader::CreateMipMapsCubemap(device, cmdList, m_cubemap.GetD12Resource());

    m_cubemap.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void EnvironmentMap::initResources(ID3D12Device* device)
{
    m_panoUploadHeap.Init(device, sizeof(float) * 4 * 4096 * 2048);

    m_ea.Init_Tex2D("Equal-Area Envmap", device, 4096, 4096, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_STATIC_SAMPLER_DESC sampler;
    InitializeSamplerLinearClamp(&sampler);

    m_rootSigPanoToEA.SmartInit(device, 1, 1, 1, false, &sampler, 1);
    m_shaderPanoToEA.InitCompute(device, "Compute/PanoToEaCS.hlsl", m_rootSigPanoToEA.Get());

    m_rootSigPanoToCM.SmartInit(device, 1, 1, 1, false, &sampler, 1);
    m_shaderPanoToCM.InitCompute(device, "Compute/PanoToCubemapCS.hlsl", m_rootSigPanoToCM.Get());

    m_resourcesInitialized = true;
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