#include "System/pch.h"
#include "Greenhouse.h"

#include "imgui.h"
#include "Scenes.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "Scene/SceneManager.h"
#include "System/FileHelper.h"
#include "System/HighResolutionClock.h"
#include "System/Input.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

#if !NDEBUG
#   include "Debug/Snapshotter.h"
#endif

void Greenhouse::Init(D3D* d3d)
{
    App::Init(d3d);

    CherryPrint("Initializing Greenhouse...");

    m_frameBuffer.Init_Tex2D("Frame Buffer", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    m_heapRTV.Init("Greenhouse Heap RTV", d3d->GetDevice(), 1, 0, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_heapIdxFrameBuffer = CreateRTV(d3d->GetDevice(), &m_heapRTV, &m_frameBuffer);

    for (int i = s_sceneConfigs.size() - 1; i >= 0; i--)
    {
        const std::string fullpath = FileHelper::GetAssetFullPath(s_sceneConfigs[i].Filepath.c_str());
        if (!std::filesystem::exists(fullpath))
        {
            CherryPrint("[WARNING] Scene not found: " << fullpath);
            s_sceneConfigs.erase(s_sceneConfigs.begin() + i);
        }
    }

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

    constexpr size_t numDescriptors = 10000; // TODO: Handle this properly
    constexpr size_t numSceneTextureDescriptors = 5000;
    m_heap.Init("Main Heap", d3d->GetDevice(), numDescriptors, numSceneTextureDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_uploadHeapCBV.Init(d3d->GetDevice(), maxCbvRequiredSize + 256); // TODO: Test without extra

    Config::SetBoolFromArg(&m_config.PathTracerConfig.DebugInfo.CbvFlagsModeEnabled, "--cbvFlagMode");
    if (m_config.PathTracerConfig.DebugInfo.CbvFlagsModeEnabled)
    {
        m_config.PathTracerConfig.FeatureFlags = s_defaultFeatureFlagsCbvMode;
        m_config.PathTracerConfig.DebugInfo.CbvFeatureFlags = s_defaultFeatureFlags;

        m_config.PathTracerConfig.DebugInfo.Flags = s_defaultDebugFlagsCbvMode;
        m_config.PathTracerConfig.DebugInfo.CbvDebugFlags = s_defaultDebugFlags;
    }

    m_aspectRatio = static_cast<float>(Config::GetSystem().RtvWidth) / static_cast<float>(Config::GetSystem().RtvHeight);
    m_renderInfo.P = XMMatrixPerspectiveFovLH(XMConvertToRadians(Config::GetRender().FoV), m_aspectRatio, Config::GetRender().NearPlane, Config::GetRender().FarPlane);
    m_renderInfo.InvP = XMMatrixInverse(nullptr, m_renderInfo.P);
    m_renderInfo.Heap = &m_heap;
    m_renderInfo.Camera = &m_cameraController.GetCamera();
    m_renderInfo.BackendConfig = &m_config.RenderBackendConfig;
    m_renderInfo.PathTracerConfig = &m_config.PathTracerConfig;
    m_renderInfo.EnvironmentMap = &m_envMap;
    m_renderInfo.LightImportanceSampler = &m_lightImportanceSampler;

    if (!d3d->GetRayTracingSupported())
    {
        if (m_config.RenderBackend == RenderBackendMode::ePathTracer)
            m_config.RenderBackend = RenderBackendMode::eForward;
    }

    m_currRenderBackend = m_config.RenderBackend == RenderBackendMode::eForward ? static_cast<IRenderBackend*>(&m_forward) : static_cast<IRenderBackend*>(&m_pathTracer);
    m_currRenderBackend->Init(d3d, &m_heap, &m_uploadHeapCBV);

    m_gbufferPrePass.Init(d3d, &m_heap, &m_uploadHeapCBV);

    CherryPrint("Greenhouse Initialized");
}

void Greenhouse::Update(D3D* d3d, const TimeArgs timeArgs)
{
    if (m_envMapDirty)
    {
        m_envMap.Init(d3d, &m_heap, "autumn_field_puresky_4k.hdr", 0); // TODO
    }

    if (m_renderBackendDirty)
    {
        m_uploadHeapCBV.FreeAssignedData();
        m_currRenderBackend->UnreserveData();

        m_currRenderBackend = m_config.RenderBackend == RenderBackendMode::eForward ? static_cast<IRenderBackend*>(&m_forward) : static_cast<IRenderBackend*>(&m_pathTracer);

        if (!m_currRenderBackend->IsInitialized())
        {
            m_currRenderBackend->Init(d3d, &m_heap, &m_uploadHeapCBV);

            m_gbufferPrePass.UnreserveData();
            m_gbufferPrePass.Init(d3d, &m_heap, &m_uploadHeapCBV);
        }

        m_renderBackendDirty = false;
    }

    if (m_sceneDirty)
    {
        d3d->Flush();

        const SceneConfig& sceneConfig = s_sceneConfigs.at(m_currentSceneIdx);
        m_sceneManager.LoadScene(sceneConfig.Filepath.c_str(), sceneConfig.SceneScale);

        m_cameraController.GetCamera().SetPosition(sceneConfig.CameraPosition);
        m_cameraController.GetCamera().SetRotation(sceneConfig.CameraPitchYaw);
        m_renderInfo.V = m_cameraController.GetViewMatrix();
        m_renderInfo.InvV = XMMatrixInverse(nullptr, m_renderInfo.V);

        m_renderInfo.Scene = &m_sceneManager.GetScene();

#if CHERRY_DEBUG_FEATURES_ENABLED
        m_gizmosSceneLoaded = false;
#endif

        m_lsdDirty = true;
        m_ptFrameDirty = true;
        m_sceneDirty = false;
    }

#if CHERRY_DEBUG_FEATURES_ENABLED
    if (m_gizmosEnabled && !m_gizmosSceneLoaded)
    {
        m_gizmoManager.ClearGizmos();
        for (int i = 0; i < m_sceneManager.GetCPU().MegaBufferPunctualLightsCount; i++)
        {
            if (m_sceneManager.GetCPU().MegaBufferPunctualLights[i].Intensity == 0.0f)
                continue;

            const XMFLOAT3 position = m_sceneManager.GetCPU().MegaBufferPunctualLights[i].Position;
            const XMFLOAT3 color3 = m_sceneManager.GetCPU().MegaBufferPunctualLights[i].Color;
            const XMFLOAT4 color = XMFLOAT4(color3.x, color3.y, color3.z, 1.0f);
            m_gizmoManager.AddGizmo(d3d, &m_heap, position, color, "Textures/LightGizmo.dds");
        }
        m_gizmosSceneLoaded = true;
    }
#endif

    if (m_sceneManager.IsGpuDataDirty())
    {
        m_sceneManager.UploadScene(d3d);
        m_sceneManager.AddSceneTexturesToHeap(d3d, &m_heap);

        m_lightImportanceSampler.MarkSceneDataDirty();
        m_lsdDirty = true;
    }

    if (!m_lightImportanceSampler.IsInitialized() || (m_config.PathTracerConfig.FeatureEnabled(eFeature_NEE) && m_lsdDirty))
    {
        const bool envMapEnabled = m_config.PathTracerConfig.FeatureEnabled(eFeature_EnvironmentMap);
        const bool aliasEnabled = m_config.PathTracerConfig.FeatureEnabled(eFeature_AliasTables);
        m_lightImportanceSampler.Build(d3d, &m_heap, m_envMap.GetEA(), &m_sceneManager.GetScene(), envMapEnabled, aliasEnabled);
        m_renderBackendSceneDataDirty = true;
        m_lsdDirty = false;
    }

    if (!m_currRenderBackend->IsSceneDataLoaded(m_sceneManager.GetScene().Filepath) || m_renderBackendSceneDataDirty)
    {
        m_currRenderBackend->LoadSceneData(d3d, &m_sceneManager.GetScene(), &m_heap, &m_uploadHeapCBV, &m_envMap, &m_lightImportanceSampler, &m_gbufferPrePass);
        m_renderBackendSceneDataDirty = false;
    }

    if (m_cameraController.UpdateCamera(timeArgs.ElapsedTime_ms / 1000.0f))
    {
        m_renderInfo.V = m_cameraController.GetViewMatrix();
        m_renderInfo.InvV = XMMatrixInverse(nullptr, m_renderInfo.V);
        m_ptFrameDirty = true;
    }

#if CHERRY_DEBUG_FEATURES_ENABLED
    if (GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugInfo.Flags, eDebug_PathDumper) && Input::IsMouseLeftDown())
    {
        XMFLOAT2 mousePos = Input::GetMousePos();
        mousePos.x -= static_cast<float>(Config::GetSystem().WindowAppGuiWidth);

        if (mousePos.x > 0 &&
            mousePos.y > 0 &&
            mousePos.x < static_cast<float>(Config::GetSystem().RtvWidth) &&
            mousePos.y < static_cast<float>(Config::GetSystem().RtvHeight))
        {
            m_config.PathTracerConfig.DebugInfo.ChosenPixelCoords.x = static_cast<uint32_t>(mousePos.x);
            m_config.PathTracerConfig.DebugInfo.ChosenPixelCoords.y = static_cast<uint32_t>(mousePos.y);
            m_ptFrameDirty = true;
        }
    }
#endif

    const bool inTextField = ImGui::GetIO().WantCaptureKeyboard;
    if (m_ptFrameDirty || (!inTextField && Input::IsKeyDown(KeyCode::R)))
    {
        m_pathTracer.Reset();
        m_ptFrameDirty = false;
    }

    if (m_ptPipelineDirty)
    {
        d3d->Flush();
        m_pathTracer.UpdatePipeline(d3d->GetDevice(),
            m_config.PathTracerConfig.FeatureFlags,
            m_config.PathTracerConfig.DebugInfo,
            m_config.PathTracerConfig.BxdfMode,
            m_config.PathTracerConfig.MicrofacetModelType);
        m_ptPipelineDirty = false;
    }

    m_currRenderBackend->Update(d3d, &m_heap, timeArgs);
}

void Greenhouse::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    m_renderInfo.EnvMapDirty = m_envMapDirty;

    // GBuffer Pass
    {
        if (GBufferRequired())
        {
            m_gbufferPrePass.LoadSceneData(d3d, &m_sceneManager.GetScene());
            m_gbufferPrePass.Render(d3d, cmdList, &m_sceneManager.GetScene(), &m_heap, m_renderInfo.V, m_renderInfo.P);
        }

        m_gbufferPrePass.GetGBufferMaterialIdx()->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_gbufferPrePass.GetGBufferNormals()->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_gbufferPrePass.GetGBufferDepth()->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_gbufferPrePass.GetGBufferUvMv()->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    }

    const auto rtvHandle = m_heapRTV.GetDescriptorHandleAtIndex(m_heapIdxFrameBuffer);

    // Forward/Path-Tracing Pass
    m_currRenderBackend->Render(d3d, cmdList, m_renderInfo, &m_frameBuffer, rtvHandle);

#if CHERRY_DEBUG_FEATURES_ENABLED
    // Gizmos Pass
    if (m_gizmosEnabled)
    {
        m_frameBuffer.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_gbufferPrePass.GetGBufferDepth()->Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        const auto& dsvHandle = m_gbufferPrePass.GetDsvHandle();
        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        m_gizmoManager.Render(d3d, &m_heap, cmdList, m_renderInfo.V, m_renderInfo.P);
    }
#endif

    // Copy to swapchain
    {
        GPU_SCOPE(cmdList, "Copy final output to swapchain");

        m_frameBuffer.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        d3d->GetRtv()->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        d3d->GetRtv()->CopyTextureInto(cmdList, m_frameBuffer.GetResource(), Config::GetSystem().WindowAppGuiWidth, 0, 0);
    }
}

void Greenhouse::PostUpdate(D3D* d3d)
{
    m_currRenderBackend->PostUpdate(d3d, m_renderInfo);

#if !NDEBUG
    if (m_scheduledTransientRenderFrameIdx != -1)
    {
        const bool completedFrame = m_scheduledTransientRenderSampleIdx > m_config.PathTracerConfig.TransientRenderNumSamples;
        if (completedFrame)
        {
            m_scheduledSnapshotPT =
                std::format("{}/Snapshots/TransientRender/Frame_{:04}",
                    BUILD_DIR,
                    m_scheduledTransientRenderFrameIdx);
            m_isSnapshotPtLDR = true;

            m_ptFrameDirty = true;
            m_scheduledTransientRenderSampleIdx = 0;
            m_scheduledTransientRenderFrameIdx++;

            m_config.PathTracerConfig.TransientTimeSinceStart += m_config.PathTracerConfig.TransientRenderTotalTime / static_cast<float>(m_config.PathTracerConfig.TransientRenderNumFrames);
        }
        else
            m_scheduledTransientRenderSampleIdx++;

        const bool completedRender = m_scheduledTransientRenderFrameIdx >= m_config.PathTracerConfig.TransientRenderNumFrames;
        if (completedRender)
        {
            m_scheduledTransientRenderFrameIdx = -1;

            const std::string framePath = std::string(BUILD_DIR) + "/Snapshots/TransientRender/Frame_%04d.png";
            const std::string outputPath = std::string(BUILD_DIR) + "/Snapshots/TransientRender/Video.mp4";

            const std::string command =
                std::string("ffmpeg")
                + " -framerate 30"
                + " -i \"" + framePath + "\""
                + " -start_number 0"
                + " -y"
                + " -c:v libx264"
                + " -pix_fmt yuv420p"
                + " -vf \"pad=ceil(iw/2)*2:ceil(ih/2)*2\""
                + " \"" + outputPath + "\"";

            std::cout << command << std::endl;
            std::system(command.c_str());
        }
    }

    if (!m_scheduledSnapshotPT.empty() && m_pathTracer.GetHdrOutput())
    {
        D12Resource* ptOut = m_pathTracer.GetHdrOutput();

        d3d->Flush();

        uint8_t* data = nullptr;
        size_t dataSize = 0;
        Snapshotter::ResourceToSnapshot(d3d, ptOut, data, dataSize);

        ScratchImage scratch;
        const Image* packed = Snapshotter::PackData(d3d, data, ptOut, scratch);

        ScratchImage rgba8;
        Snapshotter::SnapshotToRgba8(packed, rgba8);
        Snapshotter::Rgba8SnapshotToClipboard(rgba8.GetImage(0,0,0));

        if (m_isSnapshotPtLDR)
            Snapshotter::SnapshotToFile(rgba8.GetImage(0,0,0), m_scheduledSnapshotPT.c_str(), true);
        else
            Snapshotter::SnapshotToFile(packed, m_scheduledSnapshotPT.c_str());

        m_scheduledSnapshotPT = "";
    }
#endif

    m_envMapDirty = false;
}

void Greenhouse::OnResize(uint32_t width, uint32_t height)
{
    m_renderBackendDirty = true;
    m_ptFrameDirty = true;
}

bool Greenhouse::GBufferRequired() const
{
    bool debugNeeds = false;
#if CHERRY_DEBUG_FEATURES_ENABLED
    const bool outputColor = m_config.PathTracerConfig.DebugEnabled(eDebug_OutputColor);
    const bool pathDumper = m_config.PathTracerConfig.DebugEnabled(eDebug_PathDumper);
    const bool debugFlagsNeeds = m_config.RenderBackend == RenderBackendMode::ePathTracer && (outputColor || pathDumper);
    debugNeeds = m_gizmosEnabled || debugFlagsNeeds;
#endif

    // PT only for now
    return m_config.RenderBackend == RenderBackendMode::ePathTracer && debugNeeds;
}
