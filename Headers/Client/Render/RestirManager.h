#ifndef H_RESTIR_MANAGER_H
#define H_RESTIR_MANAGER_H

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

class D3D;

class RestirManager
{
public:
    void Init(ID3D12Device* device, const RootSig* rootSig, const std::vector<std::string>& compileArgs);

    void GenerateSamplesDi(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, bool isTemporalEnabled, bool isSpatialEnabled, bool clear);
    void PostUpdate(D3D* d3d);

    const std::vector<ReservoirDI>& Readback() const { return m_cpuReservoirBufferReadback; }

    static size_t TotalCbvRequiredSize() { return 0; }

    D12Resource* GetReservoirBuffer() { return &m_reservoirBufferRW; };
    [[nodiscard]] size_t GetNumReservoirs() const { return m_numReservoirs; };

private:
    D12Resource m_reservoirBufferRW, m_reservoirBufferReadback;
    size_t m_numReservoirs = 0;

    Pipeline m_pipelineDiGenerateSamples;
    Pipeline m_pipelineDiTemporalMerge;
    Pipeline m_pipelineDiSpatialSample;
    Pipeline m_pipelineDiClear;

    std::vector<ReservoirDI> m_cpuReservoirBufferReadback;
};

#endif