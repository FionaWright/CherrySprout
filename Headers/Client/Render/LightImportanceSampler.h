#ifndef H_LIGHT_IMPORTANCE_SAMPLER_H
#define H_LIGHT_IMPORTANCE_SAMPLER_H

#include <d3d12.h>

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"

struct Scene;
class D3D;

class LightImportanceSampler
{
public:
    void Build(D3D* d3d, Heap* heap, D12Resource* envMap, Scene* scene);
    bool IsInitialized() const { return m_envMapCdfMarginal.IsInitialized(); }

    D12Resource* GetEnvMapPmf() { return &m_envMapPmf; }
    D12Resource* GetEnvMapCdfConditional() { return &m_envMapCdfConditional; }
    D12Resource* GetEnvMapCdfMarginal() { return &m_envMapCdfMarginal; }

private:
    void buildEnvMapDistributions(D3D* d3d, Heap* heap, D12Resource* envMap);
    void initializeResources(const D3D* d3d, Heap* heap, const D12Resource* envMap, const Scene* scene);

    float m_envMapTotalLuminance = 0;
    float m_punctualTotalLuminance = 0;

    D12Resource m_envMapSumLumBufferRW;
    D12Resource m_envMapSumLumBufferReadback;
    D12Resource m_envMapPmf;
    D12Resource m_envMapCdfConditional;
    D12Resource m_envMapCdfMarginal;
    D12Resource m_punctualPmfRW;
    D12Resource m_punctualPmfReadback;
    D12Resource m_lightCdf;

    RootSig m_rootSigSrvUav;
    RootSig m_rootSigCbvSrvUav;
    RootSig m_rootSigUav;
    RootSig m_rootSigCbvUav2;

    DescriptorSet m_setEnvMapSumLum;
    DescriptorSet m_setEnvMapPmf;
    DescriptorSet m_setEnvMapCdfConditional;
    DescriptorSet m_setEnvMapCdfMarginal;
    DescriptorSet m_setEnvMapCdfConditionalNormalize;
    DescriptorSet m_setCdfNormalize1D;
    DescriptorSet m_setPunctualPmf;
    DescriptorSet m_setLightCdf;

    Pipeline m_pipelineEnvMapSumLum;
    Pipeline m_pipelineEnvMapPmf;
    Pipeline m_pipelineEnvMapCdfConditional;
    Pipeline m_pipelineEnvMapCdfMarginal;
    Pipeline m_pipelineEnvMapCdfConditionalNormalize;
    Pipeline m_pipelineCdfNormalize1D;
    Pipeline m_pipelinePunctualPmf;
    Pipeline m_pipelineLightCdf;

    size_t m_sumLumGroupsX = 0, m_sumLumGroupsY = 0;
    size_t m_sumLumBufferNumElements = 0, m_sumLumBufferSize = 0;
    size_t m_punctualLightsPmfBufferSize = 0;
    size_t m_lightsCdfBufferSize = 0;
};

#endif