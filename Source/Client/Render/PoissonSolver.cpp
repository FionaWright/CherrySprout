#include "System/pch.h"
#include "Render/PoissonSolver.h"

#include "HWI/Heap.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

void PoissonSolver::Prepare(D3D* d3d, Heap* heap)
{
    if (!m_isInitialized)
    {
        initResources(d3d, heap);
        m_isInitialized = true;
    }
}

void PoissonSolver::SetupDescriptorSets(D3D* d3d,
                            Heap* heap,
                            D12Resource* primal,
                            D12Resource* gradientX,
                            D12Resource* gradientY)
{
    if (!m_isInitialized)
    {
        initResources(d3d, heap);
        m_isInitialized = true;
    }

    m_setJacobi0to1.Init(heap, false, true);
    m_setJacobi1to0.Init(heap, false, true);

    m_setJacobi0to1.SetSRV_Tex2D(d3d->GetDevice(), 0, primal, primal->GetDesc().Format);
    m_setJacobi0to1.SetSRV_Tex2D(d3d->GetDevice(), 1, gradientX, gradientX->GetDesc().Format);
    m_setJacobi0to1.SetSRV_Tex2D(d3d->GetDevice(), 2, gradientY, gradientY->GetDesc().Format);
    m_setJacobi0to1.SetSRV_Tex2D(d3d->GetDevice(), 3, &m_pingPong0, m_pingPong0.GetDesc().Format);
    m_setJacobi0to1.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_gradientTerms, m_gradientTerms.GetDesc().Format);
    m_setJacobi0to1.SetUAV_Tex2D(d3d->GetDevice(), 1, &m_pingPong1, m_pingPong1.GetDesc().Format);

    m_setJacobi1to0.SetSRV_Tex2D(d3d->GetDevice(), 0, primal, primal->GetDesc().Format);
    m_setJacobi1to0.SetSRV_Tex2D(d3d->GetDevice(), 1, gradientX, gradientX->GetDesc().Format);
    m_setJacobi1to0.SetSRV_Tex2D(d3d->GetDevice(), 2, gradientY, gradientY->GetDesc().Format);
    m_setJacobi1to0.SetSRV_Tex2D(d3d->GetDevice(), 3, &m_pingPong1, m_pingPong1.GetDesc().Format);
    m_setJacobi1to0.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_gradientTerms, m_gradientTerms.GetDesc().Format);
    m_setJacobi1to0.SetUAV_Tex2D(d3d->GetDevice(), 1, &m_pingPong0, m_pingPong0.GetDesc().Format);
}

D12Resource* PoissonSolver::Solve(const D3D* d3d,
                          ID3D12GraphicsCommandList* cmdList,
                          const Heap* heap,
                          D12Resource* primal,
                          D12Resource* gradientX,
                          D12Resource* gradientY,
                          const uint32_t numIterations,
                          const float alpha,
                          const float jacobiCoefficient)
{
    if (numIterations == 0)
        return primal;

    // TODO: Use previous frame pingpong as initial estimate?

    // Setup
    {
        heap->Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigJacobi.Get());

        CbvSprJacobi cbv;
        cbv.Alpha = alpha;
        cbv.JacobiCoefficient = jacobiCoefficient;
        cbv.PrimalWidth = primal->GetDesc().Width;
        cbv.PrimalHeight = primal->GetDesc().Height;
        m_rootConstantsJacobi.Bind_Compute(cmdList, &cbv, 0);
    }

    // Compute gradient terms
    {
        cmdList->SetPipelineState(m_pipelineJacobiPre.GetPSO());

        primal->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gradientX->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gradientY->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pingPong0.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        m_gradientTerms.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_pingPong1.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_setJacobi0to1.SetDescriptorTables_Compute(cmdList);
        m_setJacobi0to1.TransitionAllSRVToShaderResource(cmdList);

        DispatchOverTexture(cmdList, 16, m_gradientTerms.GetDesc().Width, m_gradientTerms.GetDesc().Height);
    }

    // Solve
    {
        cmdList->SetPipelineState(m_pipelineJacobi.GetPSO());

        m_gradientTerms.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        for (int i = 0; i < numIterations; i++)
        {
            D12Resource& prev = i % 2 == 0 ? m_pingPong0 : m_pingPong1;
            D12Resource& next = i % 2 == 0 ? m_pingPong1 : m_pingPong0;
            DescriptorSet& set = i % 2 == 0 ? m_setJacobi0to1 : m_setJacobi1to0;

            if (i == 0)
            {
                D12Resource* initial = primal;
                //D12Resource* initial = &m_black;
                initial->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
                prev.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
                prev.CopyTextureInto(cmdList, initial->GetResource());
            }

            primal->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            prev.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            next.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            set.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 16, next.GetDesc().Width, next.GetDesc().Height);

            next.UavBarrier(cmdList);
        }
    }

    return numIterations % 2 == 0 ? &m_pingPong1 : &m_pingPong0;
}

void PoissonSolver::initResources(D3D* d3d, Heap* heap)
{
    d3d->Flush();

    const uint32_t w = Config::GetSystem().RtvWidth;
    const uint32_t h = Config::GetSystem().RtvHeight;

    m_gradientTerms.Init_Tex2D("Poisson Gradient Terms", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pingPong0.Init_Tex2D("Poisson PingPong 0", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pingPong1.Init_Tex2D("Poisson PingPong 1", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_black.Init_Tex2D("TEMP BLACK", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    m_rootConstantsJacobi.Init(0, 0, sizeof(CbvSprJacobi));
    m_rootSigJacobi.SmartInit(d3d->GetDevice(), 0, 4, 2, false, nullptr, 0, &m_rootConstantsJacobi);

    m_pipelineJacobiPre.InitCompute(d3d->GetDevice(), "Compute/PoissonSolvers/JacobiPreCS.hlsl", m_rootSigJacobi.Get());
    m_pipelineJacobi.InitCompute(d3d->GetDevice(), "Compute/PoissonSolvers/JacobiCS.hlsl", m_rootSigJacobi.Get());
}
