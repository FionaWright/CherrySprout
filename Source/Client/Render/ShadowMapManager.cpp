#include "System/pch.h"
#include "Render/ShadowMapManager.h"

#include <random>

#include "Debug/GPUEventScoped.h"
#include "Scene/Scene.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

// TODO: Inverse backface culling

void ShadowMapManager::Init(ID3D12Device* device, Heap* heap, UploadHeap* uploadHeapCBV)
{
    m_shadowMapDir.Init_Depth2D("Shadow Map Dir", device, 2048, 2048);

    m_heapDSV.Init("Shadow Map Heap DSV", device, 1, 0, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_heapIdxDir = CreateDSV(device, &m_heapDSV, &m_shadowMapDir, SHADOW_MAP_FORMAT_DEPTH);

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
    desc.SampleDesc.Count = 1;
    V(m_pipeline.InitGraphics(device, "Raster/ShadowMapVSPS.hlsl", "Raster/ShadowMapVSPS.hlsl", desc));
}

void ShadowMapManager::LoadSceneData(ID3D12Device* device, Scene* scene)
{
    // Init punctual light maps

    m_set.SetSRV_Buffer(device, 0, 0, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));
}

void ShadowMapManager::Render(ID3D12GraphicsCommandList* cmdList, Heap* heap, Scene* scene, const ShadowMapConfig& config)
{
    m_shadowMapDir.Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    {
        SetViewportScissor(cmdList, m_shadowMapDir.GetDesc().Width, m_shadowMapDir.GetDesc().Height);

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_heapDSV.GetDescriptorHandleAtIndex(m_heapIdxDir);
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

    if (m_matricesDirty)
    {
        XMFLOAT3 dirLightDir = normalize(config.DirLightDir);
        XMVECTOR L = XMLoadFloat3(&dirLightDir);

        XMVECTOR sceneCenter = XMVectorSet(0, 0, 0, 1);
        XMVECTOR lightPosition = sceneCenter - L * config.DirDist;

        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        if (abs(XMVectorGetX(XMVector3Dot(L, up))) > 0.99f)
        {
            up = XMVectorSet(0, 0, 1, 0);
        }

        m_V = XMMatrixLookAtLH(lightPosition, sceneCenter, up);

        // Projection
        if (config.Width > 0 && config.Height > 0)
        {
            // TODO: Automatic calculate from camera frustum

            const float nearZ = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? config.Near : config.Far;
            const float farZ = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? config.Far : config.Near;

            m_P = XMMatrixOrthographicLH(
                    config.Width,
                    config.Height,
                    nearZ,
                    farZ
                );
        }

        m_matricesDirty = false;
    }

    CbvMatrices_VP matricesVP = {};
    XMStoreFloat4x4(&matricesVP.V, m_V);
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

void ShadowMapManager::GeneratePoissonSamples(const uint32_t count, const float minDistance)
{
    m_poissonDiscSamples.clear();
    m_poissonDiscSamples.reserve(count);

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    constexpr int MAX_ATTEMPTS = 10000;

    for (int attempt = 0; attempt < MAX_ATTEMPTS && m_poissonDiscSamples.size() < count; ++attempt)
    {
        // Generate a uniform random point inside the unit circle.
        const float angle = dist(rng) * 2.0f * 3.14159265f;
        const float radius = std::sqrt(dist(rng));

        XMFLOAT2 candidate =
        {
            std::cos(angle) * radius,
            std::sin(angle) * radius
        };

        // Check minimum distance from existing samples.
        bool valid = true;

        for (const XMFLOAT2& sample : m_poissonDiscSamples)
        {
            const float dx = candidate.x - sample.x;
            const float dy = candidate.y - sample.y;

            const float distanceSq = dx * dx + dy * dy;

            if (distanceSq < minDistance * minDistance)
            {
                valid = false;
                break;
            }
        }

        if (valid)
            m_poissonDiscSamples.push_back(candidate);
    }
}
