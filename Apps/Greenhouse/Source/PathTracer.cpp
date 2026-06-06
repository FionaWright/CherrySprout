//
// Created by fionaw on 01/06/2026.
//

#include "System/pch.h"
#include "PathTracer.h"

#include "Greenhouse.h"
#include "Debug/GPUEventScoped.h"
#include "PathTracing/CBVs.h"
#include "System/HighResolutionClock.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

void PathTracer::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    IRenderBackend::Init(d3d, heap, uploadHeapCBV);

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

    m_rootSig.SmartInit(d3d->GetDevice(), 1, 5, 2, false, &sampler, 1);

    m_descriptorSet.Init         (heap, false);
    m_descriptorSet.AddCBV       (d3d->GetDevice(), sizeof(CbvPathTracingSettings), uploadHeapCBV);
    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 0, &m_accum);
    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 1, &m_output);
    m_descriptorSet.SetSRV_RTAS  (d3d->GetDevice(), 0, nullptr);

    auto desc = CreateComputePipelineDesc(m_rootSig.Get());
    m_pipeline.InitCompute(d3d->GetDevice(), "PathTracing/0_PathTracerCS.hlsl", desc);
}

void PathTracer::LoadSceneData(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Scene* scene)
{
    IRenderBackend::LoadSceneData(d3d, cmdList, scene);

    {
        GPU_SCOPE(cmdList, "Build RTAS");

        ComPtr<ID3D12Device5> device5;
        V(d3d->GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));
        ComPtr<ID3D12GraphicsCommandList4> cmdList4;
        V(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

        m_rtasBuilder.Build(device5.Get(), cmdList4.Get(), scene);
    }

    m_descriptorSet.SetSRV_RTAS  (d3d->GetDevice(), 0, m_rtasBuilder.GetRtasResource());
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 1, &scene->GPU.MegaBufferVertex, scene->CPU.MegaBufferVertex.size(), sizeof(Vertex));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 2, &scene->GPU.MegaBufferIndex, scene->CPU.MegaBufferIndex.size(), sizeof(uint32_t));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 3, &scene->GPU.MegaBufferInstanceData, scene->CPU.Objects.size(), sizeof(InstanceData));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 4, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterials.size(), sizeof(Material));
}

void PathTracer::Update(D3D* d3d, TimeArgs timeArgs)
{

}

void PathTracer::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo)
{
    GPU_SCOPE(cmdList, "Path-Trace");

    Scene* scene = renderInfo.Scene;
    const Heap* heap = renderInfo.Heap;

    // Fill Settings
    // TODO: push constants
    {
        CbvPathTracingSettings settings;
        XMStoreFloat4x4(&settings.InvP, *renderInfo.InvP);
        XMStoreFloat4x4(&settings.InvV, *renderInfo.InvV);
        settings.FrameIdx = m_frameIdx;
        settings.CameraPositionWorld = renderInfo.Camera->GetPosition();

        settings.MaxRayDepth = renderInfo.PathTracerConfig->MaxRayDepth;
        settings.RussianRouletteMinBounces = renderInfo.PathTracerConfig->RussianRouletteMinBounces;
        settings.SPP = renderInfo.PathTracerConfig->SPP;
        settings.FireflyThreshold = renderInfo.PathTracerConfig->FireFlyThreshold;

        settings.DirLightDirection = renderInfo.BackendConfig->DirLightDirection;
        settings.DirLightColor = renderInfo.BackendConfig->DirLightColor;
        settings.DirLightCosAngularRadius = renderInfo.BackendConfig->DirLightCosTheta;
        settings.DirLightIntensity = renderInfo.BackendConfig->DirLightIntensity;

        // TODO: Do I need both?
        settings.FrameDimensions = { Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight };
        settings.TexelSize = XMFLOAT2(1.0f / (float)settings.FrameDimensions.x, 1.0f / (float)settings.FrameDimensions.y);
        m_descriptorSet.UpdateCBV(0, &settings);
    }

    {
        m_output.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_accum.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        scene->GPU.MegaBufferVertex.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        scene->GPU.MegaBufferIndex.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        scene->GPU.MegaBufferInstanceData.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        scene->GPU.MegaBufferMaterials.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    }

    heap->Bind(cmdList);
    cmdList->SetComputeRootSignature(m_rootSig.Get());
    cmdList->SetPipelineState(m_pipeline.GetPSO());
    m_descriptorSet.SetDescriptorTables_Compute(cmdList);

    constexpr uint32_t THREAD_COUNTS = 16;
    const uint32_t groupX = (Config::GetSystem().RtvWidth + (THREAD_COUNTS-1)) / THREAD_COUNTS;
    const uint32_t groupY = (Config::GetSystem().RtvHeight + (THREAD_COUNTS-1)) / THREAD_COUNTS;
    cmdList->Dispatch(groupX, groupY, 1);

    // Copy to RTV
    {
        GPU_SCOPE(cmdList, "Copy PT Output to RTV");

        D12Resource* rtv = d3d->GetRtv();

        m_output.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        rtv->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        rtv->CopyTextureInto(cmdList, m_output.GetResource(), Config::GetSystem().WindowAppGuiWidth, 0, 0);
    }

    m_frameIdx++;
}

void PathTracer::UnreserveData()
{

}

void PathTracer::UpdatePipeline(ID3D12Device* device, const PathTracerFeatureFlags& featureFlags, const PathTracerDebugFlags& debugFlags)
{
    std::vector<std::string> compileArgs = {};

    compileArgs.emplace_back("-DFEATURE_FLAGS=" + std::to_string(featureFlags));

    compileArgs.emplace_back("-DDEBUG_FLAGS=" + std::to_string(debugFlags));

    auto desc = CreateComputePipelineDesc(m_rootSig.Get());
    m_pipeline.SetCompute(device, "PathTracing/0_PathTracerCS.hlsl", desc, compileArgs);
}
