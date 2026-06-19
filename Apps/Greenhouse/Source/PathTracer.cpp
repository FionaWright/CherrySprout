//
// Created by fionaw on 01/06/2026.
//

#include "System/pch.h"
#include "PathTracer.h"

#include "Greenhouse.h"
#include "imgui.h"
#include "Debug/GPUEventScoped.h"
#include "../../../Assets/Shaders/Utils/CBVs.h"
#include "System/HighResolutionClock.h"
#include "Utils/ConstantsCpp.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"
#include "Utils/Debug/DebugID.h"

void PathTracer::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    IRenderBackend::Init(d3d, heap, uploadHeapCBV);

    CherryPrint("Initializing Path-Tracer...");

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
        m_output.Init("Output", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
    }

    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        desc.Width = Config::GetSystem().RtvWidth;
        desc.Height = Config::GetSystem().RtvHeight;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        m_accum.Init("Accum", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
    }

#ifdef _DEBUG
    {
        constexpr size_t bufferSize = _countof(s_debugIdList) * sizeof(DebugErrorInfo);
        m_gpuErrorInfoRW.Init_Buffer("Error Info R/W", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
        m_gpuErrorInfoReadback.Init_Buffer("Error Info Readback", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_NONE, true, D3D12_RESOURCE_STATE_COPY_DEST);
    }
#endif

    UpdatePipeline(d3d->GetDevice(), s_defaultFeatureFlags, s_defaultDebugFlags, s_defaultOutputIndex, s_defaultBxdfMode);

    CherryPrint("Path-Tracer Initialized");
}

void PathTracer::LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap, LightImportanceSampler* lightImportanceSampler)
{
    IRenderBackend::LoadSceneData(d3d, scene, heap, uploadHeapCBV, envMap, lightImportanceSampler);

    {
        CherryPrint("PT: Building RTAS");

        // Note: Direct queue is required as you can't transition from D3D12_RESOURCE_STATE_INDEX_BUFFER in a compute queue. Buffer may be left in that state from previous rasterization passes
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();

        ComPtr<ID3D12Device5> device5;
        V(d3d->GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));
        ComPtr<ID3D12GraphicsCommandList4> cmdList4;
        V(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

        m_rtasBuilder.Build(device5.Get(), cmdList4.Get(), scene);

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();

        CherryPrint("PT: RTAS Built");
    }

    m_descriptorSet.Init         (heap, true);

    m_descriptorSet.AddCBV       (d3d->GetDevice(), sizeof(CbvPathTracingSettings), uploadHeapCBV);

    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 0, &m_accum, m_accum.GetDesc().Format);
    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 1, &m_output, m_output.GetDesc().Format);

#ifdef _DEBUG
    m_descriptorSet.SetUAV_Buffer(d3d->GetDevice(), 2, &m_gpuErrorInfoRW, _countof(s_debugIdList), sizeof(DebugErrorInfo));
#endif

    m_descriptorSet.SetSRV_RTAS  (d3d->GetDevice(), 0, m_rtasBuilder.GetRtasResource());
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 1, &scene->GPU.MegaBufferVertex, scene->CPU.MegaBufferVertexCount, sizeof(Vertex));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 2, &scene->GPU.MegaBufferIndex, scene->CPU.MegaBufferIndexCount, sizeof(uint32_t));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 3, &scene->GPU.MegaBufferInstanceData, scene->CPU.ObjectCount, sizeof(InstanceData));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 4, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));
    m_descriptorSet.SetSRV_Tex2D (d3d->GetDevice(), 5, envMap->GetEA(), envMap->GetEA()->GetDesc().Format);

    if (lightImportanceSampler->IsInitialized())
    {
        m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 6, lightImportanceSampler->GetEnvMapPmf(), lightImportanceSampler->GetEnvMapPmf()->GetDesc().Format);
        m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 7, lightImportanceSampler->GetEnvMapCdfConditional(), lightImportanceSampler->GetEnvMapCdfConditional()->GetDesc().Format);
        m_descriptorSet.SetSRV_Tex1D(d3d->GetDevice(), 8, lightImportanceSampler->GetEnvMapCdfMarginal(), lightImportanceSampler->GetEnvMapCdfMarginal()->GetDesc().Format);
    }
}

