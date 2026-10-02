//
// Created by fiona on 01/10/2026.
//

#include "System/pch.h"
#include "Apps/TCD/Lab2/Headers/Lab2.h"

#include "imgui.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "Scene/Material.h"
#include "Scene/Scene.h"
#include "Scene/SceneCPU.h"
#include "System/FileHelper.h"
#include "System/Gui.h"
#include "System/HighResolutionClock.h"
#include "Utils/Helper.h"

void CreateCube(XMFLOAT3 position, XMFLOAT3 scale, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<Material>& materials, std::vector<Object>& objects)
{
    Object obj;
    obj.MegaBufferVertexOffset = 0;
    obj.MegaBufferIndexOffset = 0;
    obj.MegaBufferVertexCount = 24;
    obj.MegaBufferIndexCount = 36;

    Material mat;
    mat.Albedo = XMFLOAT4(1, 0, 0, 1);
    mat.TexIdxAlbedo = std::rand() % 6;
    materials.emplace_back(mat);

    obj.MaterialIndex = materials.size();

    const XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
    const XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
    const XMMATRIX TRS = S * T;
    XMFLOAT4X4 TRS4x4{};
    XMStoreFloat4x4(&TRS4x4, TRS);

    memcpy(obj.M, &TRS4x4, sizeof(XMFLOAT4X4));
    objects.push_back(obj);
}

float Rand01()
{
    return (static_cast<float>(std::rand()) / (RAND_MAX)) + 1;
}

