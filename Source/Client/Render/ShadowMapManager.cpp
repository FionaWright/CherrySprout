#include "System/pch.h"
#include "Render/ShadowMapManager.h"

#include <random>

#include "Debug/GPUEventScoped.h"
#include "Scene/Scene.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

#define MAX_SHADOW_MAPS 100
#define SHADOW_MAP_RESO 2048

void ShadowMapManager::Init(ID3D12Device* device, Heap* heap, UploadHeap* uploadHeapCBV)
{
    m_heapDSV.Init("Shadow Map Heap DSV", device, MAX_SHADOW_MAPS, 0, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_shadowMapDir.Resource.Init_Depth2D("Shadow Map Dir", device, SHADOW_MAP_RESO, SHADOW_MAP_RESO);
    m_shadowMapDir.HeapIdx = CreateDSV(device, &m_heapDSV, &m_shadowMapDir.Resource, SHADOW_MAP_FORMAT_DEPTH);

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);

    m_rootConstants.Init(0, 0, sizeof(CbvForward_PerInstance));
    m_rootSig.AddRootConstants(&m_rootConstants);
    m_rootSig.AddCbvRange(0, 1);
    m_rootSig.AddSrvRange(0, 1);
    m_rootSig.AddSceneTextures(1);
    m_rootSig.Init(device, &sampler, 1);

    m_set.Init(heap, true, true);
    m_set.AddCBV(device, 0, sizeof(CbvShadowMapCreate), uploadHeapCBV);

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
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
    desc.SampleDesc.Count = 1;
    V(m_pipelineDistant.InitGraphics(device, "Raster/ShadowMapVSPS.hlsl", "Raster/ShadowMapVSPS.hlsl", desc));
    V(m_pipelinePoint.InitGraphics(device, "Raster/ShadowMapPointVSPS.hlsl", "Raster/ShadowMapPointVSPS.hlsl", desc));
}

void ShadowMapManager::LoadSceneData(ID3D12Device* device, Scene* scene)
{
    CherryAssert(scene->CPU.MegaBufferPunctualLightsCount + 1 < MAX_SHADOW_MAPS);

    m_shadowMapPunctuals.clear();
    m_shadowMapPunctuals.resize(scene->CPU.MegaBufferPunctualLightsCount);
    for (uint32_t i = 0; i < scene->CPU.MegaBufferPunctualLightsCount; i++)
    {
        const PunctualLight& light = scene->CPU.MegaBufferPunctualLights[i];
        if (light.Intensity <= 0.0f)
            continue;

        if (light.Type == PunctualLightType::eDistant)
        {
            m_shadowMapPunctuals[i].Resource.Init_Depth2D("Punctual Shadow Map (Distant)", device, SHADOW_MAP_RESO, SHADOW_MAP_RESO);
            m_shadowMapPunctuals[i].HeapIdx = CreateDSV(device, &m_heapDSV, &m_shadowMapPunctuals[i].Resource, SHADOW_MAP_FORMAT_DEPTH);
        }
        else if (light.Type == PunctualLightType::ePoint)
        {
            m_shadowMapPunctuals[i].Resource.Init_Depth2D("Punctual Shadow Map (Point)", device, SHADOW_MAP_RESO, SHADOW_MAP_RESO, 6);
            m_shadowMapPunctuals[i].HeapIdx = CreateDSV(device, &m_heapDSV, &m_shadowMapPunctuals[i].Resource, SHADOW_MAP_FORMAT_DEPTH);
        }
        else
            __debugbreak();
    }

    m_set.SetSRV_Buffer(device, 0, 0, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));

    m_matricesDirty = true;
}

XMMATRIX ShadowMapManager::getViewMatrixDistant(const XMFLOAT3 direction, const float distance) const
{
    const XMFLOAT3 dirLightDir = normalize(direction);
    const XMVECTOR L = XMLoadFloat3(&dirLightDir);

    const XMVECTOR sceneCenter = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR lightPosition = sceneCenter - L * distance;

    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    if (abs(XMVectorGetX(XMVector3Dot(L, up))) > 0.99f)
    {
        up = XMVectorSet(0, 0, 1, 0);
    }

    return XMMatrixLookAtLH(lightPosition, sceneCenter, up);
}

// TODO: needs manual up vector?
XMMATRIX ShadowMapManager::getViewMatrixPoint(const XMFLOAT3 position, XMFLOAT3 direction) const
{
    const XMVECTOR lightPosition = XMLoadFloat3(&position);

    direction = normalize(direction);
    const XMVECTOR dir = XMLoadFloat3(&direction);

    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    if (abs(XMVectorGetX(XMVector3Dot(dir, up))) > 0.99f)
    {
        up = XMVectorSet(0, 0, 1, 0);
    }

    return XMMatrixLookToLH(lightPosition, dir, up);
}

