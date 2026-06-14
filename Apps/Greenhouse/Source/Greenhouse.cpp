#include "System/pch.h"
#include "Greenhouse.h"

#include "imgui.h"
#include "Scenes.h"
#include "HWI/D3D.h"
#include "Scene/SceneManager.h"
#include "System/Gui.h"
#include "System/GuiUtils.h"
#include "System/HighResolutionClock.h"
#include "Utils/Constants.h"
#include "Utils/ConstantsCpp.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

#ifdef _DEBUG
#   include "Debug/Snapshotter.h"
#endif

void Greenhouse::Init(D3D* d3d)
{
    App::Init(d3d);

    CherryPrint("Initializing Greenhouse...");

    m_currentSceneIdx = 0;
    Config::SetUIntFromArg(&m_currentSceneIdx, "--scene");
    Config::SetUIntFromArg(reinterpret_cast<uint32_t*>(&m_config.RenderBackend), "--backend");

    size_t maxCbvRequiredSize = 0;
    {
        CherryPrint("Total PT      CBV Size: " << m_pathTracer.TotalCbvRequiredSize());
        CherryPrint("Total Forward CBV Size: " << m_forward.TotalCbvRequiredSize());
        maxCbvRequiredSize += std::max(m_pathTracer.TotalCbvRequiredSize(), m_forward.TotalCbvRequiredSize());
        maxCbvRequiredSize += EnvironmentMap::GetCbvRequiredSize();
    }

    constexpr size_t numDescriptors = 20000; // TODO: Handle this properly
    m_heap.Init("Main Heap", d3d->GetDevice(), numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_uploadHeapCBV.Init(d3d->GetDevice(), maxCbvRequiredSize + 256); // TODO: Test without extra

    m_cameraController.Init(XMFLOAT3(0, 0, 5), 0, PI);

    m_aspectRatio = static_cast<float>(Config::GetSystem().RtvWidth) / static_cast<float>(Config::GetSystem().RtvHeight);
    m_renderInfo.P = XMMatrixPerspectiveFovLH(XMConvertToRadians(Config::GetRender().FoV), m_aspectRatio, Config::GetRender().NearPlane, Config::GetRender().FarPlane);
    m_renderInfo.InvP = XMMatrixInverse(nullptr, m_renderInfo.P);
    m_renderInfo.V = m_cameraController.GetViewMatrix();
    m_renderInfo.InvV = XMMatrixInverse(nullptr, m_renderInfo.V);
    m_renderInfo.Heap = &m_heap;
    m_renderInfo.Camera = &m_cameraController.GetCamera();
    m_renderInfo.BackendConfig = &m_config.RenderBackendConfig;
    m_renderInfo.PathTracerConfig = &m_config.PathTracerConfig;
    m_renderInfo.EnvironmentMap = &m_envMap;

    if (!d3d->GetRayTracingSupported())
    {
        if (m_config.RenderBackend == RenderBackendMode::ePathTracer)
            m_config.RenderBackend = RenderBackendMode::eForward;
    }

    m_currRenderBackend = m_config.RenderBackend == RenderBackendMode::eForward ? static_cast<IRenderBackend*>(&m_forward) : static_cast<IRenderBackend*>(&m_pathTracer);
    m_currRenderBackend->Init(d3d, &m_heap, &m_uploadHeapCBV);

    CherryPrint("Greenhouse Initialized");
}

void Greenhouse::Update(D3D* d3d, const TimeArgs timeArgs)
{
    if (m_renderBackendDirty)
    {
        m_uploadHeapCBV.FlushData();
        m_sceneManager.UnreserveData();
        m_currRenderBackend->UnreserveData();

        m_currRenderBackend = m_config.RenderBackend == RenderBackendMode::eForward ? static_cast<IRenderBackend*>(&m_forward) : static_cast<IRenderBackend*>(&m_pathTracer);

        if (!m_currRenderBackend->IsInitialized())
        {
            m_currRenderBackend->Init(d3d, &m_heap, &m_uploadHeapCBV);
        }

        m_renderBackendDirty = false;
    }

    if (m_sceneDirty)
    {
        m_sceneManager.UnreserveData();

        const SceneConfig& sceneConfig = s_sceneConfigs.at(m_currentSceneIdx);
        m_sceneManager.LoadScene(sceneConfig.Filepath.c_str(), sceneConfig.SceneScale);
        m_cameraController.GetCamera().SetPosition(sceneConfig.CameraPosition);
        m_cameraController.GetCamera().SetRotation(sceneConfig.CameraPitchYaw);

        m_renderInfo.Scene = &m_sceneManager.GetScene();

        m_sceneDirty = false;
    }

    // Upload Scene
    if (m_sceneManager.IsGpuDataDirty())
    {
        m_sceneManager.UploadScene(d3d);
        m_sceneManager.AddSceneTexturesToHeap(d3d, &m_heap);
    }

    if (m_envMapDirty)
    {
        m_envMap.Init(d3d, &m_heap, "autumn_field_puresky_4k.hdr", 0);
    }

    if (!m_currRenderBackend->IsSceneDataLoaded(m_sceneManager.GetScene().Filepath))
    {
        m_currRenderBackend->LoadSceneData(d3d, &m_sceneManager.GetScene(), &m_heap, &m_uploadHeapCBV, &m_envMap);
    }

    if (m_cameraController.UpdateCamera(timeArgs.ElapsedTime_ms / 1000.0f))
    {
        m_renderInfo.V = m_cameraController.GetViewMatrix();
        m_renderInfo.InvV = XMMatrixInverse(nullptr, m_renderInfo.V);
        m_ptFrameDirty = true;
    }

    if (m_ptFrameDirty)
    {
        m_pathTracer.Reset();
        m_ptFrameDirty = false;
    }

    if (m_ptPipelineDirty)
    {
        m_pathTracer.UpdatePipeline(d3d->GetDevice(),
            m_config.PathTracerConfig.FeatureFlags,
            m_config.PathTracerConfig.DebugFlags,
            m_config.PathTracerConfig.DebugOutputIdx,
            m_config.PathTracerConfig.BxdfMode);
        m_ptPipelineDirty = false;
    }

    m_currRenderBackend->Update(d3d, timeArgs);
}

void Greenhouse::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    m_renderInfo.EnvMapDirty = m_envMapDirty;

    m_currRenderBackend->Render(d3d, cmdList, m_renderInfo);
}