void Lab2::Init(D3D* d3d)
{
    App::Init(d3d);

    size_t maxCbvRequiredSize = 0;
    {
        maxCbvRequiredSize += Align(sizeof(CbvMatrices_VP), 256);
        maxCbvRequiredSize += Align(sizeof(CbvForward), 256);
        maxCbvRequiredSize += Align(sizeof(Material), 256);
        maxCbvRequiredSize += EnvironmentMap::GetCbvRequiredSize();
        maxCbvRequiredSize += Skybox::GetCbvRequiredSize();
    }

    constexpr size_t numDescriptors = 10000;
    constexpr size_t numSceneTextureDescriptors = 5000;
    m_heap.Init("Main Heap", d3d->GetDevice(), numDescriptors, numSceneTextureDescriptors,
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_uploadHeapCBV.Init(d3d->GetDevice(), maxCbvRequiredSize);

    m_aspectRatio = static_cast<float>(Config::GetSystem().RtvWidth) / static_cast<float>(Config::GetSystem().RtvHeight);
    m_P = XMMatrixPerspectiveFovLH(XMConvertToRadians(Config::GetRender().FoV), m_aspectRatio,
                                   Config::GetRender().NearPlane, Config::GetRender().FarPlane);
    m_InvP = XMMatrixInverse(nullptr, m_P);

    Scene scene;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Material> materials;
    std::vector<Object> objects;

    // Front (+Z)
    vertices.emplace_back(Vertex({-0.5f, -0.5f, 0.5f}, {0, 0, 1}, {0, 1}));
    vertices.emplace_back(Vertex({-0.5f, 0.5f, 0.5f}, {0, 0, 1}, {0, 0}));
    vertices.emplace_back(Vertex({0.5f, 0.5f, 0.5f}, {0, 0, 1}, {1, 0}));
    vertices.emplace_back(Vertex({0.5f, -0.5f, 0.5f}, {0, 0, 1}, {1, 1}));

    // Back (-Z)
    vertices.emplace_back(Vertex({0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 1}));
    vertices.emplace_back(Vertex({0.5f, 0.5f, -0.5f}, {0, 0, -1}, {0, 0}));
    vertices.emplace_back(Vertex({-0.5f, 0.5f, -0.5f}, {0, 0, -1}, {1, 0}));
    vertices.emplace_back(Vertex({-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 1}));

    // Left (-X)
    vertices.emplace_back(Vertex({-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 1}));
    vertices.emplace_back(Vertex({-0.5f, 0.5f, -0.5f}, {-1, 0, 0}, {0, 0}));
    vertices.emplace_back(Vertex({-0.5f, 0.5f, 0.5f}, {-1, 0, 0}, {1, 0}));
    vertices.emplace_back(Vertex({-0.5f, -0.5f, 0.5f}, {-1, 0, 0}, {1, 1}));

    // Right (+X)
    vertices.emplace_back(Vertex({0.5f, -0.5f, 0.5f}, {1, 0, 0}, {0, 1}));
    vertices.emplace_back(Vertex({0.5f, 0.5f, 0.5f}, {1, 0, 0}, {0, 0}));
    vertices.emplace_back(Vertex({0.5f, 0.5f, -0.5f}, {1, 0, 0}, {1, 0}));
    vertices.emplace_back(Vertex({0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 1}));

    // Top (+Y)
    vertices.emplace_back(Vertex({-0.5f, 0.5f, 0.5f}, {0, 1, 0}, {0, 1}));
    vertices.emplace_back(Vertex({-0.5f, 0.5f, -0.5f}, {0, 1, 0}, {0, 0}));
    vertices.emplace_back(Vertex({0.5f, 0.5f, -0.5f}, {0, 1, 0}, {1, 0}));
    vertices.emplace_back(Vertex({0.5f, 0.5f, 0.5f}, {0, 1, 0}, {1, 1}));

    // Bottom (-Y)
    vertices.emplace_back(Vertex({-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 1}));
    vertices.emplace_back(Vertex({-0.5f, -0.5f, 0.5f}, {0, -1, 0}, {0, 0}));
    vertices.emplace_back(Vertex({0.5f, -0.5f, 0.5f}, {0, -1, 0}, {1, 0}));
    vertices.emplace_back(Vertex({0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 1}));

    // Front
    indices.emplace_back(0);
    indices.emplace_back(2);
    indices.emplace_back(1);
    indices.emplace_back(0);
    indices.emplace_back(3);
    indices.emplace_back(2);

    // Back
    indices.emplace_back(4);
    indices.emplace_back(6);
    indices.emplace_back(5);
    indices.emplace_back(4);
    indices.emplace_back(7);
    indices.emplace_back(6);

    // Left
    indices.emplace_back(8);
    indices.emplace_back(10);
    indices.emplace_back(9);
    indices.emplace_back(8);
    indices.emplace_back(11);
    indices.emplace_back(10);

    // Right
    indices.emplace_back(12);
    indices.emplace_back(14);
    indices.emplace_back(13);
    indices.emplace_back(12);
    indices.emplace_back(15);
    indices.emplace_back(14);

    // Top
    indices.emplace_back(16);
    indices.emplace_back(18);
    indices.emplace_back(17);
    indices.emplace_back(16);
    indices.emplace_back(19);
    indices.emplace_back(18);

    // Bottom
    indices.emplace_back(20);
    indices.emplace_back(22);
    indices.emplace_back(21);
    indices.emplace_back(20);
    indices.emplace_back(23);
    indices.emplace_back(22);

    constexpr int NUM_ROWS = 20;
    constexpr int NUM_COLS = 20;
    constexpr float SPACING = 2;
    constexpr float SPACING_VARIANCE = 0.1f;
    constexpr float BASE_HEIGHT = 5;
    constexpr float HEIGHT_VARIANCE = 10.0f;
    constexpr bool NORMAL_SHAPE = true;

    for (int x = -NUM_ROWS; x < NUM_ROWS; x++)
        for (int z = -NUM_COLS; z < NUM_COLS; z++)
        {
            float normalMult = 1.0f;
            if (NORMAL_SHAPE)
            {
                float r2 = x * x + z * z;
                normalMult = 1.0f / pow(r2, 0.2f);
            }

            float height = BASE_HEIGHT + (Rand01() * HEIGHT_VARIANCE * normalMult);
            m_scales.emplace_back(height);

            float xPos = x * SPACING + ((Rand01() - 0.5f) * 2.0f * SPACING_VARIANCE);
            float zPos = z * SPACING + ((Rand01() - 0.5f) * 2.0f * SPACING_VARIANCE);

            CreateCube(XMFLOAT3(xPos, (height - 1)/2.0f, zPos), XMFLOAT3(1,height,1), vertices, indices, materials, objects);
        }

    scene.CPU.MegaBufferVertexCount = vertices.size();
    scene.CPU.MegaBufferVertex = new Vertex[vertices.size()];
    memcpy(scene.CPU.MegaBufferVertex, vertices.data(), vertices.size() * sizeof(Vertex));

    scene.CPU.MegaBufferIndexCount = indices.size();
    scene.CPU.MegaBufferIndex = new uint32_t[indices.size()];
    memcpy(scene.CPU.MegaBufferIndex, indices.data(), indices.size() * sizeof(uint32_t));

    scene.CPU.MegaBufferMaterialsCount = materials.size();
    scene.CPU.MegaBufferMaterials = new Material[materials.size()];
    memcpy(scene.CPU.MegaBufferMaterials, materials.data(), materials.size() * sizeof(Material));

    scene.CPU.ObjectCount = objects.size();
    scene.CPU.Objects = new Object[objects.size()];
    memcpy(scene.CPU.Objects, objects.data(), objects.size() * sizeof(Object));

    scene.CPU.MegaBufferPunctualLightsCount = 1;
    scene.CPU.MegaBufferPunctualLights = new PunctualLight(); // Dummy

    scene.CPU.TextureFilepathCount = 6;
    scene.CPU.TextureFilepaths = new char*[scene.CPU.TextureFilepathCount];
    scene.CPU.TextureFilepaths[0] = _strdup(FileHelper::GetAssetTextureFullPath("TCD/facade0.jpg").c_str());
    scene.CPU.TextureFilepaths[1] = _strdup(FileHelper::GetAssetTextureFullPath("TCD/facade1.jpg").c_str());
    scene.CPU.TextureFilepaths[2] = _strdup(FileHelper::GetAssetTextureFullPath("TCD/facade2.jpg").c_str());
    scene.CPU.TextureFilepaths[3] = _strdup(FileHelper::GetAssetTextureFullPath("TCD/facade3.jpg").c_str());
    scene.CPU.TextureFilepaths[4] = _strdup(FileHelper::GetAssetTextureFullPath("TCD/facade4.jpg").c_str());
    scene.CPU.TextureFilepaths[5] = _strdup(FileHelper::GetAssetTextureFullPath("TCD/facade5.jpg").c_str());

    m_sceneManager.AssignScene(scene);
    m_sceneManager.UploadScene(d3d, false);
    m_sceneManager.GenerateMipMaps(d3d, &m_heap);
    m_sceneManager.AddSceneTexturesToHeap(d3d, &m_heap);

    m_rtasBuilder.FlushAndBuild(d3d, &m_sceneManager.GetScene());

    {
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        InitializeSamplerLinearWrap(&sampler);

        m_rootConstants.Init(0, 0, sizeof(CbvForward_PerInstance));

        m_rootSig.SmartInit(d3d->GetDevice(), 2, 2, 0, true, &sampler, 1, &m_rootConstants);

        m_descriptorSet.Init(&m_heap, true, true);
        m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvMatrices_VP), &m_uploadHeapCBV);
        m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvForward), &m_uploadHeapCBV);
        m_descriptorSet.SetSRV_RTAS(d3d->GetDevice(), 0, m_rtasBuilder.GetRtasResource());
        m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 1, &m_sceneManager.GetGPU().MegaBufferMaterials, m_sceneManager.GetCPU().MegaBufferMaterialsCount, sizeof(Material));

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

        auto desc = CreateGraphicsPipelineDesc(m_rootSig.Get(), {ildDesc, _countof(ildDesc)}, true);
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

        m_pipeline.InitGraphics(d3d->GetDevice(), "Raster/TCD/Facade.hlsl", "Raster/TCD/Facade.hlsl", desc);

        m_cameraController.Init(XMFLOAT3(0,0,-5), 0, 0);
    }
}