// TODO: Automatically compute params from camera frustum
XMMATRIX ShadowMapManager::getProjectionMatrix(const float width, const float height, const float nearPlane, const float farPlane)
{
    const float nearZ = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? nearPlane : farPlane;
    const float farZ = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? farPlane : nearPlane;
    return XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

void ShadowMapManager::renderSceneObjectsToBoundMap(ID3D12GraphicsCommandList* cmdList, const Scene* scene) const
{
    CbvShadowMap_PerInstance matricesM = {};

    // TODO: Sort objects
    // TODO: Frustum Cull

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

void ShadowMapManager::renderDistant(ID3D12GraphicsCommandList* cmdList, const Scene* scene, ShadowMap* shadowMap) const
{
    shadowMap->Resource.Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    cmdList->SetPipelineState(m_pipelineDistant.GetPSO());

    {
        SetViewportScissor(cmdList, shadowMap->Resource.GetDesc().Width, shadowMap->Resource.GetDesc().Height);

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_heapDSV.GetDescriptorHandleAtIndex(shadowMap->HeapIdx);
        cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

        const float depthClearValue = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? 1.0f : 0.0f;
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthClearValue, 0, 0, nullptr);
    }

    CbvShadowMapCreate cbv = {};
    XMStoreFloat4x4(&cbv.V, shadowMap->Vs[0]);
    XMStoreFloat4x4(&cbv.P, shadowMap->P);
    m_set.UpdateCBV(0, 0, &cbv);

    renderSceneObjectsToBoundMap(cmdList, scene);
}

void ShadowMapManager::renderPoint(ID3D12GraphicsCommandList* cmdList, const Scene* scene, ShadowMap* shadowMap, const XMFLOAT3 position, const float farPlane) const
{
    shadowMap->Resource.Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    cmdList->SetPipelineState(m_pipelinePoint.GetPSO());

    {
        SetViewportScissor(cmdList, shadowMap->Resource.GetDesc().Width, shadowMap->Resource.GetDesc().Height);

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_heapDSV.GetDescriptorHandleAtIndex(shadowMap->HeapIdx);
        cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

        const float depthClearValue = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? 1.0f : 0.0f;
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthClearValue, 0, 0, nullptr);
    }

    for (uint32_t i = 0; i < 6; i++)
    {
        CbvShadowMapCreate cbv = {};
        XMStoreFloat4x4(&cbv.V, shadowMap->Vs[i]);
        XMStoreFloat4x4(&cbv.P, shadowMap->P);
        cbv.PointLightPosition = position;
        cbv.FarPlane = farPlane;
        m_set.UpdateCBV(0, 0, &cbv);

        renderSceneObjectsToBoundMap(cmdList, scene);
    }
}

void ShadowMapManager::Render(ID3D12GraphicsCommandList* cmdList, const Heap* heap, Scene* scene, const ShadowMapConfig& config)
{
    if (m_matricesDirty)
    {
        m_shadowMapDir.Vs.resize(1);
        m_shadowMapDir.Vs[0] = getViewMatrixDistant(config.DirLightDir, config.DirDist);

        // Projection
        if (config.Width > 0 && config.Height > 0)
            m_shadowMapDir.P = getProjectionMatrix(config.Width, config.Height, config.Near, config.Far);

        for (uint32_t i = 0; i < scene->CPU.MegaBufferPunctualLightsCount; i++)
        {
            const PunctualLight& light = scene->CPU.MegaBufferPunctualLights[i];
            if (light.Intensity <= 0.0f)
                continue;

            if (light.Type == PunctualLightType::eDistant)
            {
                m_shadowMapPunctuals[i].Vs.resize(1);
                m_shadowMapPunctuals[i].Vs[0] = getViewMatrixDistant(light.Position, config.DirDist);
            }
            else if (light.Type == PunctualLightType::ePoint)
            {
                m_shadowMapPunctuals[i].Vs.resize(6);
                m_shadowMapPunctuals[i].Vs[0] = getViewMatrixPoint(light.Position, XMFLOAT3(1,0,0));
                m_shadowMapPunctuals[i].Vs[1] = getViewMatrixPoint(light.Position, XMFLOAT3(-1,0,0));
                m_shadowMapPunctuals[i].Vs[2] = getViewMatrixPoint(light.Position, XMFLOAT3(0,1,0));
                m_shadowMapPunctuals[i].Vs[3] = getViewMatrixPoint(light.Position, XMFLOAT3(0,-1,0));
                m_shadowMapPunctuals[i].Vs[4] = getViewMatrixPoint(light.Position, XMFLOAT3(0,0,1));
                m_shadowMapPunctuals[i].Vs[5] = getViewMatrixPoint(light.Position, XMFLOAT3(0,0,-1));
            }

            if (config.Width > 0 && config.Height > 0)
                m_shadowMapPunctuals[i].P = getProjectionMatrix(config.Width, config.Height, config.Near, config.Far);
        }

        m_matricesDirty = false;
    }

    heap->Bind(cmdList);

    // Bind VB/IB
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

        heap->BindSceneTextures_Graphics(cmdList, m_rootSig.GetParamIndexSceneTextures());
    }

    m_set.SetDescriptorTables_Graphics(cmdList);

    renderDistant(cmdList, scene, &m_shadowMapDir);

    for (uint32_t i = 0; i < scene->CPU.MegaBufferPunctualLightsCount; i++)
    {
        const PunctualLight& light = scene->CPU.MegaBufferPunctualLights[i];
        if (light.Intensity <= 0.0f)
            continue;

        if (light.Type == PunctualLightType::eDistant)
        {
            renderDistant(cmdList, scene, &m_shadowMapPunctuals[i]);
        }
        else if (light.Type == PunctualLightType::ePoint)
        {
            renderPoint(cmdList, scene, &m_shadowMapPunctuals[i], light.Position, config.Far);
        }
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
