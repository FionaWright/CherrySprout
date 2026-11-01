#ifndef H_RESTIR_MANAGER_H
#define H_RESTIR_MANAGER_H

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "Utils/D3DUtils.h"

class D3D;

struct RestirConfig
{
    bool ClearBuffers;

    bool EnabledDI;
    bool EnabledGI;
    bool EnabledPT;
    bool EnabledTemporal;
    bool EnabledSpatial;
};

class RestirManager
{
public:
    void Init(ID3D12Device* device, const RootSig* rootSig, const std::vector<std::string>& compileArgs);

    void PrePasses(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const RestirConfig& config);
    void PostUpdate(D3D* d3d);

    static size_t TotalCbvRequiredSize() { return 0; }

    D12Resource* GetReservoirBufferDI() { return &m_reservoirBufferDi; }
    D12Resource* GetReservoirBufferGI() { return &m_reservoirBufferGi; }
    D12Resource* GetReservoirBufferPT() { return &m_reservoirBufferPt; }
    D12Resource* GetContextsBuffer() { return &m_contextsPrimaryRay; }

    [[nodiscard]] size_t GetNumReservoirs() const { return m_numReservoirs; }
    [[nodiscard]] size_t GetNumContexts() const { return m_numContexts; }

private:
    void initResources(ID3D12Device* device);

    D12Resource m_reservoirBufferDi;
    D12Resource m_reservoirBufferGi;
    D12Resource m_reservoirBufferPt;
    D12Resource m_contextsPrimaryRay;
    size_t m_numReservoirs = 0;
    size_t m_numContexts = 0;

    Pipeline m_pipelineInitContext;
    Pipeline m_pipelineClear;

    Pipeline m_pipelineDiGen;
    Pipeline m_pipelineDiTemporal;
    Pipeline m_pipelineDiSpatio;

    Pipeline m_pipelineGiGen;
    Pipeline m_pipelineGiTemporal;
    Pipeline m_pipelineGiSpatio;

    Pipeline m_pipelinePtGen;
    Pipeline m_pipelinePtTemporal;
    Pipeline m_pipelinePtSpatio;

    bool m_initialisedResources = false;
};

#endif