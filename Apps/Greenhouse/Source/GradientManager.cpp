#include "System/pch.h"
#include "GradientManager.h"

#include "Greenhouse.h"
#include "PathTracing/Flags/MethodsCpp.h"

void GradientManager::Init(const D3D* d3d)
{
    // TODO: Better system than 4 textures. Compress? Separate CS pass that computes both in one?

    // TODO: Not being lazy loaded

    m_gradientXF.Release();
    m_gradientXB.Release();
    m_gradientYF.Release();
    m_gradientYB.Release();

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // TODO: Rgba16f?
    desc.Width = Config::GetSystem().RtvWidth;
    desc.Height = Config::GetSystem().RtvHeight;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    m_gradientXF.Init("GradientX (Forward)", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
    m_gradientXB.Init("GradientX (Backward)", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
    m_gradientYF.Init("GradientY (Forward)", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
    m_gradientYB.Init("GradientY (Backward)", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
}

void GradientManager::LoadSceneData(D3D* d3d, Heap* heap, D12Resource* primal)
{
    m_poissonSolver.SetupDescriptorSets(d3d, heap, primal, &m_gradientXF, &m_gradientXB, &m_gradientYF, &m_gradientYB);
}

void GradientManager::Update(D3D* d3d, Heap* heap)
{
    m_poissonSolver.Prepare(d3d, heap);
}

D12Resource* GradientManager::Render(const D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* primal)
{
    // Screen-Space Gradients
    if (renderInfo.PathTracerConfig->FeatureEnabled(eFeature_ScreenSpaceGradients))
    {
        if (!m_rootSigGradientsSS.Get())
        {
            m_setGradientsSS.Init(renderInfo.Heap);
            m_setGradientsSS.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_gradientXF, m_gradientXF.GetDesc().Format);
            m_setGradientsSS.SetUAV_Tex2D(d3d->GetDevice(), 1, &m_gradientXB, m_gradientXB.GetDesc().Format);
            m_setGradientsSS.SetUAV_Tex2D(d3d->GetDevice(), 2, &m_gradientYF, m_gradientYF.GetDesc().Format);
            m_setGradientsSS.SetUAV_Tex2D(d3d->GetDevice(), 3, &m_gradientYB, m_gradientYB.GetDesc().Format);

            m_rootSigGradientsSS.SmartInit(d3d->GetDevice(), 0, 1, 4);
            m_pipelineGradientsSS.InitCompute(d3d->GetDevice(), "Compute/GradientDomain/TexGradientsCS.hlsl", m_rootSigGradientsSS.Get());
        }

        primal->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_gradientXF.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_gradientXB.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_gradientYF.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_gradientYB.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        cmdList->SetPipelineState(m_pipelineGradientsSS.GetPSO());
        cmdList->SetComputeRootSignature(m_rootSigGradientsSS.Get());

        m_setGradientsSS.SetSRV_Tex2D(d3d->GetDevice(), 0, primal, primal->GetDesc().Format);
        m_setGradientsSS.SetDescriptorTables_Compute(cmdList);

        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        if (renderInfo.PathTracerConfig->DebugEnabled(eDebug_OutputColor))
        {
            if (renderInfo.PathTracerConfig->DebugInfo.OutputColorIdx == DebugOutputIndex::eDebugOutput_GD_GradientXF)
                return &m_gradientXF;
            if (renderInfo.PathTracerConfig->DebugInfo.OutputColorIdx == DebugOutputIndex::eDebugOutput_GD_GradientYF)
                return &m_gradientYF;
            if (renderInfo.PathTracerConfig->DebugInfo.OutputColorIdx == DebugOutputIndex::eDebugOutput_GD_GradientXB)
                return &m_gradientXB;
            if (renderInfo.PathTracerConfig->DebugInfo.OutputColorIdx == DebugOutputIndex::eDebugOutput_GD_GradientYB)
                return &m_gradientYB;
        }
    }

    if (renderInfo.PathTracerConfig->DisplayGradientX || renderInfo.PathTracerConfig->DisplayGradientY)
    {
        if (!m_rootSigGradientsCombine.Get())
        {
            m_debugGradientX.Init_Tex2D("Gradient X (Debug)", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            m_debugGradientY.Init_Tex2D("Gradient Y (Debug)", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            m_setGradientsCombine.Init(renderInfo.Heap);
            m_setGradientsCombine.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_gradientXF, m_gradientXF.GetDesc().Format);
            m_setGradientsCombine.SetSRV_Tex2D(d3d->GetDevice(), 1, &m_gradientXB, m_gradientXB.GetDesc().Format);
            m_setGradientsCombine.SetSRV_Tex2D(d3d->GetDevice(), 2, &m_gradientYF, m_gradientYF.GetDesc().Format);
            m_setGradientsCombine.SetSRV_Tex2D(d3d->GetDevice(), 3, &m_gradientYB, m_gradientYB.GetDesc().Format);
            m_setGradientsCombine.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_debugGradientX, m_debugGradientX.GetDesc().Format);
            m_setGradientsCombine.SetUAV_Tex2D(d3d->GetDevice(), 1, &m_debugGradientY, m_debugGradientY.GetDesc().Format);

            m_rootSigGradientsCombine.SmartInit(d3d->GetDevice(), 0, 4, 2);
            m_pipelineGradientsCombine.InitCompute(d3d->GetDevice(), "Compute/GradientDomain/CombineGradientsCS.hlsl", m_rootSigGradientsCombine.Get());
        }

        m_gradientXF.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_gradientXB.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_gradientYF.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_gradientYB.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_debugGradientX.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_debugGradientY.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        cmdList->SetPipelineState(m_pipelineGradientsCombine.GetPSO());
        cmdList->SetComputeRootSignature(m_rootSigGradientsCombine.Get());

        m_setGradientsCombine.SetDescriptorTables_Compute(cmdList);

        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        if (renderInfo.PathTracerConfig->DisplayGradientX)
            return &m_debugGradientX;
        else
            return &m_debugGradientY;
    }

    // Poisson Solving
    if (!renderInfo.PathTracerConfig->PoissonReconstructionEnabled)
        return primal;

    m_gradientXF.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    m_gradientXB.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    m_gradientYF.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    m_gradientYB.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

    return m_poissonSolver.Solve(cmdList,
                                renderInfo.Heap,
                                primal,
                                renderInfo.PathTracerConfig->SprNumIterations,
                                renderInfo.PathTracerConfig->SprAlpha);
}