void PathTracer::Update(D3D* d3d, TimeArgs timeArgs)
{

}

void PathTracer::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo)
{
#ifdef _DEBUG
    if (GetPathTracerDebugFlag(renderInfo.PathTracerConfig->DebugFlags, eDebug_Asserts))
    {
        d3d->Flush();
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();

        m_gpuErrorInfoRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_gpuErrorInfoReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        constexpr size_t bufferSize = _countof(s_debugIdList) * sizeof(DebugErrorInfo);
        cmdList->CopyBufferRegion(m_gpuErrorInfoReadback.GetResource(), 0, m_gpuErrorInfoRW.GetResource(), 0, bufferSize);

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();

        m_gpuErrorInfoReadback.Readback(&m_cpuErrorInfo);
    }
#endif
}

void PathTracer::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo)
{
    GPU_SCOPE(cmdList, "Path-Trace");

    Scene* scene = renderInfo.Scene;
    const Heap* heap = renderInfo.Heap;

    // Fill Settings
    {
        CbvPathTracingSettings settings;
        XMStoreFloat4x4(&settings.InvP, renderInfo.InvP);
        XMStoreFloat4x4(&settings.InvV, renderInfo.InvV);
        settings.FrameIdx = m_frameIdx;
        settings.CameraPositionWorld = renderInfo.Camera->GetPosition();

        settings.MaxRayDepth = renderInfo.PathTracerConfig->MaxRayDepth;
        settings.MaxShadowRayDepth = renderInfo.PathTracerConfig->MaxShadowRayDepth;
        settings.RussianRouletteMinBounces = renderInfo.PathTracerConfig->RussianRouletteMinBounces;
        settings.SPP = renderInfo.PathTracerConfig->SPP;
        settings.FireflyThreshold = renderInfo.PathTracerConfig->FireFlyThreshold;
        settings.IsMaxFramesReached = renderInfo.PathTracerConfig->MaxFrameNumber != 0 && m_frameIdx > renderInfo.PathTracerConfig->MaxFrameNumber;

        settings.DirLightDirection = renderInfo.BackendConfig->DirLightDirection;
        settings.DirLightColor = renderInfo.BackendConfig->DirLightColor;
        settings.DirLightCosAngularRadius = 1.0f - renderInfo.BackendConfig->DirLightCosAngularRadius;
        settings.DirLightIntensity = renderInfo.BackendConfig->DirLightIntensity;

        settings.DofFocalDist = renderInfo.PathTracerConfig->DofFocalDist;
        settings.DofLensRadius = renderInfo.PathTracerConfig->DofLensRadius;

        settings.FrameDimensions = { Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight };
        settings.TexelSize = XMFLOAT2(1.0f / (float)settings.FrameDimensions.x, 1.0f / (float)settings.FrameDimensions.y);
        m_descriptorSet.UpdateCBV(0, &settings);
    }

    {
        m_output.Transition                             (cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_accum.Transition                              (cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        scene->GPU.MegaBufferVertex.Transition          (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        scene->GPU.MegaBufferIndex.Transition           (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        scene->GPU.MegaBufferInstanceData.Transition    (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        scene->GPU.MegaBufferMaterials.Transition       (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        renderInfo.EnvironmentMap->GetEA()->Transition  (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    }

    cmdList->SetComputeRootSignature(m_rootSig.Get());
    cmdList->SetPipelineState(m_pipeline.GetPSO());
    heap->Bind(cmdList);
    heap->BindSceneTextures_Compute(cmdList);
    m_descriptorSet.SetDescriptorTables_Compute(cmdList);

    constexpr uint32_t THREAD_COUNTS = 16;
    DispatchOverTexture(cmdList, THREAD_COUNTS, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

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

void PathTracer::UpdatePipeline(ID3D12Device* device, const PathTracerFeatureFlags& featureFlags, const PathTracerDebugFlags& debugFlags, const DebugOutputIndex& debugOutputIdx, const BxdfMode& bxdfMode)
{
    uint32_t numSRV = 6;
    uint32_t numUAV = 2;
#ifdef _DEBUG
    numUAV++; // gDbgBufferErrorInfo
#endif

    if (GetPathTracerFeatureFlag(featureFlags, eFeature_NEE))
        numSRV += 3; // gEnvMapCdfConditional, gEnvMapPmfConditional, gEnvMapCdfMarginal

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);
    m_rootSig.SmartInit(device, 1, numSRV, numUAV, true, &sampler, 1);

    std::vector<std::string> compileArgs = {};
    compileArgs.emplace_back("-DFEATURE_FLAGS=" + std::to_string(featureFlags));
    compileArgs.emplace_back("-DDEBUG_FLAGS=" + std::to_string(debugFlags));
    compileArgs.emplace_back("-DBXDF_MODE=" + std::to_string(static_cast<uint32_t>(bxdfMode)));

    if (debugOutputIdx != DebugOutputIndex::eDebugOutput_Disabled)
        compileArgs.emplace_back("-DDEBUG_OUTPUT_COLOR=" + std::to_string(static_cast<uint32_t>(debugOutputIdx)));

    auto desc = CreateComputePipelineDesc(m_rootSig.Get());
    m_pipeline.InitCompute(device, "PathTracing/0_PathTracerCS.hlsl", desc, compileArgs);

    Reset();
}

void PathTracer::Reset()
{
    m_frameIdx = 0;
}

#ifdef _DEBUG
void PathTracer::RenderGUI_ErrorInfo()
{
    std::vector<uint32_t> errors;
    for (int i = 0; i < _countof(m_cpuErrorInfo); i++)
    {
        if (m_cpuErrorInfo[i].ExprCounter > 0 || m_cpuErrorInfo[i].NaNCounter > 0 || m_cpuErrorInfo[i].InfCounter > 0)
        {
            errors.emplace_back(i);
        }
    }

    if (errors.size() == 0)
        return;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(224/255., 82/255., 110/255., 255));
    ImGui::Text("%s", "(!) Assertion Errors:");

    ImGui::Indent(IM_GUI_INDENTATION);
    for (int i = 0; i < errors.size(); i++)
    {
        if (i > 0)
            ImGui::Spacing();

        const uint32_t dbgId = errors[i];
        ImGui::Text("%s: ", s_debugIdList[dbgId]);
        ImGui::SetItemTooltip("%s: ", s_debugIdList[dbgId]);

        if (m_cpuErrorInfo[dbgId].ExprCounter > 0)
        {
            ImGui::Text(" EXPR=%u", m_cpuErrorInfo[dbgId].ExprCounter);
            ImGui::SetItemTooltip(" EXPR=%u", m_cpuErrorInfo[dbgId].ExprCounter);

            ImGui::Text(" V1=(%f, %f, %f, %f)", m_cpuErrorInfo[dbgId].Value1.x, m_cpuErrorInfo[dbgId].Value1.y,m_cpuErrorInfo[dbgId].Value1.z, m_cpuErrorInfo[dbgId].Value1.w);
            ImGui::Text(" V2=(%f, %f, %f, %f)", m_cpuErrorInfo[dbgId].Value2.x, m_cpuErrorInfo[dbgId].Value2.y,m_cpuErrorInfo[dbgId].Value2.z, m_cpuErrorInfo[dbgId].Value2.w);
            ImGui::Text(" V3=(%f, %f, %f, %f)", m_cpuErrorInfo[dbgId].Value3.x, m_cpuErrorInfo[dbgId].Value3.y,m_cpuErrorInfo[dbgId].Value3.z, m_cpuErrorInfo[dbgId].Value3.w);
        }

        if (m_cpuErrorInfo[dbgId].NaNCounter > 0)
        {
            ImGui::Text(" NAN=%u",m_cpuErrorInfo[dbgId].NaNCounter);
            ImGui::SetItemTooltip(" NAN=%u",m_cpuErrorInfo[dbgId].NaNCounter);
        }

        if (m_cpuErrorInfo[dbgId].InfCounter > 0)
        {
            ImGui::Text(" INF=%u", m_cpuErrorInfo[dbgId].InfCounter);
            ImGui::SetItemTooltip(" INF=%u", m_cpuErrorInfo[dbgId].InfCounter);
        }
    }
    ImGui::Unindent(IM_GUI_INDENTATION);
    ImGui::Spacing();

    ImGui::PopStyleColor();
}
#endif