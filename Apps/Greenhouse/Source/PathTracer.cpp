//
// Created by fionaw on 01/06/2026.
//

#include "System/pch.h"
#include "PathTracer.h"

#include "Greenhouse.h"
#include "imgui.h"
#include "Debug/GPUEventScoped.h"
#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "Utils/CBVs.h"
#include "Scene/InstanceData.h"
#include "System/HighResolutionClock.h"
#include "Utils/ConstantsCpp.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"
#include "Utils/Debug/DebugID.h"
#include "Utils/Debug/DebugStructs.h"

#if !NDEBUG
#include "Debug/Profiler.h"
#endif

void PathTracer::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    IRenderBackend::Init(d3d, heap, uploadHeapCBV);

    CherryPrint("Initializing Path-Tracer...");

    // TODO: Is primal and accum doing the exact same job now? Can be combined maybe
    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        //desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // TODO: Only necessary for GDPT?
        desc.Width = Config::GetSystem().RtvWidth;
        desc.Height = Config::GetSystem().RtvHeight;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        m_primal.Release();
        m_primal.Init("Primal", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
    }

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
        m_outputRgba8Unorm.Release();
        m_outputRgba8Unorm.Init("Output", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
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
        m_accum.Release();
        m_accum.Init("Accum", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
    }

#if CHERRY_DEBUG_FEATURES_ENABLED
    m_debugManager.Init(d3d, heap);
#endif

    {
        m_rootSigBlit.SmartInit(d3d->GetDevice(), 0, 1, 1);
        m_setBlit.Init(heap);
        m_setBlit.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_outputRgba8Unorm, m_outputRgba8Unorm.GetDesc().Format);
    }

    PathTracingDebugInfo debugInfo = PathTracingDebugInfo();
    Config::SetBoolFromArg(&debugInfo.CbvFlagsModeEnabled, "--cbvFlagMode");
    const auto featureFlags = debugInfo.CbvFlagsModeEnabled ? s_defaultFeatureFlagsCbvMode : s_defaultFeatureFlags;
    debugInfo.Flags = debugInfo.CbvFlagsModeEnabled ? s_defaultDebugFlagsCbvMode : s_defaultDebugFlags;
    UpdatePipeline(d3d->GetDevice(),
        featureFlags,
        debugInfo,
        s_defaultBxdfMode,
        s_defaultMMType);

    m_descriptorSet.Init         (heap, true);
    m_descriptorSet.AddCBV       (d3d->GetDevice(), sizeof(CbvPathTracingSettings), uploadHeapCBV);
    m_descriptorSet.AddCBV       (d3d->GetDevice(), sizeof(CbvPathTracingDebugSettings), uploadHeapCBV);

    m_gradientManager.Init(d3d);

    CherryPrint("Path-Tracer Initialized");
}

void PathTracer::LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap, LightImportanceSampler* lightImportanceSampler, GBufferPrePass* gbuffer)
{
    IRenderBackend::LoadSceneData(d3d, scene, heap, uploadHeapCBV, envMap, lightImportanceSampler, gbuffer);

    {
        CherryPrint("PT: Building RTAS");

        // Note: Direct queue is required as you can't transition from D3D12_RESOURCE_STATE_INDEX_BUFFER in a compute queue. Buffer may be left in that state from previous rasterization passes
        d3d->Flush();
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();
        {
            ComPtr<ID3D12Device5> device5;
            V(d3d->GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));
            ComPtr<ID3D12GraphicsCommandList4> cmdList4;
            V(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

            m_rtasBuilder.Build(device5.Get(), cmdList4.Get(), scene);
        }
        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();

        CherryPrint("PT: RTAS Built");
    }

    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 0, &m_accum, m_accum.GetDesc().Format);
    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 1, &m_primal, m_primal.GetDesc().Format);
    m_descriptorSet.SetUAV_Buffer(d3d->GetDevice(), 2, m_restirManager.GetReservoirBuffer(), m_restirManager.GetNumReservoirs(), sizeof(ReservoirDI));
    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 3, m_gradientManager.GetGradientXF(), m_gradientManager.GetGradientXF()->GetDesc().Format);
    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 4, m_gradientManager.GetGradientXB(), m_gradientManager.GetGradientXB()->GetDesc().Format);
    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 5, m_gradientManager.GetGradientYF(), m_gradientManager.GetGradientYF()->GetDesc().Format);
    m_descriptorSet.SetUAV_Tex2D (d3d->GetDevice(), 6, m_gradientManager.GetGradientYB(), m_gradientManager.GetGradientYB()->GetDesc().Format);