void Greenhouse::PostUpdate(D3D* d3d)
{
    m_currRenderBackend->PostUpdate(d3d, m_renderInfo);

#ifdef _DEBUG
    if (!m_scheduledSnapshotPT.empty())
    {
        D12Resource* accum = m_pathTracer.GetTexAccum();

        D12Resource texGammaCorrected;
        {
            D3D12_STATIC_SAMPLER_DESC sampler;
            InitializeSamplerLinearClamp(&sampler);

            RootSig rootSig;
            rootSig.SmartInit(d3d->GetDevice(), 1, 1, 1, false, &sampler, 1);

            texGammaCorrected.Init_Tex2D("Accum Gamma Corrected", d3d->GetDevice(), accum->GetDesc().Width, accum->GetDesc().Height, 1, accum->GetDesc().Format, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

            Pipeline pipeline;
            pipeline.InitCompute(d3d->GetDevice(), "Compute/GammaCorrectCS.hlsl", rootSig.Get());

            UploadHeap uploadHeap;
            uploadHeap.Init(d3d->GetDevice(), Align(sizeof(CbvGammaCorrect), 256));

            DescriptorSet set;
            set.Init(&m_heap);
            set.AddCBV(d3d->GetDevice(), sizeof(CbvGammaCorrect), &uploadHeap);
            set.SetSRV_Tex2D(d3d->GetDevice(), 0, accum, accum->GetDesc().Format);
            set.SetUAV_Tex2D(d3d->GetDevice(), 0, &texGammaCorrected, texGammaCorrected.GetDesc().Format);

            CbvGammaCorrect cbv;
            cbv.Dimensions = hlsl::uint2(accum->GetDesc().Width, accum->GetDesc().Height);
            cbv.IsToSrgb = true;
            set.UpdateCBV(0, &cbv);

            d3d->Flush();
            const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
            const auto cmdList = cmdListPtr.Get();
            {
                accum->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

                m_heap.Bind(cmdList);
                cmdList->SetComputeRootSignature(rootSig.Get());
                cmdList->SetPipelineState(pipeline.GetPSO());
                set.TransitionAllSRVToShaderResource(cmdList);
                set.SetDescriptorTables_Compute(cmdList);

                constexpr uint32_t THREAD_COUNTS = 16;
                const uint32_t groupX = (accum->GetDesc().Width + (THREAD_COUNTS-1)) / THREAD_COUNTS;
                const uint32_t groupY = (accum->GetDesc().Height + (THREAD_COUNTS-1)) / THREAD_COUNTS;
                cmdList->Dispatch(groupX, groupY, 1);
            }
            V(cmdList->Close());
            d3d->ExecuteCommandList(cmdList);
            d3d->Flush();
        }

        uint8_t* data = nullptr;
        size_t dataSize = 0;
        Snapshotter::ResourceToSnapshot(d3d, &texGammaCorrected, data, dataSize);

        ScratchImage scratch;
        const Image* packed = Snapshotter::PackData(d3d, data, &texGammaCorrected, scratch);

        Snapshotter::SnapshotToFile(packed, m_scheduledSnapshotPT.c_str());

        ScratchImage rgba8;
        Snapshotter::SnapshotToRgba8(packed, rgba8);
        Snapshotter::Rgba8SnapshotToClipboard(rgba8.GetImage(0,0,0));

        m_scheduledSnapshotPT = "";
    }
#endif

    m_envMapDirty = false;
}

bool Greenhouse::pathTracingFeatureEnabled(const PathTracerFeatureFlags flag) const
{
    if (m_config.RenderBackend != RenderBackendMode::ePathTracer)
        return false;

    return GetPathTracerFeatureFlag(m_config.PathTracerConfig.FeatureFlags, flag);
}

void Greenhouse::RenderGUI()
{
    Gui::BeginWindow("Greenhouse", ImVec2(0, 0),
                     ImVec2(Config::GetSystem().WindowAppGuiWidth, Config::GetSystem().RtvHeight));

    static bool s_hideGUI = false;
    ImGui::Checkbox("Hide GUI", &s_hideGUI);
    if (s_hideGUI)
    {
        Gui::EndWindow();
        return;
    }

    ImGui::SeparatorText("Scene");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        const char* curName = s_sceneConfigs.at(m_currentSceneIdx).Name.c_str();
        if (GuiUtils::BeginComboWithTooltip("Scene", curName))
        {
            for (size_t i = 0; i < s_sceneConfigs.size(); i++)
            {
                const bool isSelected = m_currentSceneIdx == i;
                if (ImGui::Selectable(s_sceneConfigs.at(i).Name.c_str(), isSelected))
                {
                    m_currentSceneIdx = i;
                    m_sceneDirty = true;
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        m_sceneDirty |= ImGui::Button("Reload Scene");
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Render Backend");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        static int e = static_cast<int>(m_config.RenderBackend);
        int c = 0;
        m_renderBackendDirty |= ImGui::RadioButton("Path Tracer", &e, c++);
        m_renderBackendDirty |= ImGui::RadioButton("Forward", &e, c++);
        m_config.RenderBackend = static_cast<RenderBackendMode>(e);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Info");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        ImGui::Text("Frames : %i", m_pathTracer.GetCurrentFrameIdx());
        ImGui::Text("Samples: %i", m_pathTracer.GetCurrentFrameIdx() * m_config.PathTracerConfig.SPP);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Settings##PT");
    ImGui::Indent(IM_GUI_INDENTATION);
    {

        m_ptFrameDirty |= GuiUtils::FwInputUInt("SPP", &m_config.PathTracerConfig.SPP);
        m_ptFrameDirty |= GuiUtils::FwInputUInt("Max Ray Depth", &m_config.PathTracerConfig.MaxRayDepth);
        m_ptFrameDirty |= GuiUtils::FwInputUInt("Max Frames", &m_config.PathTracerConfig.MaxFrameNumber);
        m_ptFrameDirty |= GuiUtils::FwInputUInt("RR Min Bounces", &m_config.PathTracerConfig.RussianRouletteMinBounces);
        m_ptFrameDirty |= GuiUtils::FwInputFloat("Firefly Threshold", &m_config.PathTracerConfig.FireFlyThreshold);

        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

        // BxDF Mode
        {
            ImGui::Text("BxDF:");
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                static int e = static_cast<int>(m_config.PathTracerConfig.BxdfMode);
                int c = 0;
                for (int i = 0; i < static_cast<int>(BxdfMode::eCount); i++)
                {
                    m_ptPipelineDirty |= ImGui::RadioButton(s_bxdfNames[i], &e, c++);
                }
                m_config.PathTracerConfig.BxdfMode = static_cast<BxdfMode>(e);
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
        }

        ImGui::Text("Feature Flags:");
        ImGui::Indent(IM_GUI_INDENTATION);
        if (ImGui::BeginTable("Feature Flags", 2))
        {
            for (int i = 0; i < FEATURE_COUNT; i++)
            {
                ImGui::TableNextColumn();

                const auto flag = static_cast<PathTracerFeatureFlags>(1 << i);
                bool isEnabled = GetPathTracerFeatureFlag(m_config.PathTracerConfig.FeatureFlags, flag);
                m_ptPipelineDirty |= ImGui::Checkbox(s_featureFlagNames[i], &isEnabled);
                SetPathTracerFeatureFlag(m_config.PathTracerConfig.FeatureFlags, flag, isEnabled);

                ImGui::SetItemTooltip("%s", s_featureFlagNames[i]);
            }

            ImGui::EndTable();
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
        ImGui::Spacing();

        if (pathTracingFeatureEnabled(eFeature_DirectionalLight))
        {
            ImGui::Text("Directional Light:");
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                m_ptFrameDirty |= GuiUtils::FwInputFloat3("Dir Light Dir", (float*)&m_config.RenderBackendConfig.DirLightDirection);
                m_ptFrameDirty |= GuiUtils::FwColorEdit3("Dir Light Color", (float*)&m_config.RenderBackendConfig.DirLightColor);
                m_ptFrameDirty |= GuiUtils::FwInputFloat("Dir Light Intensity", &m_config.RenderBackendConfig.DirLightIntensity);
                m_ptFrameDirty |= GuiUtils::FwInputFloat("Dir Light Radius", &m_config.RenderBackendConfig.DirLightCosAngularRadius);
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
            ImGui::Spacing();
        }

        if (pathTracingFeatureEnabled(eFeature_DepthOfField))
        {
            ImGui::Text("Depth of Field:");
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                m_ptFrameDirty |= GuiUtils::FwDragFloat("Focal Distance", &m_config.PathTracerConfig.DofFocalDist, 0.05f);
                m_ptFrameDirty |= GuiUtils::FwInputFloat("Lens Radius", &m_config.PathTracerConfig.DofLensRadius);
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
            ImGui::Spacing();
        }

#ifdef _DEBUG
        ImGui::Text("Debug Flags:");
        ImGui::Indent(IM_GUI_INDENTATION);
        if (ImGui::BeginTable("Debug Flags", 2))
        {
            for (int i = 0; i < DEBUG_COUNT; i++)
            {
                ImGui::TableNextColumn();

                const auto flag = static_cast<PathTracerDebugFlags>(1 << i);
                bool isEnabled = GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, flag);
                m_ptPipelineDirty |= ImGui::Checkbox(s_debugFlagNames[i], &isEnabled);
                SetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, flag, isEnabled);

                ImGui::SetItemTooltip("%s", s_debugFlagNames[i]);
            }
            ImGui::EndTable();
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
        ImGui::Spacing();

        if (GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, eDebug_Asserts))
        {
            static_cast<PathTracer*>(m_currRenderBackend)->RenderGUI_ErrorInfo();
        }

        if (!GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, eDebug_OutputColor))
            m_config.PathTracerConfig.DebugOutputIdx = DebugOutputIndex::eDebugOutput_Disabled;

        if (GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, eDebug_OutputColor))
        {
            ImGui::Text("Debug Outputs:");
            ImGui::Indent(IM_GUI_INDENTATION);
            if (ImGui::BeginTable("Debug Outputs", 2))
            {
                static int e = static_cast<int>(m_config.PathTracerConfig.DebugOutputIdx);
                int c = 1;
                for (int i = 1; i < static_cast<int>(DebugOutputIndex::eCount); i++)
                {
                    ImGui::TableNextColumn();
                    m_ptPipelineDirty |= ImGui::RadioButton(s_debugOutputIdxNames[i], &e, c++);
                    ImGui::SetItemTooltip("%s", s_debugOutputIdxNames[i]);
                }
                m_config.PathTracerConfig.DebugOutputIdx = static_cast<DebugOutputIndex>(e);
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
            ImGui::EndTable();
        }
#endif

        ImGui::PopStyleVar();
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Tools:");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
#ifdef _DEBUG
        static char buff[256];
        ImGui::InputText("Snapshot Path", buff, 256);

        if (ImGui::Button("Take Snapshot (PT)"))
        {
            if (buff[0] == '\0')
                m_scheduledSnapshotPT = std::string(SOURCE_DIR) + "/Snapshots/Default_PT";
            else
                m_scheduledSnapshotPT = std::string(SOURCE_DIR) + "/Snapshots/" + buff;
        }
#endif
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    m_ptFrameDirty |= m_sceneDirty | m_renderBackendDirty | m_ptPipelineDirty;

    Gui::EndWindow();
}
