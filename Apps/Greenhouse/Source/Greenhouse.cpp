#include "System/pch.h"

#include "Greenhouse.h"

#include "HWI/D3D.h"
#include "PathTracing/CBVs.h"
#include "Scene/SceneManager.h"
#include "System/HighResolutionClock.h"
#include "Utils/Helper.h"
#include "Utils/D3DUtils.h"

void Greenhouse::Init(D3D* d3d)
{
    App::Init(d3d);

    m_sceneManager.LoadScene(
        R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\assets\full_assets\OpenChessSet\chess_set.usda)");

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);

    m_heap.Init("Test Heap", d3d->GetDevice(), 20000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_uploadHeapCBV.Init(d3d->GetDevice(), sizeof(CbvPathTracingSettings) + 256);

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

    m_rootSigPT.SmartInit(d3d->GetDevice(), 1, 6, 2, true, &sampler, 1);

    m_descriptorSet.Init(&m_heap, true);
    m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvPathTracingSettings), &m_uploadHeapCBV);
    m_descriptorSet.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_accum);
    m_descriptorSet.SetUAV_Tex2D(d3d->GetDevice(), 1, &m_output);

    m_shaderPT.InitCs("PathTracing/PathTracerCS.hlsl", d3d->GetDevice(), m_rootSigPT.Get());
}

void Greenhouse::Update(D3D* d3d, const TimeArgs timeArgs)
{
}

void Greenhouse::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    // Upload Scene
    if (m_sceneManager.IsGpuDataDirty())
    {
        m_sceneManager.UploadScene(d3d, cmdList);
    }

    // Render Path-Tracer
    {
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

        m_heap.Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigPT.Get());
        cmdList->SetPipelineState(m_shaderPT.GetPSO());
        m_descriptorSet.SetDescriptorTables_Compute(cmdList);

        constexpr uint32_t THREAD_COUNTS = 16;
        const uint32_t groupX = (Config::GetSystem().RtvWidth + (THREAD_COUNTS-1)) / THREAD_COUNTS;
        const uint32_t groupY = (Config::GetSystem().RtvHeight + (THREAD_COUNTS-1)) / THREAD_COUNTS;
        cmdList->Dispatch(groupX, groupY, 1);
    }

    // Copy to RTV
    {
        D12Resource* rtv = d3d->GetRtv();

        m_output.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        rtv->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        rtv->CopyTextureInto(cmdList, m_output.GetResource(), 0, 0, 0);
    }
}

void Greenhouse::PostUpdate(D3D* d3d)
{
}

void Greenhouse::RenderGUI()
{
}
