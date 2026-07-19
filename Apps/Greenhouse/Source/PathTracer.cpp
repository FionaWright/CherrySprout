//
// Created by fionaw on 01/06/2026.
//

#include "System/pch.h"
#include "PathTracer.h"

#include "Greenhouse.h"
#include "imgui.h"
#include "Debug/GPUEventScoped.h"
#include "Utils/CBVs.h"
#include "Scene/InstanceData.h"
#include "System/HighResolutionClock.h"
#include "Utils/ConstantsCpp.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"
#include "Utils/Debug/DebugID.h"
#include "Utils/Debug/DebugStructs.h"

#ifdef _DEBUG
#include "Debug/Profiler.h"
#endif

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

#if CHERRY_DEBUG_FEATURES_ENABLED
    {
        constexpr size_t bufferSize = _countof(s_debugIdList) * sizeof(DebugErrorInfo);
        m_gpuErrorInfoRW.Init_Buffer("Error Info (RW)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
        m_gpuErrorInfoReadback.Init_Buffer("Error Info (Readback)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_NONE, true, D3D12_RESOURCE_STATE_COPY_DEST);
    }

    {
        constexpr size_t bufferSize = sizeof(RayDump) * PATH_DUMP_MAX_RAY_DEPTH;
        m_pathDumpBufferRW.Init_Buffer("Path Dump Buffer (RW)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_pathDumpBufferReadback.Init_Buffer("Path Dump Buffer (Readback)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_NONE, true, D3D12_RESOURCE_STATE_COPY_DEST);
    }
#endif

    constexpr PathTracingDebugInfo debugInfo = PathTracingDebugInfo();
    UpdatePipeline(d3d->GetDevice(), s_defaultFeatureFlags, debugInfo, s_defaultBxdfMode);

    m_descriptorSet.Init         (heap, true);
    m_descriptorSet.AddCBV       (d3d->GetDevice(), sizeof(CbvPathTracingSettings), uploadHeapCBV);
    m_descriptorSet.AddCBV       (d3d->GetDevice(), sizeof(CbvPathTracingDebugSettings), uploadHeapCBV);

    CherryPrint("Path-Tracer Initialized");
}

void PathTracer::LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap, LightImportanceSampler* lightImportanceSampler, GBufferPrePass* gbuffer)
{
    IRenderBackend::LoadSceneData(d3d, scene, heap, uploadHeapCBV, envMap, lightImportanceSampler, gbuffer);

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

    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 0, &m_accum, m_accum.GetDesc().Format);
    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 1, &m_output, m_output.GetDesc().Format);

#if CHERRY_DEBUG_FEATURES_ENABLED
    m_descriptorSet.SetUAV_Buffer(d3d->GetDevice(), 2, &m_gpuErrorInfoRW, _countof(s_debugIdList), sizeof(DebugErrorInfo));
    m_descriptorSet.SetUAV_Buffer(d3d->GetDevice(), 3, &m_pathDumpBufferRW, PATH_DUMP_MAX_RAY_DEPTH, sizeof(RayDump));
#endif

    m_descriptorSet.SetSRV_RTAS  (d3d->GetDevice(), 0, m_rtasBuilder.GetRtasResource());
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 1, &scene->GPU.MegaBufferVertex, scene->CPU.MegaBufferVertexCount, sizeof(Vertex));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 2, &scene->GPU.MegaBufferIndex, scene->CPU.MegaBufferIndexCount, sizeof(uint32_t));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 3, &scene->GPU.MegaBufferInstanceData, scene->CPU.ObjectCount, sizeof(InstanceData));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 4, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 5, &scene->GPU.MegaBufferPunctualLights, scene->CPU.MegaBufferPunctualLightsCount, sizeof(PunctualLight));

    m_descriptorSet.SetSRV_Tex2D (d3d->GetDevice(), 6, envMap->GetEA(), envMap->GetEA()->GetDesc().Format);

    if (lightImportanceSampler->IsInitialized())
    {
        const size_t lightCount = 1 + scene->CPU.MegaBufferPunctualLightsCount;
        m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 7, lightImportanceSampler->GetEnvMapPmf(), lightImportanceSampler->GetEnvMapPmf()->GetDesc().Format);
        m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 8, lightImportanceSampler->GetEnvMapCdfConditional(), lightImportanceSampler->GetEnvMapCdfConditional()->GetDesc().Format);
        m_descriptorSet.SetSRV_Tex1D(d3d->GetDevice(), 9, lightImportanceSampler->GetEnvMapCdfMarginal(), lightImportanceSampler->GetEnvMapCdfMarginal()->GetDesc().Format);
        m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 10, lightImportanceSampler->GetLightsCdf(), lightCount, sizeof(ProbabilityDistributionSample));
    }

    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 11, gbuffer->GetGBufferMaterialIdx(), gbuffer->GetGBufferMaterialIdx()->GetDesc().Format);
    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 12, gbuffer->GetGBufferNormals(), gbuffer->GetGBufferNormals()->GetDesc().Format);
    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 13, gbuffer->GetGBufferDepth(), GBUFFER_FORMAT_DEPTH_SRV);
    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 14, gbuffer->GetGBufferUvMv(), gbuffer->GetGBufferUvMv()->GetDesc().Format);
}