#if CHERRY_DEBUG_FEATURES_ENABLED
    m_descriptorSet.SetUAV_Buffer(d3d->GetDevice(), 7, m_debugManager.GetBufferGPUErrorInfoRW(), _countof(s_debugIdList), sizeof(DebugErrorInfo));
    m_descriptorSet.SetUAV_Buffer(d3d->GetDevice(), 8, m_debugManager.GetBufferPathDumpRW(), PATH_DUMP_MAX_RAY_DEPTH, sizeof(RayDump));
    m_descriptorSet.SetUAV_ByteAddressBuffer(d3d->GetDevice(), 9, m_debugManager.GetBufferPathDumpOCRW(), PATH_DUMP_MAX_RAY_DEPTH * sizeof(DebugOutputStruct));
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
        m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 7, lightImportanceSampler->GetEnvMapPdf(), lightImportanceSampler->GetEnvMapPdf()->GetDesc().Format);
        m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 8, lightImportanceSampler->GetEnvMapCdfConditional(), lightImportanceSampler->GetEnvMapCdfConditional()->GetDesc().Format);
        m_descriptorSet.SetSRV_Tex1D(d3d->GetDevice(), 9, lightImportanceSampler->GetEnvMapCdfMarginal(), lightImportanceSampler->GetEnvMapCdfMarginal()->GetDesc().Format);
        m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 10, lightImportanceSampler->GetLightsCdf(), lightCount, sizeof(ProbabilityDistributionSample));
    }

    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 11, gbuffer->GetGBufferMaterialIdx(), gbuffer->GetGBufferMaterialIdx()->GetDesc().Format);
    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 12, gbuffer->GetGBufferNormals(), gbuffer->GetGBufferNormals()->GetDesc().Format);
    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 13, gbuffer->GetGBufferDepth(), GBUFFER_FORMAT_DEPTH_SRV);
    m_descriptorSet.SetSRV_Tex2D(d3d->GetDevice(), 14, gbuffer->GetGBufferUvMv(), gbuffer->GetGBufferUvMv()->GetDesc().Format);

    m_gradientManager.LoadSceneData(d3d, heap, &m_primal);
}

void PathTracer::Update(D3D* d3d, Heap* heap, TimeArgs timeArgs)
{
    m_gradientManager.Update(d3d, heap);
}

void PathTracer::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo)
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    m_debugManager.PostUpdate(d3d, renderInfo, m_frameIdx, &m_outputRgba8Unorm);
#endif
}

void PathTracer::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* RTV, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle)
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
        if (renderInfo.PathTracerConfig->DebugInfo.Flags != 0)
        {
            const hlsl::uint2 chosenPixelCoords = m_debugManager.IsScheduledRunState(ScheduledRunState::eRunFrame) ? m_debugManager.GetScheduledRunPixelCoords() : renderInfo.PathTracerConfig->DebugInfo.ChosenPixelCoords;

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
            debugSettings.ForcedLightIndex = renderInfo.PathTracerConfig->DebugInfo.ForcedLightIndex;
            debugSettings.CbvFeatureFlags = renderInfo.PathTracerConfig->DebugInfo.CbvFeatureFlags;
            debugSettings.CbvDebugFlags = renderInfo.PathTracerConfig->DebugInfo.CbvDebugFlags;
            m_descriptorSet.UpdateCBV(1, &debugSettings);
        }

        if (m_debugManager.IsScheduledRunState(ScheduledRunState::eRecompilePipelineAndRunFrame))
        {
            d3d->Flush();
            UpdatePipeline(d3d->GetDevice(),
                renderInfo.PathTracerConfig->FeatureFlags,
                renderInfo.PathTracerConfig->DebugInfo,
                renderInfo.PathTracerConfig->BxdfMode,
                renderInfo.PathTracerConfig->MicrofacetModelType);

            m_debugManager.ScheduledRunTransition(ScheduledRunState::eRunFrame);
        }

        if (m_debugManager.IsScheduledRunState(ScheduledRunState::eRunFrame))
        {
            m_debugManager.SetScheduledRunParameters(&settings);
            m_debugManager.ScheduledRunTransition(ScheduledRunState::eReadbackPathDump);
        }