void Lab2::Update(D3D* d3d, TimeArgs timeArgs)
{
    if (m_envMapDirty)
    {
        m_envMap.Init(d3d, &m_heap, "autumn_field_puresky_4k.hdr", 0);

        m_envMap.InitCubemap(d3d, &m_heap);
        m_skybox.Init(d3d, m_envMap.GetCubemap());
        m_skybox.UpdateDescriptorSet(d3d->GetDevice(), m_envMap.GetCubemap(), &m_heap, &m_uploadHeapCBV);

        m_envMapDirty = false;
    }

    if (m_cameraController.UpdateCamera(timeArgs.ElapsedTime_ms / 1000.0f) || m_cameraDirty)
    {
        m_V = m_cameraController.GetViewMatrix();
        m_InvV = XMMatrixInverse(nullptr, m_V);
        m_cameraDirty = false;
    }
}

void Lab2::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    GPU_SCOPE(cmdList, "Forward Backend");

    Scene* scene = &m_sceneManager.GetScene();

    {
        SetViewportScissor(cmdList, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, Config::GetSystem().WindowAppGuiWidth);

        d3d->GetRtv()->Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d->GetRtvHandle();
        const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(d3d->GetDsvHeapStart(), 0, d3d->GetDsvDescriptorSize());
        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    }

    m_heap.Bind(cmdList);

    {
        GPU_SCOPE(cmdList, "Skybox Pass");
        m_skybox.RenderForward(cmdList, &m_V, &m_P);
    }

    {
        D3D12_VERTEX_BUFFER_VIEW viewV;
        D3D12_INDEX_BUFFER_VIEW viewI;
        VertexIndexBuffersToViews(&scene->GPU.MegaBufferVertex, &scene->GPU.MegaBufferIndex,
                                  scene->CPU.MegaBufferVertexCount, sizeof(Vertex), scene->CPU.MegaBufferIndexCount,
                                  viewV, viewI);

        scene->GPU.MegaBufferVertex.Transition(cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        scene->GPU.MegaBufferIndex.Transition(cmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        m_descriptorSet.TransitionAllSRVToShaderResource(cmdList);

        cmdList->IASetVertexBuffers(0, 1, &viewV);
        cmdList->IASetIndexBuffer(&viewI);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmdList->SetGraphicsRootSignature(m_rootSig.Get());
        cmdList->SetPipelineState(m_pipeline.GetPSO());

        m_heap.BindSceneTextures_Graphics(cmdList, m_rootSig.GetParamIndexSceneTextures());
    }

    CbvMatrices_VP matricesVP = {};
    XMStoreFloat4x4(&matricesVP.V, m_V);
    XMStoreFloat4x4(&matricesVP.P, m_P);
    m_descriptorSet.UpdateCBV(0, &matricesVP);

    CbvForward cbvForward = {};
    cbvForward.DirLightDir = m_dirLightDir;
    cbvForward.MaxCubemapMipMaps = 1;
    cbvForward.OutputMode = 0;
    cbvForward.CameraPosition = m_cameraController.GetCamera().GetPosition();
    m_descriptorSet.UpdateCBV(1, &cbvForward);

    m_descriptorSet.SetDescriptorTables_Graphics(cmdList);

    CbvForward_PerInstance pushConstants = {};

    for (int i = 0; i < scene->CPU.ObjectCount; ++i)
    {
        const Object& obj = scene->CPU.Objects[i];

        const XMMATRIX M = XMMatrixSet(
            obj.M[0], obj.M[1], obj.M[2], obj.M[3],
            obj.M[4], obj.M[5], obj.M[6], obj.M[7],
            obj.M[8], obj.M[9], obj.M[10], obj.M[11],
            obj.M[12], obj.M[13], obj.M[14], obj.M[15]
        );

        XMStoreFloat4x4(&pushConstants.M, M);
        XMStoreFloat4x4(&pushConstants.MTI, XMMatrixTranspose(XMMatrixInverse(nullptr, M)));
        pushConstants.MaterialIdx = obj.MaterialIndex;
        pushConstants.ScaleY = m_scales[i];
        m_rootConstants.Bind_Graphics(cmdList, &pushConstants);

        cmdList->DrawIndexedInstanced(obj.MegaBufferIndexCount, 1, obj.MegaBufferIndexOffset,
                                      obj.MegaBufferVertexOffset, 0);
    }
}

void Lab2::PostUpdate(D3D* d3d)
{
}

void Lab2::RenderGUI()
{
    Gui::BeginWindow("Lab2", ImVec2(0, 0), ImVec2(Config::GetSystem().WindowAppGuiWidth, Config::GetSystem().RtvHeight));

    ImGui::InputFloat3("Dir Light Dir", reinterpret_cast<float*>(&m_dirLightDir));

    Gui::EndWindow();
}

void Lab2::OnResize(uint32_t width, uint32_t height)
{

}
