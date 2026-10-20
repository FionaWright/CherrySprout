#ifndef H_RESTIR_MANAGER_H
#define H_RESTIR_MANAGER_H

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "PathTracing/ReSTIR/Structs.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

class D3D;

class RestirManager
{
public:
    void Init(ID3D12Device* device, const RootSig* rootSig, const std::vector<std::string>& compileArgs);

    void PrePassesDI(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, bool isTemporalEnabled, bool isSpatialEnabled, bool clear);
    void PostUpdate(D3D* d3d);

    const std::vector<ReservoirDI>& Readback() const { return m_cpuReservoirBufferReadback; }

    static size_t TotalCbvRequiredSize() { return 0; }

    D12Resource* GetReservoirBufferDI() { return &m_reservoirBufferDiRW; }
    D12Resource* GetReservoirBufferGI() { return &m_reservoirBufferGi; }
    D12Resource* GetContextsBuffer() { return &m_contextsPrimaryRay; }

    [[nodiscard]] size_t GetNumReservoirs() const { return m_numReservoirs; }
    [[nodiscard]] size_t GetNumContexts() const { return m_numContexts; }

private:
    void initResources(ID3D12Device* device);

    D12Resource m_reservoirBufferDiRW, m_reservoirBufferDiReadback;
    D12Resource m_reservoirBufferGi;
    D12Resource m_contextsPrimaryRay;
    size_t m_numReservoirs = 0;
    size_t m_numContexts = 0;

    Pipeline m_pipelineDiGenerateSamples;
    Pipeline m_pipelineDiTemporalMerge;
    Pipeline m_pipelineDiSpatialSample;
    Pipeline m_pipelineDiClear;

    std::vector<ReservoirDI> m_cpuReservoirBufferReadback;

    bool m_initialisedResources = false;
};

#endif