void PathTracer::Update(D3D* d3d, TimeArgs timeArgs)
{

}

void PathTracer::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo)
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    if (GetPathTracerDebugFlag(renderInfo.PathTracerConfig->DebugInfo.Flags, eDebug_Asserts))
    {
        d3d->Flush();

        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();

        m_gpuErrorInfoRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_gpuErrorInfoReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        constexpr size_t bufferSize = _countof(s_debugIdList) * sizeof(DebugErrorInfo);
        cmdList->CopyBufferRegion(m_gpuErrorInfoReadback.GetResource(), 0, m_gpuErrorInfoRW.GetResource(), 0, bufferSize);

        UploadHeap uploadHeapClear;
        if (m_scheduleClearErrors)
        {
            uploadHeapClear.Init(d3d->GetDevice(), Align(m_gpuErrorInfoRW.GetIntermediateSize(), 512));

            DebugErrorInfo debugErrorInfoClear{};
            debugErrorInfoClear.ExprCounter = 0;
            debugErrorInfoClear.NaNCounter = 0;
            debugErrorInfoClear.InfCounter = 0;

            const std::vector<DebugErrorInfo> cpuClearBuffer(_countof(s_debugIdList), debugErrorInfoClear);
            m_gpuErrorInfoRW.UploadBuffer(cmdList, &uploadHeapClear, cpuClearBuffer.data(), bufferSize);

            m_scheduleClearErrors = false;
        }

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();

        m_gpuErrorInfoReadback.Readback(&m_cpuErrorInfo);
    }

#define PATH_DUMP_UPDATE_COOLDOWN 300

    const bool scheduledRun = m_scheduledRunState == ScheduledRunState::eReadbackPathDump;
    const bool needPathDump = m_isPathDumpAutomatic || scheduledRun;
    if (GetPathTracerDebugFlag(renderInfo.PathTracerConfig->DebugInfo.Flags, eDebug_PathDumper) && needPathDump)
    {
        static int s_pathDumpTimer = PATH_DUMP_UPDATE_COOLDOWN;
        if (s_pathDumpTimer > 0 && !scheduledRun)
        {
            s_pathDumpTimer--;
            return;
        }
        s_pathDumpTimer = PATH_DUMP_UPDATE_COOLDOWN;

        d3d->Flush();

        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();

        m_pathDumpBufferRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_pathDumpBufferReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        constexpr size_t bufferSize = PATH_DUMP_MAX_RAY_DEPTH * sizeof(RayDump);
        cmdList->CopyBufferRegion(m_pathDumpBufferReadback.GetResource(), 0, m_pathDumpBufferRW.GetResource(), 0, bufferSize);

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();

        m_pathDumpBufferReadback.Readback(&m_cpuPathDump);

        m_dumpedPathPixelCoords = scheduledRun ? m_scheduledRunPixelCoords : renderInfo.PathTracerConfig->DebugInfo.ChosenPixelCoords;
        m_dumpedPathFrameIdx = scheduledRun ? m_scheduledRunFrameIdx : m_frameIdx;
        m_dumpedPathCameraPosition = scheduledRun ? m_scheduledRunCameraPosition : renderInfo.Camera->GetPosition();
        m_dumpedPathViewMatrix = scheduledRun ? m_scheduledRunViewMatrix : renderInfo.Camera->GetViewMatrix();

        if (scheduledRun)
            m_scheduledRunState = ScheduledRunState::eDisplay;
    }
#endif
}

