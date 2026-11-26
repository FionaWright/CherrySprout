//
// Created by fiona on 01/06/2026.
//

#include "System/pch.h"
#include "Deferred.h"

#include "Greenhouse.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/Heap.h"
#include "Raster/Utils/ShadowMapPuncInfo.h"
#include "Scene/Scene.h"
#include "System/FileHelper.h"
#include "System/HighResolutionClock.h"
#include "System/TextureLoader.h"
#include "Utils/D3DUtils.h"
#include "Utils/Random.h"

void Deferred::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    IRenderBackend::Init(d3d, heap, uploadHeapCBV);

    m_texPrimal.Init_Tex2D("Tex Primal", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_texPing.Init_Tex2D("Tex Ping", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_texPong.Init_Tex2D("Tex Pong", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_texOutputRgba8Unorm.Init_Tex2D("Tex Output", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    m_gbufferPrePass.Init(d3d, heap, uploadHeapCBV);

    std::vector<std::string> compileArgs;
    if (Config::GetRender().DepthBufferType == DepthBufferType::eReversed)
        compileArgs.emplace_back("-DUSING_REVERSED_Z");

    // Lighting Pass
    {
        D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
        InitializeSamplerLinearWrap(&samplers[0], 0);
        InitializeSamplerPointClamp(&samplers[1], 1);

        m_rootSigLighting.AddCbvRange(0, 2);
        m_rootSigLighting.AddSrvRange(0, 10);
        m_rootSigLighting.AddUavRange(0, 1);
        m_rootSigLighting.AddSrvRangeBindless(1);
        m_rootSigLighting.AddSrvRangeBindless(2);
        m_rootSigLighting.AddSrvRangeBindless(3);
        m_rootSigLighting.Init(d3d->GetDevice(), samplers, _countof(samplers));

        m_setLighting.Init(heap);
        m_setLighting.AddBindlessSrv(1);
        m_setLighting.AddCBV(d3d->GetDevice(), 0, sizeof(CbvForward), uploadHeapCBV);
        m_setLighting.AddCBV(d3d->GetDevice(), 0, sizeof(CbvShadowMapSample), uploadHeapCBV);

        V(m_pipelineLighting.InitCompute(d3d->GetDevice(), "Raster/DeferredLightingCS.hlsl", m_rootSigLighting.Get(), compileArgs));
    }

    // Bloom
    {
        D3D12_STATIC_SAMPLER_DESC samplers[1] = {};
        InitializeSamplerLinearWrap(&samplers[0], 0);

        m_rootConstantsBloom.Init(0, 0, sizeof(CbvFilterBloom));
        m_rootSigBloom.AddRootConstants(&m_rootConstantsBloom);
        m_rootSigBloom.AddSrvRange(0, 1);
        m_rootSigBloom.AddUavRange(0, 1);
        m_rootSigBloom.Init(d3d->GetDevice(), samplers, _countof(samplers));

        m_setBloomH.Init(heap);
        m_setBloomH.SetHasRootConstants();
        m_setBloomV.Init(heap);
        m_setBloomV.SetHasRootConstants();
        m_setBloomExtract.Init(heap);
        m_setBloomExtract.SetHasRootConstants();

        uint32_t width = Config::GetSystem().RtvWidth;
        uint32_t height = Config::GetSystem().RtvHeight;
        for (uint32_t i = 0; i < MAX_BLOOM_DEPTH; i++)
        {
            m_texBloom[i].Init_Tex2D("Tex Bloom", d3d->GetDevice(), width, height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            m_setBloomDown[i].Init(heap);
            m_setBloomDown[i].SetHasRootConstants();
            m_setBloomUp[i].Init(heap);
            m_setBloomUp[i].SetHasRootConstants();

            width /= 2;
            height /= 2;
        }

        V(m_pipelineBloomExtract.InitCompute(d3d->GetDevice(), "Filters/BloomExtractCS.hlsl", m_rootSigBloom.Get(), compileArgs));
        V(m_pipelineBloomDownsample.InitCompute(d3d->GetDevice(), "Filters/BloomDownsampleCS.hlsl", m_rootSigBloom.Get(), compileArgs));
        V(m_pipelineBloomUpsample.InitCompute(d3d->GetDevice(), "Filters/BloomUpsampleCS.hlsl", m_rootSigBloom.Get(), compileArgs));
        V(m_pipelineBloomH.InitCompute(d3d->GetDevice(), "Filters/BloomHCS.hlsl", m_rootSigBloom.Get(), compileArgs));
        V(m_pipelineBloomV.InitCompute(d3d->GetDevice(), "Filters/BloomVCS.hlsl", m_rootSigBloom.Get(), compileArgs));
    }

    // Post-Processing
    {
        m_rootSigPostProcess.AddCbvRange(0, 1);
        m_rootSigPostProcess.AddSrvRange(0, 2);
        m_rootSigPostProcess.AddUavRange(0, 1);
        m_rootSigPostProcess.Init(d3d->GetDevice());

        m_setPostProcess.Init(heap);
        m_setPostProcess.AddCBV(d3d->GetDevice(), 0, sizeof(CbvPostProcessing), uploadHeapCBV);
        m_setPostProcess.SetUAV_Tex2D(d3d->GetDevice(), 0, 0, &m_texOutputRgba8Unorm, m_texOutputRgba8Unorm.GetDesc().Format);

        V(m_pipelinePostProcess.InitCompute(d3d->GetDevice(), "Raster/PostProcessCS.hlsl", m_rootSigPostProcess.Get(), compileArgs));
    }

    UploadHeap uploadHeap;

    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();
    {
        const std::wstring brdfIntPath = FileHelper::GetAssetTextureFullPath(L"BrdfIntMap.dds");

        ScratchImage scratch;
        m_brdfIntMap = TextureLoader::LoadTexture2D(d3d->GetDevice(), brdfIntPath.c_str(), scratch, D3D12_RESOURCE_FLAG_NONE, false);
        uploadHeap.Init(d3d->GetDevice(), m_brdfIntMap.GetIntermediateSize());

        TextureLoader::UploadTexture(cmdList, &uploadHeap, scratch, &m_brdfIntMap);
    }
    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();

    m_shadowMapManager.Init(d3d->GetDevice(), heap, uploadHeapCBV);
    m_shadowMapManager.GeneratePoissonSamples(16);

    m_skybox.Init(d3d, CUBEMAP_RESO, CUBEMAP_RESO);
}

void Deferred::LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap, LightImportanceSampler* lightImportanceSampler, GBufferPrePass* gbuffer, const bool isAnimationEnabled)
{
    IRenderBackend::LoadSceneData(d3d, scene, heap, uploadHeapCBV, envMap, lightImportanceSampler, gbuffer, isAnimationEnabled);

    if (!envMap->GetCubemap()->IsInitialized() || m_currentPanoFilepath != envMap->GetCurrentPanoFilepath())
    {
        envMap->InitCubemap(d3d, heap);
        m_skybox.UpdateDescriptorSet(d3d->GetDevice(), envMap->GetCubemap(), heap, uploadHeapCBV);

        m_currentPanoFilepath = envMap->GetCurrentPanoFilepath();
    }

    m_shadowMapManager.LoadSceneData(d3d, heap, scene);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Format = envMap->GetCubemap()->GetDesc().Format;
    srvDesc.TextureCube.MipLevels = envMap->GetCubemap()->GetDesc().MipLevels;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    m_setLighting.SetSRV_Buffer (d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));
    m_setLighting.SetSRV_Buffer (d3d->GetDevice(), 0, 1, &scene->GPU.MegaBufferPunctualLights, scene->CPU.MegaBufferPunctualLightsCount, sizeof(PunctualLight));
    m_setLighting.SetSRV_Buffer (d3d->GetDevice(), 0, 2, m_shadowMapManager.GetShadowMapPuncInfo(), scene->CPU.MegaBufferPunctualLightsCount, sizeof(ShadowMapPuncInfo));
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 3, &m_brdfIntMap, m_brdfIntMap.GetDesc().Format);
    m_setLighting.SetSRV        (d3d->GetDevice(), 0, 4, envMap->GetCubemap(), srvDesc);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 5, m_shadowMapManager.GetShadowMapDir(), SHADOW_MAP_FORMAT_DEPTH_SRV);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 6, m_gbufferPrePass.GetGBufferMaterialIdx(), GBUFFER_FORMAT_MAT_IDX);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 7, m_gbufferPrePass.GetGBufferNormalsUv(), GBUFFER_FORMAT_NORMALS_UV);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 8, m_gbufferPrePass.GetGBufferDepth(), GBUFFER_FORMAT_DEPTH_SRV);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 9, m_gbufferPrePass.GetGBufferMv(), GBUFFER_FORMAT_MV);
    m_setLighting.SetUAV_Tex2D  (d3d->GetDevice(), 0, 0, &m_texPrimal, m_texPrimal.GetDesc().Format);

    m_gbufferPrePass.LoadSceneData(d3d, scene);
}

