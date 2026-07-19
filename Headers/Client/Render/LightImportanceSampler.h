#ifndef H_LIGHT_IMPORTANCE_SAMPLER_H
#define H_LIGHT_IMPORTANCE_SAMPLER_H

#include <d3d12.h>

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "HWI/UploadHeap.h"
#include "PathTracing/Structs.h"

struct Scene;
class D3D;

class LightImportanceSampler
{
public:
    void Build(D3D* d3d, Heap* heap, D12Resource* envMap, Scene* scene, bool envMapEnabled);
    [[nodiscard]] bool IsInitialized() const { return m_isInitialized; }

    void MarkSceneDataDirty() { m_sceneDataLoaded = false;}

    [[nodiscard]] D12Resource* GetEnvMapPmf() { return &m_envMapPmf; }
    [[nodiscard]] D12Resource* GetEnvMapCdfConditional() { return &m_envMapCdfConditional; }
    [[nodiscard]] D12Resource* GetEnvMapCdfMarginal() { return &m_envMapCdfMarginal; }
    [[nodiscard]] D12Resource* GetLightsCdf() { return &m_lightCdf; }

    [[nodiscard]] float GetTotalEnvMapLuminance() const { return m_envMapTotalLuminance; }
    [[nodiscard]] float GetPunctualWeight() const { return m_punctualWeight; }

    [[nodiscard]] const std::vector<ProbabilityDistributionSample>& GetCpuLightsCdf() const { return m_cpuLightCdf; }

private:
    void buildEnvMapDistributions(D3D* d3d, Heap* heap, D12Resource* envMap);
    void loadSceneData(D3D* d3d, Heap* heap, Scene* scene, D12Resource* envMap);
    void initializeResources(const D3D* d3d,D12Resource* envMap);

    float m_envMapTotalLuminance = 0;
    float m_punctualWeight = 0;

    bool m_isInitialized = false;
    bool m_sceneDataLoaded = false;

    std::vector<ProbabilityDistributionSample> m_cpuLightCdf;

    UploadHeap m_uploadHeap;

    D12Resource m_envMapSumLumBufferRW;
    D12Resource m_envMapSumLumBufferReadback;
    D12Resource m_envMapPmf;
    D12Resource m_envMapCdfConditional;
    D12Resource m_envMapCdfMarginal;
    D12Resource m_punctualPmfRW;
    D12Resource m_punctualPmfReadback;
    D12Resource m_lightCdf;
    D12Resource m_lightCdfReadback;

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