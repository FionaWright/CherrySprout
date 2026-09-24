#ifndef H_GRADIENT_MANAGER_H
#define H_GRADIENT_MANAGER_H

#include "Render/PoissonSolver.h"

struct GreenHouseRenderInfo;

class GradientManager
{
public:
    void Init(const D3D* d3d);
    void LoadSceneData(D3D* d3d, Heap* heap, D12Resource* primal);
    void Update(D3D* d3d, Heap* heap);
    D12Resource* Render(const D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* primal);

    D12Resource* GetGradientXF() { return &m_gradientXF; }
    D12Resource* GetGradientXB() { return &m_gradientXB; }
    D12Resource* GetGradientYF() { return &m_gradientYF; }
    D12Resource* GetGradientYB() { return &m_gradientYB; }

private:
    Pipeline m_pipelineGradientsSS;
    RootSig m_rootSigGradientsSS;
    DescriptorSet m_setGradientsSS;

    D12Resource m_gradientXF, m_gradientXB, m_gradientYF, m_gradientYB;

    PoissonSolver m_poissonSolver;
};

#endif