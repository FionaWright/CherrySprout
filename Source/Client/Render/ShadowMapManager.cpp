#include "System/pch.h"
#include "Render/ShadowMapManager.h"

#include <random>

#include "Debug/GPUEventScoped.h"
#include "Raster/Utils/ShadowMapPuncInfo.h"
#include "Scene/Scene.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

#define MAX_SHADOW_MAPS 500
#define SHADOW_MAP_RESO_DISTANT 2048
#define SHADOW_MAP_RESO_POINT 512

void ShadowMapManager::Init(ID3D12Device* device, Heap* heap, UploadHeap* uploadHeapCBV)
{
    m_heapDSV.Init("Shadow Map Heap DSV", device, MAX_SHADOW_MAPS, 0, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_shadowMapDir.Resource.Init_Depth2D("Shadow Map Dir", device, SHADOW_MAP_RESO_DISTANT, SHADOW_MAP_RESO_DISTANT);
    m_shadowMapDir.DsvHeapIndices.emplace_back(CreateDSV(device, &m_heapDSV, &m_shadowMapDir.Resource, SHADOW_MAP_FORMAT_DEPTH));

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);

    m_rootConstants.Init(0, 0, sizeof(CbvShadowMap_PerInstance));
    m_rootSig.AddRootConstants(&m_rootConstants);
    m_rootSig.AddSrvRange(0, 1);
    m_rootSig.AddSrvRangeBindless(1);
    m_rootSig.Init(device, &sampler, 1);

    m_set.Init(heap);
    m_set.SetHasRootConstants();
    m_set.AddBindlessSrv(1);

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
    desc.SampleDesc.Count = 1;

    desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
    V(m_pipelineDistant.InitGraphics(device, "Raster/ShadowMapVSPS.hlsl", "Raster/ShadowMapVSPS.hlsl", desc));

    desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    V(m_pipelinePoint.InitGraphics(device, "Raster/ShadowMapPointVSPS.hlsl", "Raster/ShadowMapPointVSPS.hlsl", desc));
}