void Deferred::Update(D3D* d3d, Heap* heap, TimeArgs timeArgs)
{
}

void Deferred::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo)
{

}

void Deferred::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* RTV, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    GPU_SCOPE(cmdList, "Deferred Backend");

    Scene* scene = renderInfo.Scene;
    Heap* heap = renderInfo.Heap;

    heap->Bind(cmdList);

    XMMATRIX P = renderInfo.P;

    D12Resource* vertexBuffer = renderInfo.IsAnimationEnabled ? &scene->GPU.MegaBufferVertexSkinned : &scene->GPU.MegaBufferVertexBind;

    D3D12_VERTEX_BUFFER_VIEW viewV;
    D3D12_INDEX_BUFFER_VIEW viewI;
    VertexIndexBuffersToViews(vertexBuffer, &scene->GPU.MegaBufferIndex,
                              scene->CPU.MegaBufferVertexCount, sizeof(Vertex), scene->CPU.MegaBufferIndexCount,
                              viewV, viewI);

    vertexBuffer->Transition(cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    scene->GPU.MegaBufferIndex.Transition(cmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    cmdList->IASetVertexBuffers(0, 1, &viewV);
    cmdList->IASetIndexBuffer(&viewI);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Jitter
    XMFLOAT2 frameJitter = XMFLOAT2(0,0);
    if (false) // TODO
    {
        RngInfo rngInfo = InitializeRngInfo(hlsl::uint2(0,0), 0, renderInfo.FrameIndex, renderInfo.PathTracerConfig->Seed);
        frameJitter = XMFLOAT2(Rand01(rngInfo) - 0.5f, Rand01(rngInfo) - 0.5f);

        P = ApplyProjectionJitter(P, frameJitter, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);
    }

    // Sort objects and frustum cull
    {

    }

    // Shadow Map
    {
        ShadowMapConfig config;
        config.Width = renderInfo.RasterConfig->SmWidth;
        config.Height = renderInfo.RasterConfig->SmHeight;
        config.CascadeCount = renderInfo.RasterConfig->SmCascadeCount;
        config.Near = renderInfo.RasterConfig->SmNear;
        config.Far = renderInfo.RasterConfig->SmFar;
        config.DirDist = renderInfo.RasterConfig->SmDirDist;
        config.DirLightDir = renderInfo.BackendConfig->DirLightDirection;
        config.IsAnimationEnabled = renderInfo.IsAnimationEnabled;
        m_shadowMapManager.Render(d3d, cmdList, heap, scene, config);
    }

    m_gbufferPrePass.Render(cmdList, scene, renderInfo);

    // SSAO Pass
    {

    }

    // ReSTIR Pass
    {

    }

    // Lighting Pass
    {
        {
            m_setLighting.TransitionAllSRVToShaderResource(cmdList);

            cmdList->SetComputeRootSignature(m_rootSigLighting.Get());
            cmdList->SetPipelineState(m_pipelineLighting.GetPSO());

            heap->BindSceneTextures_Compute(cmdList, m_rootSigLighting.GetParamIndexSrvBindless(0));
            if (m_shadowMapManager.GetHeapBasePoint() != UINT_MAX)
                heap->BindBindless_Compute(cmdList, m_shadowMapManager.GetHeapBasePoint(), m_rootSigLighting.GetParamIndexSrvBindless(1));
        }

        const XMFLOAT2 frameDimensions = XMFLOAT2(Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        CbvForward settings{};
        settings.CameraPosition = renderInfo.Camera->GetPosition();
        settings.FrameIdx = renderInfo.FrameIndex;
        settings.DirLightDir = renderInfo.BackendConfig->DirLightDirection;
        settings.MaxCubemapMipMaps = renderInfo.EnvironmentMap->GetCubemap()->GetDesc().MipLevels;
        settings.Flags = ForwardFlags::eNone;
        settings.TexelSize = XMFLOAT2(1.0f / frameDimensions.x, 1.0f / frameDimensions.y);
        settings.JitterPerFrame = frameJitter;
        settings.NearPlane = Config::GetRender().NearPlane;
        settings.FarPlane = Config::GetRender().FarPlane;
        XMStoreFloat4x4(&settings.InvV, XMMatrixInverse(nullptr, renderInfo.V));
        XMStoreFloat4x4(&settings.InvP, XMMatrixInverse(nullptr, P));
        ForwardFlagSet(settings.Flags, ForwardFlags::eEnvironmentMap, renderInfo.RasterConfig->FlagEnabledEnvironmentMap);
        ForwardFlagSet(settings.Flags, ForwardFlags::ePunctualLights, renderInfo.RasterConfig->FlagEnabledPunctualLights);
        ForwardFlagSet(settings.Flags, ForwardFlags::eDirectionalLight, renderInfo.RasterConfig->FlagEnabledDirectionalLight);
        ForwardFlagSet(settings.Flags, ForwardFlags::eShadowMaps, renderInfo.RasterConfig->FlagEnabledShadowMaps);
        ForwardFlagSet(settings.Flags, ForwardFlags::eAmbient, renderInfo.RasterConfig->FlagEnabledAmbient);
        m_setLighting.UpdateCBV(0, 0, &settings);

        CbvShadowMapSample cbvShadow{};
        XMStoreFloat4x4(&cbvShadow.ShadowMatrices[0], m_shadowMapManager.GetShadowMatrix(0));
        XMFLOAT2 dim = m_shadowMapManager.GetShadowDimensions(0);
        cbvShadow.TexelSize = XMFLOAT2(1.0f / dim.x, 1.0f / dim.y);
        cbvShadow.BiasDepthDistant = renderInfo.RasterConfig->SmBiasDepthDistant;
        cbvShadow.BiasDepthPoint = renderInfo.RasterConfig->SmBiasDepthPoint;
        cbvShadow.CascadeCount = renderInfo.RasterConfig->SmCascadeCount;
        cbvShadow.NormalBiasDistant = renderInfo.RasterConfig->SmBiasNormalDistant;
        cbvShadow.NormalBiasPoint = renderInfo.RasterConfig->SmBiasNormalPoint;
        cbvShadow.PcfSampleCount = renderInfo.RasterConfig->SmPcfCount;
        cbvShadow.PcfRadius = renderInfo.RasterConfig->SmPcfRadius;
        auto poissonDisc = m_shadowMapManager.GetPoissonSamples();
        for (uint32_t i = 0; i < POISSON_DISC_COUNT; i++)
        {
            cbvShadow.PoissonDisc[i].x = poissonDisc[i*2+0].x;
            cbvShadow.PoissonDisc[i].y = poissonDisc[i*2+0].y;
            cbvShadow.PoissonDisc[i].z = poissonDisc[i*2+1].x;
            cbvShadow.PoissonDisc[i].w = poissonDisc[i*2+1].y;
        }
        m_setLighting.UpdateCBV(0, 1, &cbvShadow);

        m_setLighting.SetDescriptorTables_Compute(cmdList);

        DispatchOverRegion(cmdList, 16, m_texPrimal.GetDesc().Width, m_texPrimal.GetDesc().Height);
    }

    D12Resource* finalHdrOutput = &m_texPrimal;

    // Bloom Pass
    if (renderInfo.RasterConfig->BloomRadius > 0 && renderInfo.RasterConfig->BloomRadius < 8)
    {
        cmdList->SetComputeRootSignature(m_rootSigBloom.Get());

        CbvFilterBloom cbv{};
        cbv.Radius = renderInfo.RasterConfig->BloomRadius;
        cbv.LuminanceThreshold = renderInfo.RasterConfig->BloomLumThreshold;

        // Extract
        {
            cbv.DestDimensions = hlsl::uint2(m_texBloom[0].GetDesc().Width, m_texBloom[0].GetDesc().Height);
            m_rootConstantsBloom.Bind_Compute(cmdList, &cbv);

            m_setBloomExtract.SetSRV_Tex2D(d3d->GetDevice(), 0, 0, &m_texPrimal, m_texPrimal.GetDesc().Format);
            m_setBloomExtract.SetUAV_Tex2D(d3d->GetDevice(), 0, 0, &m_texBloom[0], m_texBloom[0].GetDesc().Format);

            cmdList->SetPipelineState(m_pipelineBloomExtract.GetPSO());
            m_texBloom[0].Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_setBloomExtract.TransitionAllSRVToShaderResource(cmdList);
            m_setBloomExtract.SetDescriptorTables_Compute(cmdList);

            DispatchOverRegion(cmdList, 16, m_texBloom[0].GetDesc().Width, m_texBloom[0].GetDesc().Height);
        }

        for (int i = 0; i < renderInfo.RasterConfig->BloomDownsampleCount - 1; i++)
        {
            D12Resource& src = m_texBloom[i];
            D12Resource& dst = m_texBloom[i+1];

            cbv.DestDimensions = hlsl::uint2(dst.GetDesc().Width, dst.GetDesc().Height);
            m_rootConstantsBloom.Bind_Compute(cmdList, &cbv);

            m_setBloomDown[i].SetSRV_Tex2D(d3d->GetDevice(), 0, 0, &src, src.GetDesc().Format);
            m_setBloomDown[i].SetUAV_Tex2D(d3d->GetDevice(), 0, 0, &dst, dst.GetDesc().Format);

            cmdList->SetPipelineState(m_pipelineBloomDownsample.GetPSO());
            dst.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_setBloomDown[i].TransitionAllSRVToShaderResource(cmdList);
            m_setBloomDown[i].SetDescriptorTables_Compute(cmdList);

            DispatchOverRegion(cmdList, 16, dst.GetDesc().Width, dst.GetDesc().Height);
        }

        // Horizontal
        {
            D12Resource& src = m_texBloom[renderInfo.RasterConfig->BloomDownsampleCount - 1];
            D12Resource& dst = m_texPing;

            cmdList->SetPipelineState(m_pipelineBloomH.GetPSO());

            dst.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            m_setBloomH.SetSRV_Tex2D(d3d->GetDevice(), 0, 0, &src, src.GetDesc().Format);
            m_setBloomH.SetUAV_Tex2D(d3d->GetDevice(), 0, 0, &dst, dst.GetDesc().Format);

            m_setBloomH.TransitionAllSRVToShaderResource(cmdList);
            m_setBloomH.SetDescriptorTables_Compute(cmdList);

            DispatchOverRegion(cmdList, 16, dst.GetDesc().Width, dst.GetDesc().Height);
        }

        // Vertical
        {
            D12Resource& src = m_texPing;
            D12Resource& dst = m_texBloom[renderInfo.RasterConfig->BloomDownsampleCount - 1];

            cmdList->SetPipelineState(m_pipelineBloomV.GetPSO());

            dst.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            m_setBloomV.SetSRV_Tex2D(d3d->GetDevice(), 0, 0, &src, src.GetDesc().Format);
            m_setBloomV.SetUAV_Tex2D(d3d->GetDevice(), 0, 0, &dst, dst.GetDesc().Format);

            m_setBloomV.TransitionAllSRVToShaderResource(cmdList);
            m_setBloomV.SetDescriptorTables_Compute(cmdList);

            DispatchOverRegion(cmdList, 16, dst.GetDesc().Width, dst.GetDesc().Height);
        }

        for (int i = renderInfo.RasterConfig->BloomDownsampleCount - 1; i > 0; i--)
        {
            D12Resource& src = m_texBloom[i];
            D12Resource& dst = m_texBloom[i-1];

            cbv.DestDimensions = hlsl::uint2(dst.GetDesc().Width, dst.GetDesc().Height);
            m_rootConstantsBloom.Bind_Compute(cmdList, &cbv);

            m_setBloomUp[i].SetSRV_Tex2D(d3d->GetDevice(), 0, 0, &src, src.GetDesc().Format);
            m_setBloomUp[i].SetUAV_Tex2D(d3d->GetDevice(), 0, 0, &dst, dst.GetDesc().Format);

            cmdList->SetPipelineState(m_pipelineBloomUpsample.GetPSO());
            dst.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_setBloomUp[i].TransitionAllSRVToShaderResource(cmdList);
            m_setBloomUp[i].SetDescriptorTables_Compute(cmdList);

            DispatchOverRegion(cmdList, 16, dst.GetDesc().Width, dst.GetDesc().Height);
        }
    }

    // Post-Process
    {
        GPU_SCOPE(cmdList, "Post Process");

        m_texOutputRgba8Unorm.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        cmdList->SetPipelineState(m_pipelinePostProcess.GetPSO());
        cmdList->SetComputeRootSignature(m_rootSigPostProcess.Get());

        if (m_setPostProcess.GetSRV(0, 0) != finalHdrOutput)
        {
            d3d->Flush();
            m_setPostProcess.SetSRV_Tex2D(d3d->GetDevice(), 0, 0, finalHdrOutput, finalHdrOutput->GetDesc().Format);
            m_setPostProcess.SetSRV_Tex2D(d3d->GetDevice(), 0, 1, &m_texBloom[0], m_texBloom[0].GetDesc().Format);
        }

        CbvPostProcessing cbv{};
        cbv.ToneMapWhitePoint = renderInfo.BackendConfig->PostProcessWhitePoint;
        cbv.Exposure = renderInfo.BackendConfig->PostProcessExposure;
        m_setPostProcess.UpdateCBV(0, 0, &cbv);

        m_setPostProcess.TransitionAllSRVToShaderResource(cmdList);
        m_setPostProcess.SetDescriptorTables_Compute(cmdList);

        DispatchOverRegion(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);
    }

    // Copy to RTV
    {
        GPU_SCOPE(cmdList, "Copy Output to RTV");

        {
            GPU_SCOPE(cmdList, "Copy to RTV");

            m_texOutputRgba8Unorm.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
            RTV->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

            RTV->CopyTextureInto(cmdList, m_texOutputRgba8Unorm.GetResource(), Config::GetSystem().WindowAppGuiWidth);
        }
    }
}
