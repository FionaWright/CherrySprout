//
// Created by fionaw on 01/06/2026.
//

#include "System/pch.h"
#include "PathTracer.h"

#include "Debug/GPUEventScoped.h"
#include "PathTracing/CBVs.h"
#include "System/HighResolutionClock.h"
#include "Utils/D3DUtils.h"

void PathTracer::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);

    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Width = Config::GetSystem().RtvWidth;
        desc.Height = Config::GetSystem().RtvHeight;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        m_output.Init("Output", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.Width = Config::GetSystem().RtvWidth;
        desc.Height = Config::GetSystem().RtvHeight;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        m_accum.Init("Accum", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    m_rootSigPT.SmartInit(d3d->GetDevice(), 1, 0, 2, false, &sampler, 1);

    m_descriptorSet.Init(heap, false);
    m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvPathTracingSettings), uploadHeapCBV);
    m_descriptorSet.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_accum);
    m_descriptorSet.SetUAV_Tex2D(d3d->GetDevice(), 1, &m_output);

    m_shaderPT.InitCs("PathTracing/PathTracerCS.hlsl", d3d->GetDevice(), m_rootSigPT.Get());
}

void PathTracer::Update(D3D* d3d, TimeArgs timeArgs)
{
}

void PathTracer::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const Heap* heap)
{
    GPU_SCOPE(cmdList, "Path-Trace");

    // Fill Settings
    // TODO: push constants
    {
        CbvPathTracingSettings settings;
        settings.AccumulationEnabled = true;
        settings.FrameDimensions = { Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight };
        m_descriptorSet.UpdateCBV(0, &settings);
    }

    m_output.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_accum.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    heap->Bind(cmdList);
    cmdList->SetComputeRootSignature(m_rootSigPT.Get());
    cmdList->SetPipelineState(m_shaderPT.GetPSO());
    m_descriptorSet.SetDescriptorTables_Compute(cmdList);

    constexpr uint32_t THREAD_COUNTS = 16;
    const uint32_t groupX = (Config::GetSystem().RtvWidth + (THREAD_COUNTS-1)) / THREAD_COUNTS;
    const uint32_t groupY = (Config::GetSystem().RtvHeight + (THREAD_COUNTS-1)) / THREAD_COUNTS;
    cmdList->Dispatch(groupX, groupY, 1);
}
