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

    void GenerateSamplesDi(ID3D12GraphicsCommandList* cmdList);

    static size_t TotalCbvRequiredSize() { return 0; }

    D12Resource* GetReservoirBuffer() { return &m_reservoirBuffer; };
    [[nodiscard]] size_t GetNumReservoirs() const { return m_numReservoirs; };

private:
    D12Resource m_reservoirBuffer;
    size_t m_numReservoirs = 0;

    Pipeline m_pipelineDiGenerateSamples;
};

#endif