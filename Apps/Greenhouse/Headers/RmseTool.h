//
// Created by fiona on 03/02/2026.
//

#ifndef H_RMSETESTER_H
#define H_RMSETESTER_H

#include <cstdint>
#include <vector>

#include "HWI/D12Resource.h"
#include "HWI/Heap.h"
#include "HWI/D3D.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"

struct GreenHouseRenderInfo;
class D3D;

enum class RmseToolState
{
    eIdle,
    eStoreNextOutput,
    eSaveToFile,
    eLoadFromFile,
    eComputeSingleRMSE,
    eComputeConvergence,
    ePlotConvergence,
};

class RmseTool
{
public:
    void Init(const D3D* d3d, Heap* heap);

    void StoreTextureToSlot(D3D* d3d, Heap* heap, D12Resource* texture, uint32_t slotIdx, const char* nameSuffix = nullptr);
    void StoreTextureToSelectedSlot(D3D* d3d, Heap* heap, D12Resource* texture, const char* nameSuffix = nullptr) { StoreTextureToSlot(d3d, heap, texture, m_selectedSlot, nameSuffix); }

    D12Resource* LoadTextureFromSlot(uint32_t slotIdx) const;
    D12Resource* LoadTextureFromSelectedSlot() const { return LoadTextureFromSlot(m_selectedSlot); }

    void SaveSlotToFile(D3D* d3d, const char* path, uint32_t slotIdx);
    void SaveSelectedSlotToFile(D3D* d3d, const char* path) { SaveSlotToFile(d3d, path, m_selectedSlot); }

    void LoadSlotFromFile(D3D* d3d, Heap* heap, const char* path, uint32_t slotIdx);
    void LoadSelectedSlotFromFile(D3D* d3d, Heap* heap, const char* path) { LoadSlotFromFile(d3d, heap, path, m_selectedSlot); }

    RmseToolState GetCurrentState() const { return m_state; }
    void TransitionState(const RmseToolState newState) { m_state = newState; }

    uint32_t GetSelectedSlot() const { return m_selectedSlot; }
    void SetSelectedSlot(const uint32_t slotIdx) { m_selectedSlot = slotIdx; }

    std::string GetSlotName(uint32_t slotIdx) const;

    void TriggerStoreNextOutput();
    void TriggerSaveToFile(const std::string& path);
    void TriggerLoadFromFile(const std::string& path);
    void TriggerComputeSingleRMSE();
    void TriggerComputeConvergence(uint32_t maxFrames, uint32_t frameInc);
    void TriggerPlotConvergence(const std::string& testName);

    void PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo, uint32_t frameIdx, D12Resource* ptOutput);

    void ComputeSingleRMSE(D3D* d3d, Heap* heap);
    [[nodiscard]] float GetComputedRMSE() const { return m_computedRMSE; }

    void BeginConvergenceTest(uint32_t maxFrames, const char* testName, uint32_t frameInc, bool plotAndShow);
    void UpdateConvergenceTest(D3D* d3d, const uint32_t currFrame, Heap* heap, D12Resource* finalRTV);

    //void CompareTests(const std::vector<std::string>& testNames, bool logPlot);

private:
    D12Resource m_slots[2] = {};
    uint32_t m_selectedSlot = 0;

    RmseToolState m_state = RmseToolState::eIdle;

    float m_computedRMSE = NAN;
    std::vector<float> m_rmses;

    std::string m_path = "";
    uint32_t m_maxFrames = 0;
    uint32_t m_frameInc = 0;

    // ===

    D12Resource m_bufferSumSqrErrRW;
    D12Resource m_bufferSumSqrErrReadback;

    Pipeline m_pipelineSumSquaredErr;
    RootSig m_rootSigSumSquaredErr;
    DescriptorSet m_setSumSquaredErr;

    Pipeline m_pipelineBlit;
    RootSig m_rootSigBlit;
    DescriptorSet m_setBlit;
};


#endif // H_RMSETESTER_H