#endif

        if (renderInfo.PathTracerConfig->TrueRandomSeedMode)
            renderInfo.PathTracerConfig->Seed = std::rand();

        settings.Seed = renderInfo.PathTracerConfig->Seed;
        settings.MaxRayDepth = renderInfo.PathTracerConfig->MaxRayDepth;
        settings.MaxShadowRayDepth = renderInfo.PathTracerConfig->MaxShadowRayDepth;
        settings.RussianRouletteMinBounces = renderInfo.PathTracerConfig->RussianRouletteMinBounces;
        settings.SPP = renderInfo.PathTracerConfig->SPP;
        settings.FireflyThreshold = renderInfo.PathTracerConfig->FireFlyThreshold;
        settings.IsMaxFramesReached = renderInfo.PathTracerConfig->MaxFrameNumber != 0 && m_frameIdx > renderInfo.PathTracerConfig->MaxFrameNumber;
        settings.DirectNumSamples = renderInfo.PathTracerConfig->DirectNumSamples;

        float speedOfLight = renderInfo.PathTracerConfig->TransientSpeedOfLight;
        settings.TransientLightIdx = renderInfo.PathTracerConfig->TransientLightIndex;
        settings.TransientDistanceSinceStart = (renderInfo.PathTracerConfig->TransientTimeSinceStart) * speedOfLight;
        settings.TransientPulseDistance = (renderInfo.PathTracerConfig->TransientPulseDuration) * speedOfLight;

        settings.DirLightDirection = renderInfo.BackendConfig->DirLightDirection;
        settings.DirLightColor = renderInfo.BackendConfig->DirLightColor;
        settings.DirLightCosAngularRadius = 1.0f - renderInfo.BackendConfig->DirLightCosAngularRadius;
        settings.DirLightIntensity = renderInfo.BackendConfig->DirLightIntensity;

        settings.DofFocalDist = renderInfo.PathTracerConfig->DofFocalDist;
        settings.DofLensRadius = renderInfo.PathTracerConfig->DofLensRadius;

        settings.RestirConfidenceCap = renderInfo.PathTracerConfig->RestirConfidenceCap;
        settings.RestirNumCandidates = renderInfo.PathTracerConfig->RestirNumCandidates;

        settings.FrameDimensions = { Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight };
        settings.TexelSize = XMFLOAT2(1.0f / (float)settings.FrameDimensions.x, 1.0f / (float)settings.FrameDimensions.y);
        m_descriptorSet.UpdateCBV(0, &settings);
    }

    {
        m_primal.Transition                             (cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_accum.Transition                              (cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        scene->GPU.MegaBufferVertex.Transition          (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        scene->GPU.MegaBufferIndex.Transition           (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        scene->GPU.MegaBufferInstanceData.Transition    (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        scene->GPU.MegaBufferMaterials.Transition       (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        renderInfo.EnvironmentMap->GetEA()->Transition  (cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    }

    if (renderInfo.PathTracerConfig->FeatureEnabled(eFeature_RestirDI))
    {
        cmdList->SetComputeRootSignature(m_rootSig.Get());
        heap->Bind(cmdList);
        heap->BindSceneTextures_Compute(cmdList, m_rootSig.GetParamIndexSceneTextures());
        m_descriptorSet.SetDescriptorTables_Compute(cmdList);

        m_restirManager.GenerateSamplesDi(d3d->GetDevice(), cmdList);
    }

    // Main Pass
#if CHERRY_DEBUG_FEATURES_ENABLED
    if (!m_debugManager.IsScheduledRunState(ScheduledRunState::eDisplay))
#endif
    {
        cmdList->SetComputeRootSignature(m_rootSig.Get());
        cmdList->SetPipelineState(m_pipeline.GetPSO());
        heap->Bind(cmdList);
        heap->BindSceneTextures_Compute(cmdList, m_rootSig.GetParamIndexSceneTextures());
        m_descriptorSet.SetDescriptorTables_Compute(cmdList);

        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_frameIdx++;
    }

    m_finalHdrOutput = &m_primal;

    if (renderInfo.PathTracerConfig->FeatureEnabled(eFeature_GradientDomain))
        m_finalHdrOutput = m_gradientManager.Render(d3d, cmdList, renderInfo, &m_primal);

    // Copy to RTV
    {
        GPU_SCOPE(cmdList, "Copy Output to RTV");

        {
            GPU_SCOPE(cmdList, "Blit to Rgba8Unorm");

            m_finalHdrOutput->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            m_outputRgba8Unorm.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            cmdList->SetPipelineState(m_pipelineBlit.GetPSO());
            cmdList->SetComputeRootSignature(m_rootSigBlit.Get());

            if (m_setBlit.GetSRV(0) != m_finalHdrOutput)
            {
                d3d->Flush();
                m_setBlit.SetSRV_Tex2D(d3d->GetDevice(), 0, m_finalHdrOutput, m_finalHdrOutput->GetDesc().Format);
            }

            m_setBlit.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);
        }

        {
            GPU_SCOPE(cmdList, "Copy to RTV");

            m_outputRgba8Unorm.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
            RTV->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

            RTV->CopyTextureInto(cmdList, m_outputRgba8Unorm.GetResource());
        }
    }
}

void PathTracer::UnreserveData()
{
    m_primal.Release();
    m_accum.Release();
    m_outputRgba8Unorm.Release();
#if CHERRY_DEBUG_FEATURES_ENABLED
    m_debugManager.UnreserveData();
#endif
    m_isInitialized = false;
    m_currentlyLoadedScene = "";
}

void PathTracer::UpdatePipeline(ID3D12Device* device,
    const PathTracerFeatureFlags& featureFlags,
    const PathTracingDebugInfo& debugInfo,
    const BxdfMode& bxdfMode,
    const MicrofacetModelType& microfacetModelType)
{
    constexpr uint32_t numCBV = 2;
    constexpr uint32_t numSRV = 15;
    constexpr uint32_t numUAV = 10;

    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
    InitializeSamplerLinearClamp(&samplers[0], 0);
    InitializeSamplerLinearWrap(&samplers[1], 1);
    m_rootSig.SmartInit(device, numCBV, numSRV, numUAV, true, samplers, _countof(samplers));

    std::vector<std::string> compileArgs = {};
    compileArgs.emplace_back("-DBXDF_MODE=" + std::to_string(static_cast<uint32_t>(bxdfMode)));
    compileArgs.emplace_back("-DMICROFACET_MODEL_TYPE=" + std::to_string(static_cast<uint32_t>(microfacetModelType)));

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

    if (debugInfo.CbvFlagsModeEnabled)
        compileArgs.emplace_back("-DDEBUG_CBV_FLAGS_MODE_ENABLED=1");

#if !NDEBUG
    Profiler::AddToStack("Path-Tracer Update Pipeline");
#endif

    auto desc = CreateComputePipelineDesc(m_rootSig.Get());
    m_pipeline.InitCompute(device, "PathTracing/0_PathTracerCS.hlsl", desc, compileArgs);

    m_pipelineBlit.InitCompute(device, "Compute/TexBlitGCCS.hlsl", m_rootSigBlit.Get(), compileArgs);

#if !NDEBUG
    Profiler::PopAndPrint();
#endif

    Reset();

    if (GetPathTracerFeatureFlag(featureFlags, eFeature_RestirDI))
        m_restirManager.Init(device, &m_rootSig, compileArgs);
}

void PathTracer::Reset()
{
    m_frameIdx = 0;

#if CHERRY_DEBUG_FEATURES_ENABLED
    if (m_debugManager.IsScheduledRunState(ScheduledRunState::eDisplay))
        m_debugManager.ScheduledRunTransition(ScheduledRunState::eIdle);
#endif
}