void PathTracer::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* RTV)
{
    GPU_SCOPE(cmdList, "Path-Trace");

    Scene* scene = renderInfo.Scene;
    const Heap* heap = renderInfo.Heap;

    // Fill Settings
    {
        CbvPathTracingSettings settings{};
        XMStoreFloat4x4(&settings.InvP, renderInfo.InvP);
        XMStoreFloat4x4(&settings.InvV, renderInfo.InvV);
        settings.FrameIdx = m_frameIdx;
        settings.CameraPositionWorld = renderInfo.Camera->GetPosition();

#if CHERRY_DEBUG_FEATURES_ENABLED
        const bool outputColorEnabled = GetPathTracerDebugFlag(renderInfo.PathTracerConfig->DebugInfo.Flags, eDebug_OutputColor);
        const bool pathDumpEnabled = GetPathTracerDebugFlag(renderInfo.PathTracerConfig->DebugInfo.Flags, eDebug_PathDumper);
        const bool scalesEnabled = GetPathTracerDebugFlag(renderInfo.PathTracerConfig->DebugInfo.Flags, eDebug_Scales);
        if (outputColorEnabled || pathDumpEnabled || scalesEnabled)
        {
            const hlsl::uint2 chosenPixelCoords = m_scheduledRunState == ScheduledRunState::eRunFrame ? m_scheduledRunPixelCoords : renderInfo.PathTracerConfig->DebugInfo.ChosenPixelCoords;

            CbvPathTracingDebugSettings debugSettings{};
            debugSettings.OutputColorIdx = static_cast<uint32_t>(renderInfo.PathTracerConfig->DebugInfo.OutputColorIdx);
            debugSettings.OutputColorRemapIdx = static_cast<uint32_t>(renderInfo.PathTracerConfig->DebugInfo.OutputColorRemap);
            debugSettings.ChosenRayDepth = renderInfo.PathTracerConfig->DebugInfo.ChosenRayDepth;
            debugSettings.ChosenPixelCoords = chosenPixelCoords;
            debugSettings.ScaleIntensityGlobal = renderInfo.PathTracerConfig->DebugInfo.ScaleIntensityGlobal;
            debugSettings.ScaleIntensityPunctual = renderInfo.PathTracerConfig->DebugInfo.ScaleIntensityPunctual;
            debugSettings.ScaleIntensityPoint = renderInfo.PathTracerConfig->DebugInfo.ScaleIntensityPoint;
            debugSettings.ScaleIntensityDistant = renderInfo.PathTracerConfig->DebugInfo.ScaleIntensityDistant;
            debugSettings.ScaleIntensitySpot = renderInfo.PathTracerConfig->DebugInfo.ScaleIntensitySpot;
            debugSettings.ScaleIntensityEnvMap = renderInfo.PathTracerConfig->DebugInfo.ScaleIntensityEnvMap;
            debugSettings.ScalePointLightRadius = renderInfo.PathTracerConfig->DebugInfo.ScalePointLightRadius;
            debugSettings.ScaleF = renderInfo.PathTracerConfig->DebugInfo.ScaleF;
            debugSettings.ScaleD = renderInfo.PathTracerConfig->DebugInfo.ScaleD;
            debugSettings.ScaleG = renderInfo.PathTracerConfig->DebugInfo.ScaleG;
            debugSettings.ScaleDiffuse = renderInfo.PathTracerConfig->DebugInfo.ScaleDiffuse;
            debugSettings.ScaleSpecular = renderInfo.PathTracerConfig->DebugInfo.ScaleSpecular;
            debugSettings.ScaleReflect = renderInfo.PathTracerConfig->DebugInfo.ScaleReflect;
            debugSettings.ScaleRefract = renderInfo.PathTracerConfig->DebugInfo.ScaleRefract;
            m_descriptorSet.UpdateCBV(1, &debugSettings);
        }

        if (m_scheduledRunState == ScheduledRunState::eRunFrame)
        {
            settings.FrameIdx = m_scheduledRunFrameIdx;
            settings.CameraPositionWorld = m_scheduledRunCameraPosition;
            XMStoreFloat4x4(&settings.InvV, XMMatrixInverse(nullptr, m_scheduledRunViewMatrix));

            SetPathTracerFeatureFlag(renderInfo.PathTracerConfig->FeatureFlags, eFeature_Accumulation, false);
            d3d->Flush();
            UpdatePipeline(d3d->GetDevice(), renderInfo.PathTracerConfig->FeatureFlags, renderInfo.PathTracerConfig->DebugInfo, renderInfo.PathTracerConfig->BxdfMode);

            m_scheduledRunState = ScheduledRunState::eReadbackPathDump;
        }
#endif

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

#if CHERRY_DEBUG_FEATURES_ENABLED
    if (m_scheduledRunState != ScheduledRunState::eDisplay)
#endif
    {
        cmdList->SetComputeRootSignature(m_rootSig.Get());
        cmdList->SetPipelineState(m_pipeline.GetPSO());
        heap->Bind(cmdList);
        heap->BindSceneTextures_Compute(cmdList, m_rootSig.GetParamIndexSceneTextures());
        m_descriptorSet.SetDescriptorTables_Compute(cmdList);

        constexpr uint32_t THREAD_COUNTS = 16;
        DispatchOverTexture(cmdList, THREAD_COUNTS, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_frameIdx++;
    }

    // Copy to RTV
    {
        GPU_SCOPE(cmdList, "Copy PT Output to RTV");

        m_output.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        RTV->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        RTV->CopyTextureInto(cmdList, m_output.GetResource(), 0, 0, 0);
    }
}

void PathTracer::UnreserveData()
{
    m_output.Release();
    m_accum.Release();
#if CHERRY_DEBUG_FEATURES_ENABLED
    m_gpuErrorInfoRW.Release();
    m_gpuErrorInfoReadback.Release();
    m_pathDumpBufferRW.Release();
    m_pathDumpBufferReadback.Release();
#endif
    m_isInitialized = false;
    m_currentlyLoadedScene = "";
}

void PathTracer::UpdatePipeline(ID3D12Device* device, const PathTracerFeatureFlags& featureFlags, const PathTracingDebugInfo& debugInfo, const BxdfMode& bxdfMode)
{
    constexpr uint32_t numCBV = 2;
    constexpr uint32_t numSRV = 15;
    constexpr uint32_t numUAV = 4;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);
    m_rootSig.SmartInit(device, numCBV, numSRV, numUAV, true, &sampler, 1);

    std::vector<std::string> compileArgs = {};
    compileArgs.emplace_back("-DBXDF_MODE=" + std::to_string(static_cast<uint32_t>(bxdfMode)));

    compileArgs.emplace_back("-DFEATURE_FLAGS=" + std::to_string(featureFlags));
    for (int i = 0; i < FEATURE_COUNT; i++)
    {
        const hlsl::uint flagValue = 1u << i;
        compileArgs.emplace_back("-DFEATURE_FLAG_VALUE_" + std::string(s_featureFlagNames[i]) + "=" + std::to_string(flagValue));
    }
    for (int i = 0; i < DEBUG_COUNT; i++)
    {
        const hlsl::uint flagValue = 1u << i;
        compileArgs.emplace_back("-DDEBUG_FLAG_VALUE_" + std::string(s_debugFlagNames[i]) + "=" + std::to_string(flagValue));
    }

#if CHERRY_DEBUG_FEATURES_ENABLED
    compileArgs.emplace_back("-DDEBUG_FLAGS=" + std::to_string(debugInfo.Flags));
#endif

#ifdef _DEBUG
    Profiler::AddToStack("Path-Tracer Update Pipeline");
#endif

    auto desc = CreateComputePipelineDesc(m_rootSig.Get());
    m_pipeline.InitCompute(device, "PathTracing/0_PathTracerCS.hlsl", desc, compileArgs);

#ifdef _DEBUG
    Profiler::PopAndPrint();
#endif

    Reset();
}

void PathTracer::Reset()
{
    m_frameIdx = 0;

#if CHERRY_DEBUG_FEATURES_ENABLED
    if (m_scheduledRunState == ScheduledRunState::eDisplay)
        m_scheduledRunState = ScheduledRunState::eIdle;
#endif
}

#if CHERRY_DEBUG_FEATURES_ENABLED
void PathTracer::RenderGUI_DebugInfo(PathTracerConfig& config)
{
    if (GetPathTracerDebugFlag(config.DebugInfo.Flags, eDebug_PathDumper))
    {
        if (ImGui::CollapsingHeader("Path Dump"))
        {
            ImGui::Indent(IM_GUI_INDENTATION);

            ImGui::Text("Pixel Coord:     (%i, %i)", m_dumpedPathPixelCoords.x, m_dumpedPathPixelCoords.y);
            ImGui::Text("Frame Index:     %i", m_dumpedPathFrameIdx);
            ImGui::Text("Camera Position: (%f, %f, %f)", m_dumpedPathCameraPosition.x, m_dumpedPathCameraPosition.y, m_dumpedPathCameraPosition.z);

            if (ImGui::Button("Re-Run Path"))
            {
                m_scheduledRunState = ScheduledRunState::eRunFrame;
                m_isPathDumpAutomatic = false;

                m_scheduledRunPixelCoords = m_dumpedPathPixelCoords;
                m_scheduledRunFrameIdx = m_dumpedPathFrameIdx;
                m_scheduledRunCameraPosition = m_dumpedPathCameraPosition;
                m_scheduledRunViewMatrix = m_dumpedPathViewMatrix;
            }

            if (!m_isPathDumpAutomatic && ImGui::Button("Enable Automatic Path Dump"))
                m_isPathDumpAutomatic = true;

            uint32_t rowIncrementer = 0;
            for (int i = 0; i < static_cast<uint32_t>(PATH_DUMP_MAX_RAY_DEPTH); i++)
            {
                if (!m_cpuPathDump[i].Explored)
                    break;

                const std::string label = std::string("Ray ") + std::to_string(i);

                if (ImGui::TreeNode(label.c_str()))
                {
                    ImGui::Indent(IM_GUI_INDENTATION/2);

                    char buf[64];

                    if (ImGui::BeginTable((label + "##table").c_str(), 3,
                                          ImGuiTableFlags_Borders |
                                          ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_SizingFixedFit))
                    {
                        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 170.0f);
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 18.0f);
                        ImGui::TableHeadersRow();

                        auto RowInt = [&](const char* key, const int value)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(key);

                            ImGui::TableSetColumnIndex(1);
                            snprintf(buf, sizeof(buf), "%d##int-%i", value, rowIncrementer);
                            ImGui::Selectable(buf);
                            if (ImGui::IsItemClicked())
                                ImGui::SetClipboardText(std::string(buf).substr(0, std::string(buf).find('#')).c_str());

                            rowIncrementer++;
                        };

                        auto RowFloat = [&](const char* key, const float value)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(key);

                            ImGui::TableSetColumnIndex(1);
                            snprintf(buf, sizeof(buf), "%.6f##float-%i", value, rowIncrementer);
                            ImGui::Selectable(buf);
                            if (ImGui::IsItemClicked())
                                ImGui::SetClipboardText(std::string(buf).substr(0, std::string(buf).find('#')).c_str());

                            ImGui::TableSetColumnIndex(2);
                            const ImVec4 color(std::clamp(value, 0.0f, 1.0f), std::clamp(value, 0.0f, 1.0f), std::clamp(value, 0.0f, 1.0f), 1.0f);
                            ImGui::ColorButton((std::string("##color1-") + std::to_string(rowIncrementer)).c_str(), color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(18, 18));
                            rowIncrementer++;
                        };

                        auto RowFloat3 = [&](const char* key, const hlsl::float3& v)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(key);
                            ImGui::TableSetColumnIndex(1);

                            ImGui::TableSetColumnIndex(1);
                            snprintf(buf, sizeof(buf), "(%.3f, %.3f, %.3f)##float3-%i", v.x, v.y, v.z, rowIncrementer);
                            ImGui::Selectable(buf);
                            if (ImGui::IsItemClicked())
                                ImGui::SetClipboardText(std::string(buf).substr(0, std::string(buf).find('#')).c_str());

                            ImGui::TableSetColumnIndex(2);
                            const ImVec4 color(std::clamp(v.x, 0.0f, 1.0f), std::clamp(v.y, 0.0f, 1.0f), std::clamp(v.z, 0.0f, 1.0f), 1.0f);
                            ImGui::ColorButton((std::string("##color3-") + std::to_string(rowIncrementer)).c_str(), color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(18, 18));
                            rowIncrementer++;
                        };

                        RowInt("Ray Segment", static_cast<int>(m_cpuPathDump[i].PathState.RaySegmentIdx));
                        RowInt("Dirac Delta", m_cpuPathDump[i].PathState.LastRayDiracDelta);
                        RowFloat("Last PDF", m_cpuPathDump[i].PathState.LastBxdfPdf);

                        for (int j = 1; j < static_cast<int>(DebugOutputIndex::eCount); ++j) // Starting from 1 due to ignored eDebugOutput_Disabled
                        {
                            if (m_cpuPathDump[i].DebugOutputs.Float3List[j].x == m_cpuPathDump[i].DebugOutputs.Float3List[j].y && m_cpuPathDump[i].DebugOutputs.Float3List[j].x == m_cpuPathDump[i].DebugOutputs.Float3List[j].z)
                                RowFloat(s_debugOutputIdxNames[j], m_cpuPathDump[i].DebugOutputs.Float3List[j].x);
                            else
                                RowFloat3(s_debugOutputIdxNames[j], m_cpuPathDump[i].DebugOutputs.Float3List[j]);
                        }

                        ImGui::EndTable();
                    }

                    ImGui::Unindent(IM_GUI_INDENTATION/2);
                    ImGui::TreePop();
                }
            }

            ImGui::Unindent(IM_GUI_INDENTATION);
        }
    }

    if (!GetPathTracerDebugFlag(config.DebugInfo.Flags, eDebug_Asserts))
        return;

    std::vector<uint32_t> errors;
    for (int i = 0; i < _countof(m_cpuErrorInfo); i++)
    {
        if (m_cpuErrorInfo[i].ExprCounter > 0 || m_cpuErrorInfo[i].NaNCounter > 0 || m_cpuErrorInfo[i].InfCounter > 0)
        {
            errors.emplace_back(i);
        }
    }

    if (errors.empty())
        return;

    const std::string assertLabel = std::string("Assertion Errors (") + std::to_string(errors.size()) + ")";
    if (ImGui::CollapsingHeader(assertLabel.c_str()))
    {
        ImGui::Indent(IM_GUI_INDENTATION);
        for (int i = 0; i < errors.size(); i++)
        {
            if (i > 0)
                ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(224 / 255., 82 / 255., 110 / 255., 255));

            const uint32_t dbgId = errors[i];
            ImGui::Text("%s: ", s_debugIdList[dbgId]);
            ImGui::SetItemTooltip("%s: ", s_debugIdList[dbgId]);
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                ImGui::Text("PixelCoord: (%i, %i)", m_cpuErrorInfo[dbgId].PixelCoord.x, m_cpuErrorInfo[dbgId].PixelCoord.y);
                ImGui::Text("FrameIndex: %i", m_cpuErrorInfo[dbgId].FrameIndex);

                if (m_cpuErrorInfo[dbgId].ExprCounter > 0)
                {
                    ImGui::Text("EXPR=%u", m_cpuErrorInfo[dbgId].ExprCounter);
                    ImGui::SetItemTooltip("EXPR=%u", m_cpuErrorInfo[dbgId].ExprCounter);

                    ImGui::Indent(IM_GUI_INDENTATION);
                    auto printValue = [&](const char* name, const XMFLOAT4& value)
                    {
                        if (value.x == value.y && value.x == value.z && value.x == value.w)
                            ImGui::Text("%s=(%f)", name, value.x);
                        else
                            ImGui::Text("%s=(%f, %f, %f, %f)", name, value.x, value.y, value.z, value.w);
                    };

                    printValue("V1", m_cpuErrorInfo[dbgId].Value1);
                    printValue("V2", m_cpuErrorInfo[dbgId].Value2);
                    printValue("V3", m_cpuErrorInfo[dbgId].Value3);
                    ImGui::Unindent(IM_GUI_INDENTATION);
                }

                if (m_cpuErrorInfo[dbgId].NaNCounter > 0)
                {
                    ImGui::Text("NAN=%u", m_cpuErrorInfo[dbgId].NaNCounter);
                    ImGui::SetItemTooltip(" NAN=%u", m_cpuErrorInfo[dbgId].NaNCounter);
                }

                if (m_cpuErrorInfo[dbgId].InfCounter > 0)
                {
                    ImGui::Text("INF=%u", m_cpuErrorInfo[dbgId].InfCounter);
                    ImGui::SetItemTooltip(" INF=%u", m_cpuErrorInfo[dbgId].InfCounter);
                }

                ImGui::PopStyleColor();

                if (ImGui::Button((std::string("Dump Path##") + std::to_string(dbgId)).c_str()))
                {
                    m_scheduledRunState = ScheduledRunState::eRunFrame;
                    m_isPathDumpAutomatic = false;

                    m_scheduledRunPixelCoords = m_cpuErrorInfo[dbgId].PixelCoord;
                    m_scheduledRunFrameIdx = m_cpuErrorInfo[dbgId].FrameIndex;
                    m_scheduledRunCameraPosition = m_cpuErrorInfo[dbgId].CameraPositionWorld;

                    m_scheduledRunViewMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_cpuErrorInfo[dbgId].InvV));

                    SetPathTracerDebugFlag(config.DebugInfo.Flags, eDebug_PathDumper, true);
                }
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
        }

        if (ImGui::Button("Clear Assertions"))
            m_scheduleClearErrors = true;

        ImGui::Unindent(IM_GUI_INDENTATION);
    }
    ImGui::Spacing();
}
#endif