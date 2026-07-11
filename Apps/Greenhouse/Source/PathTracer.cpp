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

void PathTracer::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    IRenderBackend::Init(d3d, heap, uploadHeapCBV);

    CherryPrint("Initializing Path-Tracer...");

    m_gbufferPrePass.Init(d3d, heap, uploadHeapCBV);

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
    m_descriptorSet.SetUAV_Buffer(d3d->GetDevice(), 3, &m_pathDumpBufferRW, PATH_DUMP_MAX_RAY_DEPTH, sizeof(RayDump));
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

    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 9, m_gbufferPrePass.GetGBufferMaterialIdx(), m_gbufferPrePass.GetGBufferMaterialIdx()->GetDesc().Format);
    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 10, m_gbufferPrePass.GetGBufferNormals(), m_gbufferPrePass.GetGBufferNormals()->GetDesc().Format);
    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 11, m_gbufferPrePass.GetGBufferDepth(), GBUFFER_FORMAT_DEPTH_SRV);
    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 12, m_gbufferPrePass.GetGBufferUvMv(), m_gbufferPrePass.GetGBufferUvMv()->GetDesc().Format);
}

void PathTracer::Update(D3D* d3d, TimeArgs timeArgs)
{

}

void PathTracer::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo)
{
#ifdef _DEBUG
    if (GetPathTracerDebugFlag(renderInfo.PathTracerConfig->DebugInfo.Flags, eDebug_Asserts))
    {
        d3d->Flush();

        if (m_scheduleClearErrors)
        {
            constexpr size_t bufferSize = _countof(s_debugIdList) * sizeof(DebugErrorInfo);
            m_gpuErrorInfoRW.Release();
            m_gpuErrorInfoRW.Init_Buffer("Error Info R/W", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
            m_scheduleClearErrors = false;
        }

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

void PathTracer::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo)
{
    GPU_SCOPE(cmdList, "Path-Trace");

    Scene* scene = renderInfo.Scene;
    const Heap* heap = renderInfo.Heap;

    XMMATRIX V = renderInfo.V;
#ifdef _DEBUG
    if (m_scheduledRunState == ScheduledRunState::eRunFrame)
        V = m_scheduledRunViewMatrix;
#endif

    if (GBufferRequired(renderInfo.PathTracerConfig->FeatureFlags, renderInfo.PathTracerConfig->DebugInfo.Flags))
    {
        m_gbufferPrePass.Render(d3d, cmdList, scene, renderInfo.Heap, V, renderInfo.P);

        m_gbufferPrePass.GetGBufferMaterialIdx()->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_gbufferPrePass.GetGBufferNormals()->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_gbufferPrePass.GetGBufferDepth()->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_gbufferPrePass.GetGBufferUvMv()->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    }

    // Fill Settings
    {
        CbvPathTracingSettings settings{};
        XMStoreFloat4x4(&settings.InvP, renderInfo.InvP);
        XMStoreFloat4x4(&settings.InvV, renderInfo.InvV);
        settings.FrameIdx = m_frameIdx;
        settings.CameraPositionWorld = renderInfo.Camera->GetPosition();

#ifdef _DEBUG
        if (m_scheduledRunState == ScheduledRunState::eRunFrame)
        {
            settings.FrameIdx = m_scheduledRunFrameIdx;
            settings.CameraPositionWorld = m_scheduledRunCameraPosition;
            XMStoreFloat4x4(&settings.InvV, XMMatrixInverse(nullptr, m_scheduledRunViewMatrix));

            // TODO: Make chosen pixel coord a CBV value to avoid recompilation
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

#ifdef _DEBUG
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
    }

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

bool PathTracer::GBufferRequired(const PathTracerFeatureFlags& featureFlags, const PathTracerDebugFlags& debugFlags) const
{
    return GetPathTracerDebugFlag(debugFlags, eDebug_OutputColor) || GetPathTracerDebugFlag(debugFlags, eDebug_PathDumper);
}

void PathTracer::UpdatePipeline(ID3D12Device* device, const PathTracerFeatureFlags& featureFlags, const PathTracingDebugInfo& debugInfo, const BxdfMode& bxdfMode)
{
    uint32_t numSRV = 6;
    uint32_t numUAV = 2;
#ifdef _DEBUG
    const bool outputColorEnabled = GetPathTracerDebugFlag(debugInfo.Flags, eDebug_OutputColor);
    const bool assertsEnabled = GetPathTracerDebugFlag(debugInfo.Flags, eDebug_Asserts);
    const bool pathDumpEnabled = GetPathTracerDebugFlag(debugInfo.Flags, eDebug_PathDumper);
    if (assertsEnabled)
        numUAV++; // gDbgBufferErrorInfo
    if (pathDumpEnabled)
        numUAV++; // gPathDump
#endif

    if (GetPathTracerFeatureFlag(featureFlags, eFeature_NEE))
        numSRV += 3; // gEnvMapCdfConditional, gEnvMapPmfConditional, gEnvMapCdfMarginal

    if (GBufferRequired(featureFlags, debugInfo.Flags))
        numSRV += 4;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);
    m_rootSig.SmartInit(device, 1, numSRV, numUAV, true, &sampler, 1);

    std::vector<std::string> compileArgs = {};
    compileArgs.emplace_back("-DFEATURE_FLAGS=" + std::to_string(featureFlags));
    compileArgs.emplace_back("-DDEBUG_FLAGS=" + std::to_string(debugInfo.Flags));
    compileArgs.emplace_back("-DBXDF_MODE=" + std::to_string(static_cast<uint32_t>(bxdfMode)));

    for (int i = 0; i < FEATURE_COUNT; i++)
    {
        const hlsl::uint flagValue = 1u << i;
        compileArgs.emplace_back("-DFEATURE_FLAG_VALUE_" + std::string(s_featureFlagNames[i]) + "=" + std::to_string(flagValue));
    }

#ifdef _DEBUG
    for (int i = 0; i < DEBUG_COUNT; i++)
    {
        const hlsl::uint flagValue = 1u << i;
        compileArgs.emplace_back("-DDEBUG_FLAG_VALUE_" + std::string(s_debugFlagNames[i]) + "=" + std::to_string(flagValue));
    }

    if (outputColorEnabled || pathDumpEnabled)
    {
        const hlsl::uint2 chosenPixelCoords = m_scheduledRunState == ScheduledRunState::eRunFrame ? m_scheduledRunPixelCoords : debugInfo.ChosenPixelCoords;

        compileArgs.emplace_back("-DDEBUG_OUTPUT_COLOR=" + std::to_string(static_cast<uint32_t>(debugInfo.OutputColorIdx)));
        compileArgs.emplace_back("-DDEBUG_CHOSEN_RAY_DEPTH=" + std::to_string(debugInfo.ChosenRayDepth));
        compileArgs.emplace_back("-DDEBUG_CHOSEN_PIXEL_COORDS=uint2(" + std::to_string(chosenPixelCoords.x) + "," + std::to_string(chosenPixelCoords.y) + ")");
    }
#endif

    auto desc = CreateComputePipelineDesc(m_rootSig.Get());
    m_pipeline.InitCompute(device, "PathTracing/0_PathTracerCS.hlsl", desc, compileArgs);

    Reset();
}

void PathTracer::Reset()
{
    m_frameIdx = 0;

#ifdef _DEBUG
    if (m_scheduledRunState == ScheduledRunState::eDisplay)
        m_scheduledRunState = ScheduledRunState::eIdle;
#endif
}

#ifdef _DEBUG
void PathTracer::RenderGUI_DebugInfo(const PathTracerConfig& config)
{
    if (GetPathTracerDebugFlag(config.DebugInfo.Flags, eDebug_PathDumper))
    {
        if (ImGui::CollapsingHeader("Path Dump"))
        {
            ImGui::Indent(IM_GUI_INDENTATION);

            if (!m_isPathDumpAutomatic && ImGui::Button("Enable Automatic Path Dump"))
                m_isPathDumpAutomatic = true;

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

            const size_t maxRayDepth = std::min(config.MaxRayDepth, static_cast<uint32_t>(PATH_DUMP_MAX_RAY_DEPTH));
            for (int i = 0; i < maxRayDepth; i++)
            {
                const std::string label = std::string("Ray ") + std::to_string(i);

                if (ImGui::TreeNode(label.c_str()))
                {
                    ImGui::Indent(IM_GUI_INDENTATION/2);

                    if (ImGui::BeginTable((label + "##table").c_str(), 2,
                                          ImGuiTableFlags_Borders |
                                          ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_SizingFixedFit))
                    {
                        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 220.0f);
                        ImGui::TableHeadersRow();

                        auto RowInt = [](const char* key, const int value)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(key);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", value);
                        };

                        auto RowFloat = [](const char* key, const float value)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(key);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%.6f", value);
                        };

                        auto RowFloat3 = [](const char* key, const hlsl::float3& v)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(key);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
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
                ImGui::Text("NAN=%u",m_cpuErrorInfo[dbgId].NaNCounter);
                ImGui::SetItemTooltip(" NAN=%u",m_cpuErrorInfo[dbgId].NaNCounter);
            }

            if (m_cpuErrorInfo[dbgId].InfCounter > 0)
            {
                ImGui::Text("INF=%u", m_cpuErrorInfo[dbgId].InfCounter);
                ImGui::SetItemTooltip(" INF=%u", m_cpuErrorInfo[dbgId].InfCounter);
            }
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);
    ImGui::Spacing();
    ImGui::PopStyleColor();

    if (ImGui::Button("Clear Assertions"))
        m_scheduleClearErrors = true;
}
#endif