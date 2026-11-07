//
// Created by fiona on 01/06/2026.
//

#include "System/pch.h"
#include "Forward.h"

#include "Greenhouse.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/Heap.h"
#include "Raster/Utils/ShadowMapPuncInfo.h"
#include "Scene/Scene.h"
#include "System/FileHelper.h"
#include "System/HighResolutionClock.h"
#include "System/TextureLoader.h"
#include "Utils/D3DUtils.h"

void Forward::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    IRenderBackend::Init(d3d, heap, uploadHeapCBV);

    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
    InitializeSamplerLinearClamp(&samplers[0], 0);
    InitializeSamplerPointClamp(&samplers[1], 1);

    m_rootConstants.Init(0, 0, sizeof(CbvForward_PerInstance));

    m_rootSig.AddRootConstants(&m_rootConstants);
    m_rootSig.AddCbvRange(0, 3);
    m_rootSig.AddSrvRange(0, 6);
    m_rootSig.AddSrvRangeBindless(1);
    m_rootSig.AddSrvRangeBindless(2);
    m_rootSig.AddSrvRangeBindless(3);
    m_rootSig.Init(d3d->GetDevice(), samplers, _countof(samplers));

    m_set.Init(heap);
    m_set.SetHasRootConstants();
    m_set.AddBindlessSrv(1);
    m_set.AddCBV(d3d->GetDevice(), 0, sizeof(CbvMatrices_VP), uploadHeapCBV);
    m_set.AddCBV(d3d->GetDevice(), 0, sizeof(CbvForward), uploadHeapCBV);
    m_set.AddCBV(d3d->GetDevice(), 0, sizeof(CbvShadowMapSample), uploadHeapCBV);

    D3D12_INPUT_ELEMENT_DESC ildDesc[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
    };

    auto desc = CreateGraphicsPipelineDesc(m_rootSig.Get(), { ildDesc, _countof(ildDesc) }, true);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    V(m_pipeline.InitGraphics(d3d->GetDevice(), "Raster/Forward.hlsl", "Raster/Forward.hlsl", desc));

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

void Forward::LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap, LightImportanceSampler* lightImportanceSampler, GBufferPrePass* gbuffer)
{
    IRenderBackend::LoadSceneData(d3d, scene, heap, uploadHeapCBV, envMap, lightImportanceSampler, gbuffer);

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

    m_set.SetSRV_Buffer (d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));
    m_set.SetSRV_Buffer (d3d->GetDevice(), 0, 1, &scene->GPU.MegaBufferPunctualLights, scene->CPU.MegaBufferPunctualLightsCount, sizeof(PunctualLight));
    m_set.SetSRV_Buffer (d3d->GetDevice(), 0, 2, m_shadowMapManager.GetShadowMapPuncInfo(), scene->CPU.MegaBufferPunctualLightsCount, sizeof(ShadowMapPuncInfo));
    m_set.SetSRV_Tex2D  (d3d->GetDevice(), 0, 3, &m_brdfIntMap, m_brdfIntMap.GetDesc().Format);
    m_set.SetSRV        (d3d->GetDevice(), 0, 4, envMap->GetCubemap(), srvDesc);
    m_set.SetSRV_Tex2D  (d3d->GetDevice(), 0, 5, m_shadowMapManager.GetShadowMapDir(), SHADOW_MAP_FORMAT_DEPTH_SRV);
}

void Forward::Update(D3D* d3d, Heap* heap, TimeArgs timeArgs)
{
}

void Forward::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo)
{

}

