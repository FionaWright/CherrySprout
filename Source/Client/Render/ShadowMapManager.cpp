#include "System/pch.h"
#include "Render/ShadowMapManager.h"

#include "Debug/GPUEventScoped.h"
#include "Scene/Scene.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

#define SHADOW_MAP_FORMAT_DEPTH        DXGI_FORMAT_D32_FLOAT
#define SHADOW_MAP_FORMAT_DEPTH_SRV    DXGI_FORMAT_R32_FLOAT

void ShadowMapManager::Init(ID3D12Device* device, Heap* heap, UploadHeap* uploadHeapCBV)
{
    m_shadowMapDir.Init_Depth2D("Shadow Map Dir", device, 2048, 2048);

    m_heapDSV.Init("GBuffer Pre-Pass Heap DSV", device, 1, 0, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_heapIdxDepth = CreateDSV(device, &m_heapDSV, &m_shadowMapDir, SHADOW_MAP_FORMAT_DEPTH);

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);

    m_rootConstants.Init(0, 0, sizeof(CbvForward_PerInstance));
    m_rootSig.AddRootConstants(&m_rootConstants);
    m_rootSig.AddCbvRange(0, 1);
    m_rootSig.AddSrvRange(0, 1);
    m_rootSig.AddSceneTextures(1);
    m_rootSig.Init(device, &sampler, 1);

    m_set.Init(heap, true, true);
    m_set.AddCBV(device, 0, sizeof(CbvMatrices_VP), uploadHeapCBV);

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

    auto desc = CreateGraphicsPipelineDesc(m_rootSig.Get(), { ildDesc, _countof(ildDesc) }, true, 0);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    V(m_pipeline.InitGraphics(device, "Raster/ShadowMapVSPS.hlsl", "Raster/ShadowMapVSPS.hlsl", desc));

    // Projection
    {
        // TODO: Automatic calculate from camera frustum
        float nearPlane = 0.1f;
        float farPlane  = 500.0f;

        m_P = XMMatrixOrthographicLH(
                m_shadowMapDir.GetDesc().Width,
                m_shadowMapDir.GetDesc().Height,
                nearPlane,
                farPlane
            );
    }
}

void ShadowMapManager::LoadSceneData(ID3D12Device* device, Scene* scene)
{
    // Init punctual light maps

    m_set.SetSRV_Buffer(device, 0, 0, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));
}

void ShadowMapManager::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Heap* heap, Scene* scene, XMFLOAT3 dirLightDir)
{
    {
        SetViewportScissor(cmdList, m_shadowMapDir.GetDesc().Width, m_shadowMapDir.GetDesc().Height);

        const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(d3d->GetDsvHeapStart(), 0, d3d->GetDsvDescriptorSize());
        cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

        const float depthClearValue = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? 1.0f : 0.0f;
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthClearValue, 0, 0, nullptr);
    }

    heap->Bind(cmdList);

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

        heap->BindSceneTextures_Graphics(cmdList, m_rootSig.GetParamIndexSceneTextures());
    }

    XMMATRIX V;
    {
        float lightDistance = 100.0f; // TODO

        dirLightDir = normalize(dirLightDir);
        XMVECTOR L = XMLoadFloat3(&dirLightDir);

        XMVECTOR sceneCenter = XMVectorSet(0, 0, 0, 1);
        XMVECTOR lightPosition = sceneCenter - L * lightDistance;

        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        if (abs(XMVectorGetX(XMVector3Dot(L, up))) > 0.99f)
        {
            up = XMVectorSet(0, 0, 1, 0);
        }

        V = XMMatrixLookAtLH(lightPosition, sceneCenter, up);
    }

    CbvMatrices_VP matricesVP = {};
    XMStoreFloat4x4(&matricesVP.V, V);
    XMStoreFloat4x4(&matricesVP.P, m_P);
    m_set.UpdateCBV(0, 0, &matricesVP);

    m_set.SetDescriptorTables_Graphics(cmdList);

    CbvShadowMap_PerInstance matricesM = {};

    // TODO: Sort objects

    for (int i = 0; i < scene->CPU.ObjectCount; ++i)
    {
        const Object& obj = scene->CPU.Objects[i];

        // TODO: Cache
        const XMMATRIX M = XMMatrixSet(
            obj.M[0], obj.M[1], obj.M[2], obj.M[3],
            obj.M[4], obj.M[5], obj.M[6], obj.M[7],
            obj.M[8], obj.M[9], obj.M[10], obj.M[11],
            obj.M[12], obj.M[13], obj.M[14], obj.M[15]
        );

        XMStoreFloat4x4(&matricesM.M, M);
        matricesM.MaterialIdx = obj.MaterialIndex;
        m_rootConstants.Bind_Graphics(cmdList, &matricesM);

        cmdList->DrawIndexedInstanced(obj.MegaBufferIndexCount, 1, obj.MegaBufferIndexOffset, obj.MegaBufferVertexOffset, 0);
    }
}
