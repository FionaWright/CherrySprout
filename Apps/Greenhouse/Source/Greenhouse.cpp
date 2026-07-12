#include "System/pch.h"
#include "Greenhouse.h"

#include "imgui.h"
#include "Scenes.h"
#include "HWI/D3D.h"
#include "Scene/SceneManager.h"
#include "System/FileHelper.h"
#include "System/HighResolutionClock.h"
#include "System/Input.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

#ifdef _DEBUG
#   include "Debug/Snapshotter.h"
#endif

void Greenhouse::Init(D3D* d3d)
{
    App::Init(d3d);

    CherryPrint("Initializing Greenhouse...");

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

    CherryPrint("Greenhouse Initialized");
}

void Greenhouse::Update(D3D* d3d, const TimeArgs timeArgs)
{
    bool loadSceneDataIntoRenderBackend = false;

    if (m_envMapDirty)
    {
        m_envMap.Init(d3d, &m_heap, "autumn_field_puresky_4k.hdr", 0); // TODO
    }

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
        d3d->Flush();

        m_sceneManager.UnreserveData();

        const SceneConfig& sceneConfig = s_sceneConfigs.at(m_currentSceneIdx);
        m_sceneManager.LoadScene(sceneConfig.Filepath.c_str(), sceneConfig.SceneScale);

        m_cameraController.GetCamera().SetPosition(sceneConfig.CameraPosition);
        m_cameraController.GetCamera().SetRotation(sceneConfig.CameraPitchYaw);
        m_renderInfo.V = m_cameraController.GetViewMatrix();
        m_renderInfo.InvV = XMMatrixInverse(nullptr, m_renderInfo.V);

        m_renderInfo.Scene = &m_sceneManager.GetScene();

        m_sceneDirty = false;
    }

    if (m_sceneManager.IsGpuDataDirty())
    {
        m_sceneManager.UploadScene(d3d);
        m_sceneManager.AddSceneTexturesToHeap(d3d, &m_heap);
    }

    if (!m_lightImportanceSampler.IsInitialized() && pathTracingFeatureEnabled(eFeature_NEE))
    {
        m_lightImportanceSampler.Build(d3d, &m_heap, m_envMap.GetEA(), &m_sceneManager.GetScene());
        loadSceneDataIntoRenderBackend = true;
    }

    if (!m_currRenderBackend->IsSceneDataLoaded(m_sceneManager.GetScene().Filepath) || loadSceneDataIntoRenderBackend)
    {
        m_currRenderBackend->LoadSceneData(d3d, &m_sceneManager.GetScene(), &m_heap, &m_uploadHeapCBV, &m_envMap, &m_lightImportanceSampler);
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

    if (m_ptFrameDirty)
    {
        m_pathTracer.Reset();
        m_ptFrameDirty = false;
    }

    if (m_ptPipelineDirty)
    {
        m_pathTracer.UpdatePipeline(d3d->GetDevice(), m_config.PathTracerConfig.FeatureFlags, m_config.PathTracerConfig.DebugInfo, m_config.PathTracerConfig.BxdfMode);
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
                DispatchOverTexture(cmdList, THREAD_COUNTS, accum->GetDesc().Width, accum->GetDesc().Height);
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

void Greenhouse::OnResize(uint32_t width, uint32_t height)
{
    m_renderBackendDirty = true;
    m_ptFrameDirty = true;
}