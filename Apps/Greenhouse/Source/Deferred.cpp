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

#define NUM_RENDER_TARGETS 4

void Deferred::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    IRenderBackend::Init(d3d, heap, uploadHeapCBV);

    // GBuffer Resources
    {
        const size_t w = Config::GetSystem().RtvWidth;
        const size_t h = Config::GetSystem().RtvHeight;

        m_rtvClearValues = {
            XMFLOAT4(0, 0, 0, 0),
            XMFLOAT4(1, 1, 1, 1),
            XMFLOAT4(1, 1, 0, 0),
            XMFLOAT4(0, 0, 0, 0),
        };
        m_gbufferTexMaterialIdx.Init_Tex2D("GBuffer Material Idx", d3d->GetDevice(), w, h, 1, DEFERRED_GBUFFER_FORMAT_MAT_IDX, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET, &m_rtvClearValues[0]);
        m_gbufferTexNormalsUv.Init_Tex2D("GBuffer Normals + UV", d3d->GetDevice(), w, h, 1, DEFERRED_GBUFFER_FORMAT_NORMALS_UV, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET, &m_rtvClearValues[1]);
        m_gbufferTexMv.Init_Tex2D("GBuffer MV", d3d->GetDevice(), w, h, 1, DEFERRED_GBUFFER_FORMAT_MV, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET, &m_rtvClearValues[2]);
        m_gbufferTexEmission.Init_Tex2D("GBuffer Emission", d3d->GetDevice(), w, h, 1, DEFERRED_GBUFFER_FORMAT_EMISSION, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET, &m_rtvClearValues[3]);
        m_gbufferTexDepth.Init_Depth2D("GBuffer Depth", d3d->GetDevice(), w, h, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        m_heapRTV.Init("GBuffer Heap RTV", d3d->GetDevice(), NUM_RENDER_TARGETS, 0, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_heapIdxMatIdx = CreateRTV(d3d->GetDevice(), &m_heapRTV, &m_gbufferTexMaterialIdx);
        m_heapIdxNormalsUv = CreateRTV(d3d->GetDevice(), &m_heapRTV, &m_gbufferTexNormalsUv);
        m_heapIdxMv = CreateRTV(d3d->GetDevice(), &m_heapRTV, &m_gbufferTexMv);
        m_heapIdxEmission = CreateRTV(d3d->GetDevice(), &m_heapRTV, &m_gbufferTexEmission);

        m_heapDSV.Init("GBuffer Heap DSV", d3d->GetDevice(), 1, 0, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_heapIdxDepth = CreateDSV(d3d->GetDevice(), &m_heapDSV, &m_gbufferTexDepth, DEFERRED_GBUFFER_FORMAT_DEPTH);
    }

    std::vector<std::string> compileArgs;
    if (Config::GetRender().DepthBufferType == DepthBufferType::eReversed)
        compileArgs.emplace_back("-DUSING_REVERSED_Z");

    // GBuffer Pass
    {
        D3D12_STATIC_SAMPLER_DESC samplers[1] = {};
        InitializeSamplerLinearWrap(&samplers[0], 0);

        m_rootConstantsGBuffer.Init(0, 0, sizeof(CbvGBufferPrePass_PerInstance));

        m_rootSigGBuffer.AddRootConstants(&m_rootConstantsGBuffer);
        m_rootSigGBuffer.AddCbvRange(0, 1);
        m_rootSigGBuffer.AddSrvRange(0, 2);
        m_rootSigGBuffer.AddSrvRangeBindless(1);
        m_rootSigGBuffer.Init(d3d->GetDevice(), samplers, _countof(samplers));

        m_setGBuffer.Init(heap);
        m_setGBuffer.SetHasRootConstants();
        m_setGBuffer.AddBindlessSrv(1);
        m_setGBuffer.AddCBV(d3d->GetDevice(), 0, sizeof(CbvMatrices_VP), uploadHeapCBV);

        auto desc = CreateGraphicsPipelineDesc(m_rootSigGBuffer.Get(), GetStandardILD(), true);
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        desc.NumRenderTargets = NUM_RENDER_TARGETS;
        desc.RTVFormats[0] = DEFERRED_GBUFFER_FORMAT_MAT_IDX;
        desc.RTVFormats[1] = DEFERRED_GBUFFER_FORMAT_NORMALS_UV;
        desc.RTVFormats[2] = DEFERRED_GBUFFER_FORMAT_MV;
        desc.RTVFormats[3] = DEFERRED_GBUFFER_FORMAT_EMISSION;

        V(m_pipelineGBuffer.InitGraphics(d3d->GetDevice(), "Raster/DeferredGBuffer.hlsl", "Raster/DeferredGBuffer.hlsl", desc, compileArgs));
    }

    // Lighting Pass
    {
        D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
        InitializeSamplerLinearWrap(&samplers[0], 0);
        InitializeSamplerPointClamp(&samplers[1], 1);

        m_rootSigLighting.AddCbvRange(0, 2);
        m_rootSigLighting.AddSrvRange(0, 11);
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

    m_texPrimal.Init_Tex2D("Tex Primal", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_texBloom.Init_Tex2D("Tex Bloom", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

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

    m_setGBuffer.SetSRV_Buffer  (d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferInstanceData, scene->CPU.ObjectCount, sizeof(InstanceData));
    m_setGBuffer.SetSRV_Buffer  (d3d->GetDevice(), 0, 1, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));

    m_setLighting.SetSRV_Buffer (d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));
    m_setLighting.SetSRV_Buffer (d3d->GetDevice(), 0, 1, &scene->GPU.MegaBufferPunctualLights, scene->CPU.MegaBufferPunctualLightsCount, sizeof(PunctualLight));
    m_setLighting.SetSRV_Buffer (d3d->GetDevice(), 0, 2, m_shadowMapManager.GetShadowMapPuncInfo(), scene->CPU.MegaBufferPunctualLightsCount, sizeof(ShadowMapPuncInfo));
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 3, &m_brdfIntMap, m_brdfIntMap.GetDesc().Format);
    m_setLighting.SetSRV        (d3d->GetDevice(), 0, 4, envMap->GetCubemap(), srvDesc);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 5, m_shadowMapManager.GetShadowMapDir(), SHADOW_MAP_FORMAT_DEPTH_SRV);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 6, &m_gbufferTexMaterialIdx, DEFERRED_GBUFFER_FORMAT_MAT_IDX);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 7, &m_gbufferTexNormalsUv, DEFERRED_GBUFFER_FORMAT_NORMALS_UV);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 8, &m_gbufferTexDepth, DEFERRED_GBUFFER_FORMAT_DEPTH_SRV);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 9, &m_gbufferTexMv, DEFERRED_GBUFFER_FORMAT_MV);
    m_setLighting.SetSRV_Tex2D  (d3d->GetDevice(), 0, 10, &m_texBloom, m_texBloom.GetDesc().Format);
    m_setLighting.SetUAV_Tex2D  (d3d->GetDevice(), 0, 0, &m_texPrimal, m_texPrimal.GetDesc().Format);
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

    // Clear gbuffers and prepare as render target
    {
        CD3DX12_RECT scissorRect = SetViewportScissor(cmdList, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_gbufferTexMaterialIdx.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_gbufferTexNormalsUv.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_gbufferTexMv.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_gbufferTexEmission.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_gbufferTexDepth.Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
            m_heapRTV.GetDescriptorHandleAtIndex(m_heapIdxMatIdx),
            m_heapRTV.GetDescriptorHandleAtIndex(m_heapIdxNormalsUv),
            m_heapRTV.GetDescriptorHandleAtIndex(m_heapIdxMv),
            m_heapRTV.GetDescriptorHandleAtIndex(m_heapIdxEmission),
        };

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_heapDSV.GetDescriptorHandleAtIndex(m_heapIdxDepth);
        cmdList->OMSetRenderTargets(_countof(rtvs), rtvs, FALSE, &dsvHandle);

        for (int i = 0; i < _countof(rtvs); i++)
        {
            cmdList->ClearRenderTargetView(rtvs[i], reinterpret_cast<FLOAT*>(&m_rtvClearValues[i]), 1, &scissorRect);
        }

        const float depthClearValue = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? 1.0f : 0.0f;
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, depthClearValue, 0, 0, nullptr);
    }

    // GBuffer Pass
    {
        {
            m_setGBuffer.TransitionAllSRVToShaderResource(cmdList);

            cmdList->SetGraphicsRootSignature(m_rootSigGBuffer.Get());
            cmdList->SetPipelineState(m_pipelineGBuffer.GetPSO());
            renderInfo.Heap->Bind(cmdList);
            renderInfo.Heap->BindSceneTextures_Graphics(cmdList, m_rootSigGBuffer.GetParamIndexSrvBindless(0));
            m_setGBuffer.SetDescriptorTables_Graphics(cmdList);
        }

        CbvMatrices_VP matricesVP = {};
        XMStoreFloat4x4(&matricesVP.V, renderInfo.V);
        XMStoreFloat4x4(&matricesVP.P, P);
        XMStoreFloat4x4(&matricesVP.PrevV, m_prevV);
        m_setGBuffer.UpdateCBV(0, 0, &matricesVP);

        m_prevV = renderInfo.V;

        CbvGBufferPrePass_PerInstance perInstance{};

        for (int i = 0; i < scene->CPU.ObjectCount; ++i)
        {
            const Object& obj = scene->CPU.Objects[i];

            perInstance.InstanceIdx = i;
            m_rootConstantsGBuffer.Bind_Graphics(cmdList, &perInstance);

            cmdList->DrawIndexedInstanced(obj.IndexCount, 1, obj.MegaBufferIndexOffset,
                                          static_cast<int>(obj.MegaBufferVertexOffset), 0);
        }
    }

    // Bloom Pass
    {

    }

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

    //if (renderInfo.ForwardConfig->FlagEnabledEnvironmentMap)
    //{
    //    GPU_SCOPE(cmdList, "Skybox Pass");
    //    m_skybox.RenderForward(cmdList, &renderInfo.V, &renderInfo.P);
    //}
    //else
    //{
    //    D3D12_RECT rect;
    //    rect.top = 0;
    //    rect.left = Config::GetSystem().WindowAppGuiWidth;
    //    rect.bottom = Config::GetSystem().RtvHeight;
    //    rect.right = Config::GetSystem().WindowAppGuiWidth + Config::GetSystem().RtvWidth;
    //    cmdList->ClearRenderTargetView(rtvHandle, Config::GetRender().RtvClearColor, 1, &rect);
    //}

    // Copy to RTV
    {
        GPU_SCOPE(cmdList, "Copy Output to RTV");

        {
            GPU_SCOPE(cmdList, "Copy to RTV");

            m_texPrimal.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
            RTV->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

            RTV->CopyTextureInto(cmdList, m_texPrimal.GetResource(), Config::GetSystem().WindowAppGuiWidth);
        }
    }
}