void ShadowMapManager::LoadSceneData(D3D* d3d, Heap* heap, Scene* scene)
{
    CherryAssert(scene->CPU.MegaBufferPunctualLightsCount + 1 < MAX_SHADOW_MAPS);

    m_shadowMapPunctuals.clear();
    m_shadowMapPunctuals.resize(scene->CPU.MegaBufferPunctualLightsCount);
    for (uint32_t i = 0; i < scene->CPU.MegaBufferPunctualLightsCount; i++)
    {
        const PunctualLight& light = scene->CPU.MegaBufferPunctualLights[i];
        if (light.Intensity <= 0.0f || light.Type != PunctualLightType::eDistant)
            continue;

        m_shadowMapPunctuals[i].Resource.Init_Depth2D("Punctual Shadow Map (Distant)", d3d->GetDevice(), SHADOW_MAP_RESO_DISTANT, SHADOW_MAP_RESO_DISTANT);
        m_shadowMapPunctuals[i].DsvHeapIndices.emplace_back(CreateDSV(d3d->GetDevice(), &m_heapDSV, &m_shadowMapPunctuals[i].Resource, SHADOW_MAP_FORMAT_DEPTH));

        // TODO
    }

    bool assignedHeapBase = false;
    for (uint32_t i = 0; i < scene->CPU.MegaBufferPunctualLightsCount; i++)
    {
        const PunctualLight& light = scene->CPU.MegaBufferPunctualLights[i];
        if (light.Intensity <= 0.0f || light.Type != PunctualLightType::ePoint)
            continue;

        m_shadowMapPunctuals[i].Resource.Init_Depth2D("Punctual Shadow Map (Point)", d3d->GetDevice(), SHADOW_MAP_RESO_POINT, SHADOW_MAP_RESO_POINT, 6);
        m_shadowMapPunctuals[i].DsvHeapIndices = CreateDSVs_Array(d3d->GetDevice(), &m_heapDSV, &m_shadowMapPunctuals[i].Resource, SHADOW_MAP_FORMAT_DEPTH);

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        desc.Format = SHADOW_MAP_FORMAT_DEPTH_SRV;
        desc.TextureCube.MipLevels = 1;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_shadowMapPunctuals[i].HeapIdx = heap->AddSRV_Bindless(d3d->GetDevice(), &m_shadowMapPunctuals[i].Resource, desc);
        if (!assignedHeapBase)
            m_heapBasePoint = m_shadowMapPunctuals[i].HeapIdx;
        assignedHeapBase = true;
    }

    const size_t expectedPuncInfoSize = scene->CPU.MegaBufferPunctualLightsCount * sizeof(ShadowMapPuncInfo);
    if (!m_shadowMapPuncInfoBuffer.IsInitialized() || m_shadowMapPuncInfoBuffer.GetDesc().Width != expectedPuncInfoSize)
    {
        m_shadowMapPuncInfoBuffer.Reset();
        m_shadowMapPuncInfoBuffer.Init_Buffer("Shadow Map Punc Info", d3d->GetDevice(), expectedPuncInfoSize);

        std::vector<ShadowMapPuncInfo> cpuSmPuncInfoBuffer;
        cpuSmPuncInfoBuffer.resize(scene->CPU.MegaBufferPunctualLightsCount);

        for (uint32_t i = 0; i < scene->CPU.MegaBufferPunctualLightsCount; i++)
        {
            const PunctualLight& light = scene->CPU.MegaBufferPunctualLights[i];
            if (light.Intensity <= 0.0f)
                continue;

            // TODO: ShadowMatrix, TexelSize
            cpuSmPuncInfoBuffer[i] = {};
            cpuSmPuncInfoBuffer[i].ShadowMapIndex =
                light.Type == PunctualLightType::eDistant ? m_shadowMapPunctuals[i].HeapIdx - m_heapBaseDistant :
                light.Type == PunctualLightType::ePoint ? m_shadowMapPunctuals[i].HeapIdx - m_heapBasePoint : UINT_MAX;
            cpuSmPuncInfoBuffer[i].NearPlane = 0.01f; // TODO
            cpuSmPuncInfoBuffer[i].FarPlane = 100.0f; // TODO
        }

        UploadHeap uploadHeap;
        uploadHeap.Init(d3d->GetDevice(), Align(m_shadowMapPuncInfoBuffer.GetIntermediateSize(), 512));

        d3d->Flush();
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();
        {
            m_shadowMapPuncInfoBuffer.UploadBuffer(cmdList, &uploadHeap, cpuSmPuncInfoBuffer.data(), expectedPuncInfoSize);
        }
        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }

    m_set.SetSRV_Buffer(d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));

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
XMMATRIX ShadowMapManager::getProjectionMatrixDistant(const float width, const float height, const float nearPlane, const float farPlane)
{
    const float nearZ = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? nearPlane : farPlane;
    const float farZ = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? farPlane : nearPlane;
    return XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

XMMATRIX ShadowMapManager::getProjectionMatrixPoint(const float width, const float height, const float nearPlane, const float farPlane)
{
    const float FoV = 90.0f;

    const float aspectRatio = width / height;
    const float nearZ = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? nearPlane : farPlane;
    const float farZ = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? farPlane : nearPlane;
    return XMMatrixPerspectiveFovLH(XMConvertToRadians(FoV), aspectRatio, nearZ, farZ);
}

void ShadowMapManager::renderSceneObjectsToBoundMap(ID3D12GraphicsCommandList* cmdList, const Scene* scene, CbvShadowMap_PerInstance& cbv) const
{
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

        XMStoreFloat4x4(&cbv.M, M);
        cbv.MaterialIdx = obj.MaterialIndex;
        m_rootConstants.Bind_Graphics(cmdList, &cbv);

        cmdList->DrawIndexedInstanced(obj.MegaBufferIndexCount, 1, obj.MegaBufferIndexOffset, obj.MegaBufferVertexOffset, 0);
    }
}

