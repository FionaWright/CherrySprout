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

    constexpr size_t numDescriptors = 10000; // TODO: Handle this properly
    constexpr size_t numSceneTextureDescriptors = 5000;
    m_heap.Init("Main Heap", d3d->GetDevice(), numDescriptors, numSceneTextureDescriptors,
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_uploadHeapCBV.Init(d3d->GetDevice(), maxCbvRequiredSize); // TODO: Test without extra

    m_aspectRatio = static_cast<float>(Config::GetSystem().RtvWidth) / static_cast<float>(Config::GetSystem().
        RtvHeight);
    m_P = XMMatrixPerspectiveFovLH(XMConvertToRadians(Config::GetRender().FoV), m_aspectRatio,
                                   Config::GetRender().NearPlane, Config::GetRender().FarPlane);
    m_InvP = XMMatrixInverse(nullptr, m_P);

    Scene scene;

    scene.CPU.MegaBufferVertexCount = 24;
    scene.CPU.MegaBufferVertex = new Vertex[scene.CPU.MegaBufferVertexCount];

    // Front (+Z)
    scene.CPU.MegaBufferVertex[0] = {.Position = {-1, -1, 1}, .Normal = {0, 0, 1}, .UV = {0, 1}};
    scene.CPU.MegaBufferVertex[1] = {.Position = {-1, 1, 1}, .Normal = {0, 0, 1}, .UV = {0, 0}};
    scene.CPU.MegaBufferVertex[2] = {.Position = {1, 1, 1}, .Normal = {0, 0, 1}, .UV = {1, 0}};
    scene.CPU.MegaBufferVertex[3] = {.Position = {1, -1, 1}, .Normal = {0, 0, 1}, .UV = {1, 1}};

    // Back (-Z)
    scene.CPU.MegaBufferVertex[4] = {.Position = {1, -1, -1}, .Normal = {0, 0, -1}, .UV = {0, 1}};
    scene.CPU.MegaBufferVertex[5] = {.Position = {1, 1, -1}, .Normal = {0, 0, -1}, .UV = {0, 0}};
    scene.CPU.MegaBufferVertex[6] = {.Position = {-1, 1, -1}, .Normal = {0, 0, -1}, .UV = {1, 0}};
    scene.CPU.MegaBufferVertex[7] = {.Position = {-1, -1, -1}, .Normal = {0, 0, -1}, .UV = {1, 1}};

    // Left (-X)
    scene.CPU.MegaBufferVertex[8] = {.Position = {-1, -1, -1}, .Normal = {-1, 0, 0}, .UV = {0, 1}};
    scene.CPU.MegaBufferVertex[9] = {.Position = {-1, 1, -1}, .Normal = {-1, 0, 0}, .UV = {0, 0}};
    scene.CPU.MegaBufferVertex[10] = {.Position = {-1, 1, 1}, .Normal = {-1, 0, 0}, .UV = {1, 0}};
    scene.CPU.MegaBufferVertex[11] = {.Position = {-1, -1, 1}, .Normal = {-1, 0, 0}, .UV = {1, 1}};

    // Right (+X)
    scene.CPU.MegaBufferVertex[12] = {.Position = {1, -1, 1}, .Normal = {1, 0, 0}, .UV = {0, 1}};
    scene.CPU.MegaBufferVertex[13] = {.Position = {1, 1, 1}, .Normal = {1, 0, 0}, .UV = {0, 0}};
    scene.CPU.MegaBufferVertex[14] = {.Position = {1, 1, -1}, .Normal = {1, 0, 0}, .UV = {1, 0}};
    scene.CPU.MegaBufferVertex[15] = {.Position = {1, -1, -1}, .Normal = {1, 0, 0}, .UV = {1, 1}};

    // Top (+Y)
    scene.CPU.MegaBufferVertex[16] = {.Position = {-1, 1, 1}, .Normal = {0, 1, 0}, .UV = {0, 1}};
    scene.CPU.MegaBufferVertex[17] = {.Position = {-1, 1, -1}, .Normal = {0, 1, 0}, .UV = {0, 0}};
    scene.CPU.MegaBufferVertex[18] = {.Position = {1, 1, -1}, .Normal = {0, 1, 0}, .UV = {1, 0}};
    scene.CPU.MegaBufferVertex[19] = {.Position = {1, 1, 1}, .Normal = {0, 1, 0}, .UV = {1, 1}};

    // Bottom (-Y)
    scene.CPU.MegaBufferVertex[20] = {.Position = {-1, -1, -1}, .Normal = {0, -1, 0}, .UV = {0, 1}};
    scene.CPU.MegaBufferVertex[21] = {.Position = {-1, -1, 1}, .Normal = {0, -1, 0}, .UV = {0, 0}};
    scene.CPU.MegaBufferVertex[22] = {.Position = {1, -1, 1}, .Normal = {0, -1, 0}, .UV = {1, 0}};
    scene.CPU.MegaBufferVertex[23] = {.Position = {1, -1, -1}, .Normal = {0, -1, 0}, .UV = {1, 1}};

    scene.CPU.MegaBufferIndexCount = 36;
    scene.CPU.MegaBufferIndex = new uint32_t[scene.CPU.MegaBufferIndexCount];

    // Front
    scene.CPU.MegaBufferIndex[0] = 0;
    scene.CPU.MegaBufferIndex[1] = 2;
    scene.CPU.MegaBufferIndex[2] = 1;
    scene.CPU.MegaBufferIndex[3] = 0;
    scene.CPU.MegaBufferIndex[4] = 3;
    scene.CPU.MegaBufferIndex[5] = 2;

    // Back
    scene.CPU.MegaBufferIndex[6] = 4;
    scene.CPU.MegaBufferIndex[7] = 6;
    scene.CPU.MegaBufferIndex[8] = 5;
    scene.CPU.MegaBufferIndex[9] = 4;
    scene.CPU.MegaBufferIndex[10] = 7;
    scene.CPU.MegaBufferIndex[11] = 6;

    // Left
    scene.CPU.MegaBufferIndex[12] = 8;
    scene.CPU.MegaBufferIndex[13] = 10;
    scene.CPU.MegaBufferIndex[14] = 9;
    scene.CPU.MegaBufferIndex[15] = 8;
    scene.CPU.MegaBufferIndex[16] = 11;
    scene.CPU.MegaBufferIndex[17] = 10;

    // Right
    scene.CPU.MegaBufferIndex[18] = 12;
    scene.CPU.MegaBufferIndex[19] = 14;
    scene.CPU.MegaBufferIndex[20] = 13;
    scene.CPU.MegaBufferIndex[21] = 12;
    scene.CPU.MegaBufferIndex[22] = 15;
    scene.CPU.MegaBufferIndex[23] = 14;

    // Top
    scene.CPU.MegaBufferIndex[24] = 16;
    scene.CPU.MegaBufferIndex[25] = 18;
    scene.CPU.MegaBufferIndex[26] = 17;
    scene.CPU.MegaBufferIndex[27] = 16;
    scene.CPU.MegaBufferIndex[28] = 19;
    scene.CPU.MegaBufferIndex[29] = 18;

    // Bottom
    scene.CPU.MegaBufferIndex[30] = 20;
    scene.CPU.MegaBufferIndex[31] = 22;
    scene.CPU.MegaBufferIndex[32] = 21;
    scene.CPU.MegaBufferIndex[33] = 20;
    scene.CPU.MegaBufferIndex[34] = 23;
    scene.CPU.MegaBufferIndex[35] = 22;

    scene.CPU.MegaBufferMaterialsCount = 1;
    scene.CPU.MegaBufferMaterials = new Material();
    scene.CPU.MegaBufferMaterials->Albedo = XMFLOAT4(1, 0, 0, 1);
    scene.CPU.MegaBufferMaterials->TexIdxAlbedo = 0;

    XMMATRIX TRS = XMMatrixIdentity();
    XMFLOAT4X4 TRS4x4{};
    XMStoreFloat4x4(&TRS4x4, TRS);

    scene.CPU.ObjectCount = 1;
    scene.CPU.Objects = new Object();
    scene.CPU.Objects->MaterialIndex = 0;
    memcpy(scene.CPU.Objects->M, &TRS4x4, sizeof(XMFLOAT4X4));
    scene.CPU.Objects->MegaBufferVertexCount = scene.CPU.MegaBufferVertexCount;
    scene.CPU.Objects->MegaBufferVertexOffset = 0;
    scene.CPU.Objects->MegaBufferIndexCount = scene.CPU.MegaBufferIndexCount;
    scene.CPU.Objects->MegaBufferIndexOffset = 0;

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
    m_sceneManager.UploadScene(d3d);
    m_sceneManager.AddSceneTexturesToHeap(d3d, &m_heap);

    {
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        InitializeSamplerLinearClamp(&sampler);

        m_rootConstants.Init(0, 0, sizeof(CbvForward_PerInstance));

        m_rootSig.SmartInit(d3d->GetDevice(), 2, 1, 0, true, &sampler, 1, &m_rootConstants);

        m_descriptorSet.Init(&m_heap, true, true);
        m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvMatrices_VP), &m_uploadHeapCBV);
        m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvForward), &m_uploadHeapCBV);
        m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 0, &m_sceneManager.GetGPU().MegaBufferMaterials, m_sceneManager.GetCPU().MegaBufferMaterialsCount, sizeof(Material));

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
    cbvForward.DirLightDir = XMFLOAT3(0,-1,0);
    cbvForward.MaxCubemapMipMaps = 1;
    cbvForward.OutputMode = 0;
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

    ImGui::TextUnformatted("Lab2");

    Gui::EndWindow();
}

void Lab2::OnResize(uint32_t width, uint32_t height)
{

}
