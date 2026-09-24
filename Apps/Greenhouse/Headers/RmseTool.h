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
    eStoreAtMaxFrames,
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

    void SaveSlotToFile(const char* path, uint32_t slotIdx);
    void SaveSelectedSlotToFile(const char* path) { SaveSlotToFile(path, m_selectedSlot); }

    void LoadSlotFromFile(const char* path, uint32_t slotIdx);
    void LoadSelectedSlotFromFile(const char* path) { LoadSlotFromFile(path, m_selectedSlot); }

    RmseToolState GetCurrentState() const { return m_state; }
    void TransitionState(const RmseToolState newState) { m_state = newState; }

    uint32_t GetSelectedSlot() const { return m_selectedSlot; }
    void SetSelectedSlot(const uint32_t slotIdx) { m_selectedSlot = slotIdx; }

    std::string GetSlotName(uint32_t slotIdx) const;

    void TriggerStoreNextOutput();
    void TriggerStoreAtMaxFrames(uint32_t maxFrames);
    void TriggerComputeSingleRMSE();
    void TriggerComputeConvergence();
    void TriggerPlotConvergence();

    void PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo, uint32_t frameIdx, D12Resource* ptOutput);

    void ComputeSingleRMSE(D3D* d3d, Heap* heap);
    [[nodiscard]] float GetComputedRMSE() const { return m_computedRMSE; }

    //void UpdateComputeGolden(D3D* d3d, uint32_t currFrame, D12Resource* finalRTV);

    //void SaveGolden(const uint8_t* data, size_t bufferSize, int width, int height) const;

    //void PrepareLoadGolden(const char* path);
    //[[nodiscard]] bool NeedLoadGolden() const { return m_loadGoldenNextFrame; }
    //void LoadGolden(D3D* d3d, uint32_t slot);

    //void BeginConvergenceTest(uint32_t maxFrames, const char* testName, uint32_t frameInc, bool plotAndShow);
    //void UpdateConvergenceTest(D3D* d3d, const uint32_t currFrame, Heap* heap, D12Resource* finalRTV);
    //[[nodiscard]] float GetConvergenceTestPercent() const { return m_lastFrameConvergenceTested / static_cast<float>(m_maxFrames);}

    void CompareTests(const std::vector<std::string>& testNames, bool logPlot);

private:
    D12Resource m_slots[2] = {};
    uint32_t m_selectedSlot = 0;

    RmseToolState m_state = RmseToolState::eIdle;

    float m_computedRMSE = NAN;

    bool m_plotAndShow = false;

    uint32_t m_maxFrames = 0;
    const char* m_taskName = nullptr;
    //ReadbackBuffer m_goldenReadbackBuffer;

    uint32_t m_frameIncrement = 0;
    uint32_t m_lastFrameConvergenceTested = 0;
    std::vector<float> m_rmses;

    RootSig m_rootSigSumSquaredErr;
    Pipeline m_pipelineSumSquaredErr;
    DescriptorSet m_setSumSquaredErr;
    D12Resource m_bufferSumSqrErrRW;
    D12Resource m_bufferSumSqrErrReadback;

    Pipeline m_pipelineBlit;
    RootSig m_rootSigBlit;
    DescriptorSet m_setBlit;
};


#endif // H_RMSETESTER_H