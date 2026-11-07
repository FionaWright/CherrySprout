#include "System/pch.h"
#include "Apps/Lab3/Headers/Lab3.h"

#include <numeric>
#include <Render/ShadowMapManager.h>

#include "imgui.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "Raster/Utils/ShadowMapPuncInfo.h"
#include "Scene/Material.h"
#include "Scene/Scene.h"
#include "Scene/SceneCPU.h"
#include "System/FileHelper.h"
#include "System/Gui.h"
#include "System/HighResolutionClock.h"
#include "System/TextureLoader.h"
#include "Utils/Constants.h"
#include "Utils/Helper.h"

struct SceneConfig
{
    std::string Name;
    std::string Filepath;

    XMFLOAT3 CameraPosition{0, 0, 0};
    XMFLOAT2 CameraPitchYaw{0, PI};
    float SceneScale = 1.0f;
};

void Lab3::Init(D3D* d3d)
{
    App::Init(d3d);

    // Heaps, Matrices, etc
    {
        size_t maxCbvRequiredSize = 0;
        {
            maxCbvRequiredSize += Align(sizeof(CbvMatrices_VP), 256);
            maxCbvRequiredSize += Align(sizeof(CbvForward), 256);
            maxCbvRequiredSize += Align(sizeof(CbvShadowMapSample), 256);
            maxCbvRequiredSize += Align(sizeof(Material), 256);
            maxCbvRequiredSize += Skybox::GetCbvRequiredSize();
            maxCbvRequiredSize += ShadowMapManager::GetCbvRequiredSize();
        }

        constexpr size_t numDescriptors = 10000;
        constexpr size_t numSceneTextureDescriptors = 5000;
        m_heap.Init("Main Heap", d3d->GetDevice(), numDescriptors, numSceneTextureDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_uploadHeapCBV.Init(d3d->GetDevice(), maxCbvRequiredSize);

        m_aspectRatio = static_cast<float>(Config::GetSystem().RtvWidth) / static_cast<float>(Config::GetSystem().RtvHeight);
        m_P = Config::ComputeProjectionMatrix();
        m_InvP = XMMatrixInverse(nullptr, m_P);

        m_cameraController.Init(XMFLOAT3(0,0,-5), 0, 0);
    }

    // Scene
    {
        SceneConfig sceneConfig = {
            .Name = "Chess",
            .Filepath = "Scenes/USD/OpenChessSet/chess_set.usda",
            .CameraPosition = {-0.114393, 0.159540, -0.291603},
            .CameraPitchYaw = {0.251301, 0.375632},
        };
        m_sceneManager.LoadScene(sceneConfig.Filepath.c_str(), sceneConfig.SceneScale);
        m_sceneManager.UploadScene(d3d, true);
        m_sceneManager.GenerateMipMaps(d3d, &m_heap);
        m_sceneManager.AddSceneTexturesToHeap(d3d, &m_heap);
    }

    // RTAS
    if (d3d->GetRayTracingSupported())
    {
        m_rtasBuilder.FlushAndBuild(d3d, &m_sceneManager.GetScene());
    }

    // Skybox
    {
        m_envMap.Init(d3d, &m_heap, L"autumn_field_puresky_4k.hdr", 0);
        m_envMap.InitCubemap(d3d, &m_heap);
        m_skybox.Init(d3d, CUBEMAP_RESO, CUBEMAP_RESO);
        m_skybox.UpdateDescriptorSet(d3d->GetDevice(), m_envMap.GetCubemap(), &m_heap, &m_uploadHeapCBV);
        //m_skybox.GenerateIrradianceMap(d3d, &m_heap);
    }

    // Shadow Maps
    {
        m_shadowMapManager.Init(d3d->GetDevice(), &m_heap, &m_uploadHeapCBV);
        m_shadowMapManager.GeneratePoissonSamples(16);
    }

    // BRDF Integration Map
    {
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
    }

    // Pipeline
    {
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

        m_set.Init(&m_heap);
        m_set.SetHasRootConstants();
        m_set.AddBindlessSrv(1);
        m_set.AddCBV(d3d->GetDevice(), 0, sizeof(CbvMatrices_VP), &m_uploadHeapCBV);
        m_set.AddCBV(d3d->GetDevice(), 0, sizeof(CbvForward), &m_uploadHeapCBV);
        m_set.AddCBV(d3d->GetDevice(), 0, sizeof(CbvShadowMapSample), &m_uploadHeapCBV);

        m_shadowMapManager.LoadSceneData(d3d, &m_heap, &m_sceneManager.GetScene());

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.Format = m_envMap.GetCubemap()->GetDesc().Format;
        srvDesc.TextureCube.MipLevels = m_envMap.GetCubemap()->GetDesc().MipLevels;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_set.SetSRV_Buffer (d3d->GetDevice(), 0, 0, &m_sceneManager.GetScene().GPU.MegaBufferMaterials, m_sceneManager.GetScene().CPU.MegaBufferMaterialsCount, sizeof(Material));
        m_set.SetSRV_Buffer (d3d->GetDevice(), 0, 1, &m_sceneManager.GetScene().GPU.MegaBufferPunctualLights, m_sceneManager.GetScene().CPU.MegaBufferPunctualLightsCount, sizeof(PunctualLight));
        m_set.SetSRV_Buffer (d3d->GetDevice(), 0, 2, m_shadowMapManager.GetShadowMapPuncInfo(), m_sceneManager.GetScene().CPU.MegaBufferPunctualLightsCount, sizeof(ShadowMapPuncInfo));
        m_set.SetSRV_Tex2D  (d3d->GetDevice(), 0, 3, &m_brdfIntMap, m_brdfIntMap.GetDesc().Format);
        m_set.SetSRV        (d3d->GetDevice(), 0, 4, m_envMap.GetCubemap(), srvDesc);
        m_set.SetSRV_Tex2D  (d3d->GetDevice(), 0, 5, m_shadowMapManager.GetShadowMapDir(), SHADOW_MAP_FORMAT_DEPTH_SRV);

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

        std::vector<std::string> compileArgs;
        if (Config::GetRender().DepthBufferType == DepthBufferType::eReversed)
            compileArgs.emplace_back("-DUSING_REVERSED_Z");

        auto desc = CreateGraphicsPipelineDesc(m_rootSig.Get(), { ildDesc, _countof(ildDesc) }, true);
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        V(m_pipeline.InitGraphics(d3d->GetDevice(), "Raster/Forward.hlsl", "Raster/Forward.hlsl", desc, compileArgs));
    }

    for (uint32_t i = 0; i < m_sceneManager.GetScene().CPU.ObjectCount; i++)
    {
        const float* M = m_sceneManager.GetScene().CPU.Objects[i].M;
        const XMFLOAT3 pos = XMFLOAT3(M[4], M[8], M[12]);
        m_positions.emplace_back(pos);
    }
}

void Lab3::Update(D3D* d3d, TimeArgs timeArgs)
{
    if (m_cameraController.UpdateCamera(timeArgs.ElapsedTime_ms / 1000.0f) || m_cameraDirty)
    {
        m_V = m_cameraController.GetViewMatrix();
        m_InvV = XMMatrixInverse(nullptr, m_V);
        m_cameraDirty = false;
    }
}

void Lab3::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    GPU_SCOPE(cmdList, "Forward Backend");

    // Shadow Map
    {
        ShadowMapConfig config;
        config.Width = 1.0f;
        config.Height = 1.0f;
        config.CascadeCount = 1;
        config.Near = 0.01f;
        config.Far = 100.0f;
        config.DirDist = 80.0f;
        config.DirLightDir = m_dirLightDir;
        m_shadowMapManager.Render(d3d, cmdList, &m_heap, &m_sceneManager.GetScene(), config);
    }

    {
        SetViewportScissor(cmdList, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, Config::GetSystem().WindowAppGuiWidth);

        d3d->GetRtv()->Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d->GetRtvHandle();
        const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(d3d->GetDsvHeapStart(), 0, d3d->GetDsvDescriptorSize());
        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        const float depthClearValue = Config::GetRender().DepthBufferType == DepthBufferType::eForward ? 1.0f : 0.0f;
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthClearValue, 0, 0, nullptr);
    }

    m_heap.Bind(cmdList);

    {
        GPU_SCOPE(cmdList, "Skybox Pass");
        m_skybox.RenderForward(cmdList, &m_V, &m_P);
    }

    {
        D3D12_VERTEX_BUFFER_VIEW viewV;
        D3D12_INDEX_BUFFER_VIEW viewI;
        VertexIndexBuffersToViews(&m_sceneManager.GetScene().GPU.MegaBufferVertex, &m_sceneManager.GetScene().GPU.MegaBufferIndex,
                                  m_sceneManager.GetScene().CPU.MegaBufferVertexCount, sizeof(Vertex), m_sceneManager.GetScene().CPU.MegaBufferIndexCount,
                                  viewV, viewI);

        m_sceneManager.GetScene().GPU.MegaBufferVertex.Transition(cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        m_sceneManager.GetScene().GPU.MegaBufferIndex.Transition(cmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        m_set.TransitionAllSRVToShaderResource(cmdList);

        cmdList->IASetVertexBuffers(0, 1, &viewV);
        cmdList->IASetIndexBuffer(&viewI);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmdList->SetGraphicsRootSignature(m_rootSig.Get());
        cmdList->SetPipelineState(m_pipeline.GetPSO());

        m_heap.BindSceneTextures_Graphics(cmdList, m_rootSig.GetParamIndexSrvBindless(0));
        if (m_shadowMapManager.GetHeapBasePoint() != UINT_MAX)
            m_heap.BindBindless_Graphics(cmdList, m_shadowMapManager.GetHeapBasePoint(), m_rootSig.GetParamIndexSrvBindless(1));
    }

    XMFLOAT3 cameraPos = m_cameraController.GetCamera().GetPosition();

    CbvMatrices_VP matricesVP = {};
    XMStoreFloat4x4(&matricesVP.V, m_V);
    XMStoreFloat4x4(&matricesVP.P, m_P);
    m_set.UpdateCBV(0, 0, &matricesVP);

    CbvForward cbvForward{};
    cbvForward.CameraPosition = cameraPos;
    cbvForward.FrameIdx = m_frameIdx;
    cbvForward.DirLightDir = m_dirLightDir;
    cbvForward.MaxCubemapMipMaps = m_envMap.GetCubemap()->GetDesc().MipLevels;
    cbvForward.Flags = ForwardFlags::eNone;
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::eEnvironmentMap, true);
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::ePunctualLights, true);
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::eDirectionalLight, true);
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::eShadowMaps, true);
    ForwardFlagSet(cbvForward.Flags, ForwardFlags::eAmbient, true);
    m_set.UpdateCBV(0, 1, &cbvForward);

    CbvShadowMapSample cbvShadow{};
    XMStoreFloat4x4(&cbvShadow.ShadowMatrices[0], m_shadowMapManager.GetShadowMatrix(0));
    XMFLOAT2 dim = m_shadowMapManager.GetShadowDimensions(0);
    cbvShadow.TexelSize = XMFLOAT2(1.0f / dim.x, 1.0f / dim.y);
    cbvShadow.BiasDepthDistant = 0.000001f;
    cbvShadow.BiasDepthPoint = 0.001f;
    cbvShadow.CascadeCount = 1;
    cbvShadow.NormalBiasDistant = 0.01f;
    cbvShadow.NormalBiasPoint = 0.001f;
    cbvShadow.PcfSampleCount = 32;
    cbvShadow.PcfRadius = 1.5f;
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

    if (m_renderOrder.size() == 0)
    {
        m_renderOrder.resize(m_sceneManager.GetScene().CPU.ObjectCount);
        std::iota(m_renderOrder.begin(), m_renderOrder.end(), 0);
    }

    std::ranges::sort(m_renderOrder,
                      [&](const int a, const int b)
                      {
                          const XMFLOAT3& pa = m_positions[a];
                          const XMFLOAT3& pb = m_positions[b];

                          const XMFLOAT3 da = {
                              pa.x - cameraPos.x,
                              pa.y - cameraPos.y,
                              pa.z - cameraPos.z
                          };

                          const XMFLOAT3 db = {
                              pb.x - cameraPos.x,
                              pb.y - cameraPos.y,
                              pb.z - cameraPos.z
                          };

                          const float distA = da.x * da.x + da.y * da.y + da.z * da.z;
                          const float distB = db.x * db.x + db.y * db.y + db.z * db.z;

                          return distA < distB;
                      });

    CbvForward_PerInstance pushConstants = {};

    for (int i : m_renderOrder)
    {
        const Object& obj = m_sceneManager.GetScene().CPU.Objects[i];

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

void Lab3::PostUpdate(D3D* d3d)
{
    m_frameIdx++;
}

void Lab3::RenderGUI()
{
    Gui::BeginWindow("Lab3", ImVec2(0, 0), ImVec2(Config::GetSystem().WindowAppGuiWidth, Config::GetSystem().RtvHeight));

    ImGui::InputFloat3("Dir Light Dir", reinterpret_cast<float*>(&m_dirLightDir));

    Gui::EndWindow();
}

void Lab3::OnResize(uint32_t width, uint32_t height)
{

}