void Forward::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* RTV, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    GPU_SCOPE(cmdList, "Forward Backend");

    Scene* scene = renderInfo.Scene;
    Heap* heap = renderInfo.Heap;

    // Shadow Map
    {
        ShadowMapConfig config;
        config.Width = renderInfo.ForwardConfig->SmWidth;
        config.Height = renderInfo.ForwardConfig->SmHeight;
        config.CascadeCount = renderInfo.ForwardConfig->SmCascadeCount;
        config.Near = renderInfo.ForwardConfig->SmNear;
        config.Far = renderInfo.ForwardConfig->SmFar;
        config.DirDist = renderInfo.ForwardConfig->SmDirDist;
        config.DirLightDir = renderInfo.BackendConfig->DirLightDirection;
        m_shadowMapManager.Render(cmdList, heap, scene, config);
    }

    {
        SetViewportScissor(cmdList, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, Config::GetSystem().WindowAppGuiWidth);

        RTV->Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(d3d->GetDsvHeapStart(), 0, d3d->GetDsvDescriptorSize());
        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        const float depthClearValue = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? 1.0f : 0.0f;
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthClearValue, 0, 0, nullptr);
    }

    heap->Bind(cmdList);

    if (renderInfo.ForwardConfig->FlagEnabledEnvironmentMap)
    {
        GPU_SCOPE(cmdList, "Skybox Pass");
        m_skybox.RenderForward(cmdList, &renderInfo.V, &renderInfo.P);
    }
    else
    {
        D3D12_RECT rect;
        rect.top = 0;
        rect.left = Config::GetSystem().WindowAppGuiWidth;
        rect.bottom = Config::GetSystem().RtvHeight;
        rect.right = Config::GetSystem().WindowAppGuiWidth + Config::GetSystem().RtvWidth;
        cmdList->ClearRenderTargetView(rtvHandle, Config::GetRender().RtvClearColor, 1, &rect);
    }

    {
        D3D12_VERTEX_BUFFER_VIEW viewV;
        D3D12_INDEX_BUFFER_VIEW viewI;
        VertexIndexBuffersToViews(&scene->GPU.MegaBufferVertex, &scene->GPU.MegaBufferIndex,
            scene->CPU.MegaBufferVertexCount, sizeof(Vertex), scene->CPU.MegaBufferIndexCount,
                    viewV, viewI);

        scene->GPU.MegaBufferVertex.Transition(cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        scene->GPU.MegaBufferIndex.Transition(cmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        m_set.TransitionAllSRVToShaderResource(cmdList);

        cmdList->IASetVertexBuffers(0, 1, &viewV);
        cmdList->IASetIndexBuffer(&viewI);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmdList->SetGraphicsRootSignature(m_rootSig.Get());
        cmdList->SetPipelineState(m_pipeline.GetPSO());

        heap->BindSceneTextures_Graphics(cmdList, m_rootSig.GetParamIndexSrvBindless(0));
        if (m_shadowMapManager.GetHeapBasePoint() != UINT_MAX)
            heap->BindBindless_Graphics(cmdList, m_shadowMapManager.GetHeapBasePoint(), m_rootSig.GetParamIndexSrvBindless(1));
    }

    CbvMatrices_VP matricesVP = {};
    XMStoreFloat4x4(&matricesVP.V, renderInfo.V);
    XMStoreFloat4x4(&matricesVP.P, renderInfo.P);
    m_set.UpdateCBV(0, 0, &matricesVP);

    CbvForward cbvForward{};
    cbvForward.CameraPosition = renderInfo.Camera->GetPosition();
    cbvForward.FrameIdx = renderInfo.FrameIndex;
    cbvForward.DirLightDir = renderInfo.BackendConfig->DirLightDirection;
    cbvForward.MaxCubemapMipMaps = renderInfo.EnvironmentMap->GetCubemap()->GetDesc().MipLevels;
    cbvForward.Flags = ForwardFlags::eNone;
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::eEnvironmentMap, renderInfo.ForwardConfig->FlagEnabledEnvironmentMap);
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::ePunctualLights, renderInfo.ForwardConfig->FlagEnabledPunctualLights);
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::eDirectionalLight, renderInfo.ForwardConfig->FlagEnabledDirectionalLight);
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::eShadowMaps, renderInfo.ForwardConfig->FlagEnabledShadowMaps);
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::eAmbient, renderInfo.ForwardConfig->FlagEnabledAmbient);
    m_set.UpdateCBV(0, 1, &cbvForward);

    CbvShadowMapSample cbvShadow{};
    XMStoreFloat4x4(&cbvShadow.ShadowMatrices[0], m_shadowMapManager.GetShadowMatrix(0));
    XMFLOAT2 dim = m_shadowMapManager.GetShadowDimensions(0);
    cbvShadow.TexelSize = XMFLOAT2(1.0f / dim.x, 1.0f / dim.y);
    cbvShadow.BiasDepth = renderInfo.ForwardConfig->SmBiasDepth;
    cbvShadow.CascadeCount = renderInfo.ForwardConfig->SmCascadeCount;
    cbvShadow.NormalBias = renderInfo.ForwardConfig->SmBiasNormal;
    cbvShadow.PcfSampleCount = renderInfo.ForwardConfig->SmPcfCount;
    cbvShadow.PcfRadius = renderInfo.ForwardConfig->SmPcfRadius;
    auto poissonDisc = m_shadowMapManager.GetPoissonSamples();
    for (uint32_t i = 0; i < POISSON_DISC_COUNT; i++)
    {
        cbvShadow.PoissonDisc[i].x = poissonDisc[i*2+0].x;
        cbvShadow.PoissonDisc[i].y = poissonDisc[i*2+0].y;
        cbvShadow.PoissonDisc[i].z = poissonDisc[i*2+1].x;
        cbvShadow.PoissonDisc[i].w = poissonDisc[i*2+1].y;
    }
    m_set.UpdateCBV(0, 2, &cbvShadow);

    m_set.SetDescriptorTables_Graphics(cmdList);

    CbvForward_PerInstance matricesM = {};

    // TODO: Sort objects

    for (int i = 0; i < scene->CPU.ObjectCount; ++i)
    {
        const Object& obj = scene->CPU.Objects[i];

        //GPU_SCOPE(cmdList, obj.DebugName);

        const XMMATRIX M = XMMatrixSet(
            obj.M[0], obj.M[1], obj.M[2], obj.M[3],
            obj.M[4], obj.M[5], obj.M[6], obj.M[7],
            obj.M[8], obj.M[9], obj.M[10], obj.M[11],
            obj.M[12], obj.M[13], obj.M[14], obj.M[15]
            );

        XMStoreFloat4x4(&matricesM.M, M);
        XMStoreFloat4x4(&matricesM.MTI, XMMatrixTranspose(XMMatrixInverse(nullptr, M)));
        matricesM.MaterialIdx = obj.MaterialIndex;
        m_rootConstants.Bind_Graphics(cmdList, &matricesM);

        cmdList->DrawIndexedInstanced(obj.MegaBufferIndexCount, 1, obj.MegaBufferIndexOffset, obj.MegaBufferVertexOffset, 0);
    }
}
