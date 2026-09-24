#ifndef H_POISSON_SOLVER_H
#define H_POISSON_SOLVER_H

#include "HWI/D3D.h"
#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "HWI/UploadHeap.h"

class PoissonSolver
{
public:
    void Prepare(D3D* d3d, Heap* heap);
    void SetupDescriptorSets(D3D* d3d, Heap* heap, D12Resource* primal, D12Resource* gradientXF, D12Resource* gradientXB, D12Resource* gradientYF, D12Resource
                             * gradientYB);
    D12Resource* Solve(ID3D12GraphicsCommandList* cmdList, const Heap* heap, D12Resource* primal, uint32_t numIterations, float alpha, float
                       jacobiCoefficient);

private:
    void initResources(D3D* d3d, Heap* heap);

    UploadHeap m_uploadHeap;

    D12Resource m_gradientTerms, m_pingPong0, m_pingPong1, m_black;
    RootSig m_rootSigJacobi;
    DescriptorSet m_setJacobi0to1, m_setJacobi1to0;
    Pipeline m_pipelineJacobi, m_pipelineJacobiPre;
    RootConstants m_rootConstantsJacobi;

    bool m_isInitialized = false;

};

#endif