void ShadowMapManager::renderDistant(ID3D12GraphicsCommandList* cmdList, const Scene* scene, ShadowMap* shadowMap) const
{
    GPU_SCOPE(cmdList, "Shadow Map (Distant)");

    shadowMap->Resource.Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    cmdList->SetPipelineState(m_pipelineDistant.GetPSO());

    {
        SetViewportScissor(cmdList, shadowMap->Resource.GetDesc().Width, shadowMap->Resource.GetDesc().Height);

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_heapDSV.GetDescriptorHandleAtIndex(shadowMap->DsvHeapIndices[0]);
        cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

        const float depthClearValue = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? 1.0f : 0.0f;
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthClearValue, 0, 0, nullptr);
    }

    CbvShadowMap_PerInstance cbv = {};
    XMStoreFloat4x4(&cbv.V, shadowMap->Vs[0]);
    XMStoreFloat4x4(&cbv.P, shadowMap->P);

    renderSceneObjectsToBoundMap(cmdList, scene, cbv);
}

void ShadowMapManager::renderPoint(ID3D12GraphicsCommandList* cmdList, const Scene* scene, ShadowMap* shadowMap, const XMFLOAT3 position, const float farPlane) const
{
    GPU_SCOPE(cmdList, "Shadow Map (Point)");

    shadowMap->Resource.Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    cmdList->SetPipelineState(m_pipelinePoint.GetPSO());

    {
        SetViewportScissor(cmdList, shadowMap->Resource.GetDesc().Width, shadowMap->Resource.GetDesc().Height);
    }

    for (uint32_t i = 0; i < 6; i++)
    {
        GPU_SCOPE(cmdList, (std::string("Shadow Map (Point, Face ") + std::to_string(i) + ")").c_str());

        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_heapDSV.GetDescriptorHandleAtIndex(shadowMap->DsvHeapIndices[i]);
        cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

        const float depthClearValue = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? 1.0f : 0.0f;
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthClearValue, 0, 0, nullptr);

        CbvShadowMap_PerInstance cbv = {};
        XMStoreFloat4x4(&cbv.V, shadowMap->Vs[i]);
        XMStoreFloat4x4(&cbv.P, shadowMap->P);
        cbv.PointLightPosition = position;
        cbv.FarPlane = farPlane;

        renderSceneObjectsToBoundMap(cmdList, scene, cbv);
    }
}

void ShadowMapManager::Render(ID3D12GraphicsCommandList* cmdList, const Heap* heap, Scene* scene, const ShadowMapConfig& config)
{
    GPU_SCOPE(cmdList, "Shadow Maps");

    if (m_matricesDirty)
    {
        m_shadowMapDir.Vs.resize(1);
        m_shadowMapDir.Vs[0] = getViewMatrixDistant(config.DirLightDir, config.DirDist);

        // Projection
        if (config.Width > 0 && config.Height > 0)
            m_shadowMapDir.P = getProjectionMatrixDistant(config.Width, config.Height, config.Near, config.Far);

        for (uint32_t i = 0; i < scene->CPU.MegaBufferPunctualLightsCount; i++)
        {
            const PunctualLight& light = scene->CPU.MegaBufferPunctualLights[i];
            if (light.Intensity <= 0.0f)
                continue;

            if (light.Type == PunctualLightType::eDistant)
            {
                m_shadowMapPunctuals[i].Vs.resize(1);
                m_shadowMapPunctuals[i].Vs[0] = getViewMatrixDistant(light.Position, config.DirDist);

                if (config.Width > 0 && config.Height > 0)
                    m_shadowMapPunctuals[i].P = getProjectionMatrixDistant(config.Width, config.Height, config.Near, config.Far);
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

                if (config.Width > 0 && config.Height > 0)
                    m_shadowMapPunctuals[i].P = getProjectionMatrixPoint(config.Width, config.Height, config.Near, config.Far);
            }
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

        heap->BindSceneTextures_Graphics(cmdList, m_rootSig.GetParamIndexSrvBindless(0));
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
