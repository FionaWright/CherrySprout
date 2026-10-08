#ifndef H_LIGHT_IMPORTANCE_SAMPLER_H
#define H_LIGHT_IMPORTANCE_SAMPLER_H

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "HWI/UploadHeap.h"
#include "PathTracing/Structs.h"
#include "../../../Assets/Shaders/PathTracing/NEE/LSD/Alias.h"

struct Scene;
class D3D;

#define ALIAS_TABLE_BUILD_MAX_STACK_SIZE 200
#define ALIAS_TABLE_BUILD_MAX_STACK_SIZE_DEFINE (std::string("-DMAX_STACK_SIZE=") + std::to_string(ALIAS_TABLE_BUILD_MAX_STACK_SIZE))

struct LsdConfig
{
    float EnvMapScale = 1.0f;
    float EmissiveScale = 1.0f;
    float PunctualScale = 1.0f;
    bool UseAliasTables = false;
};

class LightImportanceSampler
{
public:
    void Build(D3D* d3d, Heap* heap, D12Resource* envMap, Scene* scene, const LsdConfig& config);
    [[nodiscard]] bool IsInitialized() const { return m_isInitialized; }

    void MarkSceneDataDirty() { m_sceneDataLoaded = false;}

    [[nodiscard]] D12Resource* GetEnvMapPdf() { return &m_envMapPdf; }
    [[nodiscard]] D12Resource* GetEnvMapCdfConditional() { return &m_envMapCdfConditional; }
    [[nodiscard]] D12Resource* GetEnvMapCdfMarginal() { return &m_envMapCdfMarginal; }
    [[nodiscard]] D12Resource* GetEmissiveInstanceMap() { return &m_emissiveInstanceMap; }
    [[nodiscard]] D12Resource* GetLightsCdf() { return &m_lsdRW; }

    [[nodiscard]] float GetEnvMapWeight() const { return m_envMapWeight; }
    [[nodiscard]] float GetPunctualWeight() const { return m_punctualWeight; }
    [[nodiscard]] float GetEmissiveWeight() const { return m_emissiveWeight; }

    [[nodiscard]] uint32_t GetLsdBasePunctuals() const;
    [[nodiscard]] uint32_t GetLsdBaseEmissives() const;
    [[nodiscard]] uint32_t GetLsdCount() const;

    [[nodiscard]] bool IsLsdAliasTable() const { return m_lsdIsAliasTable; }
    [[nodiscard]] uint32_t GetEmissiveInstanceCount() const { return m_numEmissiveInstances; }

#if CHERRY_DEBUG_FEATURES_ENABLED
    [[nodiscard]] const std::vector<ProbabilityDistributionSample>& GetCpuLightsCdf() const { return m_cpuLsdCdf; }
    [[nodiscard]] const std::vector<AliasEntry>& GetCpuLightsAlias() const { return m_cpuLsdAlias; }
#endif

private:
    void buildPunctualDistributions(D3D* d3d, const Heap* heap, Scene* scene);
    void buildEmissiveDistributions(D3D* d3d, const Heap* heap, Scene* scene);
    void buildEnvMapDistributions(D3D* d3d, const Heap* heap, const D12Resource* envMap);
    void loadSceneData(D3D* d3d, Heap* heap, Scene* scene, D12Resource* envMap, bool aliasTablesEnabled);
    void initializeResources(const D3D* d3d, const D12Resource* envMap);

    float m_envMapWeight = 0;
    float m_punctualWeight = 0;
    float m_emissiveWeight = 0;

    uint32_t m_numPunctuals = 0;
    uint32_t m_numEmissiveInstances = 0;
    uint32_t m_maxPunctuals = 0;
    uint32_t m_maxEmissives = 0;
    uint32_t m_maxLsdCount = 0;

    bool m_isInitialized = false;
    bool m_sceneDataLoaded = false;
    bool m_lsdIsAliasTable = false;

    UploadHeap m_uploadHeap;

    D12Resource m_envMapSumLumBufferRW;
    D12Resource m_envMapSumLumBufferReadback;
    D12Resource m_envMapPdf;
    D12Resource m_envMapCdfConditional;
    D12Resource m_envMapCdfMarginal;

    D12Resource m_punctualPdfRW;
    D12Resource m_punctualPdfReadback;

    D12Resource m_emissiveInstanceMap;
    D12Resource m_emissivePdfRW;
    D12Resource m_emissivePdfReadback;

    D12Resource m_lsdRW;
    D12Resource m_lsdReadback;

    RootSig m_rootSigSrvUav;
    RootSig m_rootSigSrv5Uav;
    RootSig m_rootSigCbvSrvUav;
    RootSig m_rootSigUav;
    RootSig m_rootSigCbvUav3;

    DescriptorSet m_setEnvMapSumLum;
    DescriptorSet m_setEnvMapPdf;
    DescriptorSet m_setEnvMapCdfConditional;
    DescriptorSet m_setEnvMapCdfMarginal;
    DescriptorSet m_setEnvMapCdfConditionalNormalize;
    DescriptorSet m_setCdfNormalize1D;
    DescriptorSet m_setPunctualPdf;
    DescriptorSet m_setEmissivePdf;
    DescriptorSet m_setLSD;

    Pipeline m_pipelineEnvMapSumLum;
    Pipeline m_pipelineEnvMapPdf;
    Pipeline m_pipelineEnvMapCdfConditional;
    Pipeline m_pipelineEnvMapCdfMarginal;
    Pipeline m_pipelineEnvMapCdfConditionalNormalize;
    Pipeline m_pipelineCdfNormalize1D;
    Pipeline m_pipelinePunctualPdf;
    Pipeline m_pipelineLsdCdf;
    Pipeline m_pipelineLsdAlias;
    Pipeline m_pipelineEmissivePdf;

    size_t m_sumLumGroupsX = 0, m_sumLumGroupsY = 0;
    size_t m_sumLumBufferNumElements = 0, m_sumLumBufferSize = 0;
    size_t m_punctualLightsPdfBufferSize = 0;
    size_t m_lsdBufferSize = 0;

#if CHERRY_DEBUG_FEATURES_ENABLED
    std::vector<ProbabilityDistributionSample> m_cpuLsdCdf;
    std::vector<AliasEntry> m_cpuLsdAlias;
#endif
};

#endif