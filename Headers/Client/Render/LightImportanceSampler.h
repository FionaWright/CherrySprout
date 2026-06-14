#ifndef H_LIGHT_IMPORTANCE_SAMPLER_H
#define H_LIGHT_IMPORTANCE_SAMPLER_H

#include <d3d12.h>

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"

class D3D;

class LightImportanceSampler
{
public:
    void BuildEnvMapDistributions(D3D* d3d, ID3D12GraphicsCommandList* cmdList, D12Resource* envMap);

private:
    void initializeResources(const D3D* d3d, Heap* heap, const D12Resource* envMap);

    D12Resource m_envMapSumLumBufferRW;
    D12Resource m_envMapSumLumBufferReadback;
    D12Resource m_envMapPmf;
    D12Resource m_envMapCdfConditional;
    D12Resource m_envMapCdfMarginal;

    RootSig m_rootSigSrvUav, m_rootSigPmf;
    DescriptorSet m_setSumLum, m_setPmf, m_setCdfConditional, m_setCdfMarginal;
    Pipeline m_pipelineSumLum, m_pipelinePmf, m_pipelineCdfConditional, m_pipelineCdfMarginal;
};

#endif