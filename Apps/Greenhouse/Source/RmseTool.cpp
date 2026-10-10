//
// Created by fiona on 03/02/2026.
//

#include "System/pch.h"
#include "../Headers/RmseTool.h"

#include <iostream>

#include "Greenhouse.h"
#include "HWI/D3D.h"
#include "HWI/Heap.h"
#include "System/FileHelper.h"
#include "Utils/Helper.h"
#include "Utils/Debug/DebugStructs.h"

#include "Debug/GPUEventScoped.h"

#if CHERRY_DEBUG_FEATURES_ENABLED
#include "Debug/PythonExecutor.h"
#include "Debug/Snapshotter.h"
#endif

void RmseTool::Init(const D3D* d3d, Heap* heap)
{
    m_slots[0].Init_Tex2D("Empty", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_slots[1].Init_Tex2D("Empty", d3d->GetDevice(), Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void RmseTool::StoreTextureToSlot(D3D* d3d, Heap* heap, D12Resource* texture, const uint32_t slotIdx, const char* nameSuffix)
{
    if (!m_pipelineBlit.GetPSO())
    {
        m_setBlit.Init(heap);
        m_rootSigBlit.SmartInit(d3d->GetDevice(), 0, 1, 1);
        m_pipelineBlit.InitCompute(d3d->GetDevice(), "Compute/TexBlitCS.hlsl", m_rootSigBlit.Get());
    }

    D12Resource* slot = &m_slots[slotIdx];
    CherryAssert(slot);

    d3d->Flush();
    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();
    {
        GPU_SCOPE(cmdList, "Blit to Slot");

        texture->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        slot->Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        heap->Bind(cmdList);

        cmdList->SetPipelineState(m_pipelineBlit.GetPSO());
        cmdList->SetComputeRootSignature(m_rootSigBlit.Get());

        m_setBlit.SetSRV_Tex2D(d3d->GetDevice(), 0, texture, texture->GetDesc().Format);
        m_setBlit.SetUAV_Tex2D(d3d->GetDevice(), 0, slot, slot->GetDesc().Format);
        m_setBlit.SetDescriptorTables_Compute(cmdList);

        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);
    }
    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();

    std::string newName = texture->GetName();
    if (nameSuffix)
        newName += nameSuffix;
    slot->SetName(newName.c_str());
}

D12Resource* RmseTool::LoadTextureFromSlot(const uint32_t slotIdx) const
{
    return nullptr;
}

void RmseTool::SaveSlotToFile(D3D* d3d, const char* path, const uint32_t slotIdx)
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    D12Resource* slot = &m_slots[slotIdx];
    CherryAssert(slot->GetResource());

    uint8_t* data = nullptr;
    size_t dataSize = 0;
    Snapshotter::ResourceToSnapshot(d3d, slot, data, dataSize);

    ScratchImage scratch;
    const Image* packed = Snapshotter::PackData(d3d, data, slot, scratch);

    // Copy to clipboard
    {
        ScratchImage rgba8;
        Snapshotter::SnapshotToRgba8(packed, rgba8);
        Snapshotter::Rgba8SnapshotToClipboard(rgba8.GetImage(0,0,0));
    }

    Snapshotter::SnapshotToFile(packed, path);
#endif
}

void RmseTool::LoadSlotFromFile(D3D* d3d, Heap* heap, const char* path, const uint32_t slotIdx)
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    const std::string filename = std::filesystem::path(path).filename().string();

    const ScratchImage scratch = Snapshotter::FileToSnapshot(path);
    D12Resource resource = Snapshotter::SnapshotToResource(d3d, scratch, filename.c_str());

    StoreTextureToSlot(d3d, heap, &resource, slotIdx);
#endif
}

std::string RmseTool::GetSlotName(const uint32_t slotIdx) const
{
    std::string name = "Slot " + std::to_string(slotIdx);

    if (m_slots[slotIdx].GetName())
    {
        name += ": (" + std::string(m_slots[slotIdx].GetName()) + ")";
    }

    return name;
}

void RmseTool::CancelOperation()
{
    CherryAssert(m_state != RmseToolState::eIdle);
    TransitionState(RmseToolState::eIdle);
}

void RmseTool::TriggerStoreNextOutput()
{
    CherryAssert(m_state == RmseToolState::eIdle);
    TransitionState(RmseToolState::eStoreNextOutput);
}

void RmseTool::TriggerSaveToFile(const std::string& path)
{
    CherryAssert(m_state == RmseToolState::eIdle);
    TransitionState(RmseToolState::eSaveToFile);

    m_path = path;
}

void RmseTool::TriggerLoadFromFile(const std::string& path)
{
    CherryAssert(m_state == RmseToolState::eIdle);
    TransitionState(RmseToolState::eLoadFromFile);

    m_path = path;
}

void RmseTool::TriggerComputeSingleRMSE()
{
    CherryAssert(m_state == RmseToolState::eIdle);
    TransitionState(RmseToolState::eComputeSingleRMSE);
}

void RmseTool::TriggerComputeConvergence(const uint32_t maxFrames, const uint32_t frameInc)
{
    CherryAssert(m_state == RmseToolState::eIdle);
    TransitionState(RmseToolState::eResetPTForConvergence);

    m_maxFrames = maxFrames;
    m_frameInc = frameInc;
    m_rmses.clear();
}

void RmseTool::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo, uint32_t frameIdx, D12Resource* ptOutput)
{
    if (m_state == RmseToolState::eStoreNextOutput)
    {
        const std::string nameSuffix = " (Frame=" + std::to_string(frameIdx) + ")";
        StoreTextureToSelectedSlot(d3d, renderInfo.Heap, ptOutput, nameSuffix.c_str());
        TransitionState(RmseToolState::eIdle);
        return;
    }

    if (m_state == RmseToolState::eComputeSingleRMSE)
    {
        ComputeSingleRMSE(d3d, renderInfo.Heap);
        TransitionState(RmseToolState::eIdle);
        return;
    }

    if (m_state == RmseToolState::eSaveToFile)
    {
        SaveSelectedSlotToFile(d3d, m_path.c_str());
        TransitionState(RmseToolState::eIdle);
        return;
    }

    if (m_state == RmseToolState::eLoadFromFile)
    {
        LoadSelectedSlotFromFile(d3d, renderInfo.Heap, m_path.c_str());
        TransitionState(RmseToolState::eIdle);
        return;
    }

    if (m_state == RmseToolState::eComputeConvergence)
    {
        if (frameIdx > m_maxFrames)
        {
            TransitionState(RmseToolState::eIdle);
            return;
        }

        const uint32_t nonSelectedSlot = m_selectedSlot == 0 ? 1 : 0;

        const std::string nameSuffix = " (Frame=" + std::to_string(frameIdx) + ")";
        StoreTextureToSlot(d3d, renderInfo.Heap, ptOutput, nonSelectedSlot, nameSuffix.c_str());

        ComputeSingleRMSE(d3d, renderInfo.Heap);

        m_rmses.emplace_back(m_computedRMSE);

        frameIdx += m_frameInc;
        return;
    }

    if (m_state == RmseToolState::eResetPTForConvergence)
    {
        TransitionState(RmseToolState::eComputeConvergence);
        return;
    }
}

void RmseTool::ComputeSingleRMSE(D3D* d3d, Heap* heap)
{
    // 9x9 * 16x16 = 144x144
    constexpr float c_blockSize = 144.0f;

    const float fWidth = static_cast<float>(m_slots[0].GetDesc().Width);
    const float fHeight = static_cast<float>(m_slots[0].GetDesc().Height);
    const float maxDim = std::max(fWidth, fHeight);
    const size_t numThreadGroups1D = std::ceil(maxDim / c_blockSize);

    const size_t bufferNumElements = numThreadGroups1D * numThreadGroups1D;
    const size_t bufferSize = bufferNumElements * sizeof(SumSquaredErrorStruct);

    if (!m_pipelineSumSquaredErr.GetPSO())
    {
        m_rootSigSumSquaredErr.SmartInit(d3d->GetDevice(), 0, 2, 1);

        m_pipelineSumSquaredErr.InitCompute(d3d->GetDevice(), "Compute/SumSquaredErrorCS.hlsl", m_rootSigSumSquaredErr.Get());

        m_bufferSumSqrErrRW.Init_Buffer("SumSquaredErr StructuredBuffer", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_bufferSumSqrErrReadback.Init_Buffer("SumSquaredErr ReadbackBuffer", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_NONE, true);

        m_setSumSquaredErr.Init(heap);
        m_setSumSquaredErr.SetUAV_Buffer(d3d->GetDevice(), 0, &m_bufferSumSqrErrRW, bufferNumElements, sizeof(SumSquaredErrorStruct));
    }

    m_setSumSquaredErr.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_slots[0], m_slots[0].GetDesc().Format);
    m_setSumSquaredErr.SetSRV_Tex2D(d3d->GetDevice(), 1, &m_slots[1], m_slots[1].GetDesc().Format);

    d3d->Flush();

    // Compute Sum of Squared Errors
    {
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();
        {
            GPU_SCOPE(cmdList, "Compute Sum of Squared Errors");

            m_bufferSumSqrErrRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_slots[0].Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            m_slots[1].Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigSumSquaredErr.Get());
            cmdList->SetPipelineState(m_pipelineSumSquaredErr.GetPSO());

            m_setSumSquaredErr.TransitionAllSRVToShaderResource(cmdList);
            m_setSumSquaredErr.SetDescriptorTables_Compute(cmdList);

            cmdList->Dispatch(numThreadGroups1D, numThreadGroups1D, 1);

            m_bufferSumSqrErrRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
            m_bufferSumSqrErrReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
            cmdList->CopyBufferRegion(m_bufferSumSqrErrReadback.GetResource(), 0, m_bufferSumSqrErrRW.GetResource(), 0, bufferSize);
        }
        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }

    std::vector<SumSquaredErrorStruct> readbackData;
    readbackData.resize(bufferNumElements);
    m_bufferSumSqrErrReadback.Readback(readbackData.data());

    // Sum readback
    float totalSum = 0.0f;
    for (int i = 0; i < bufferNumElements; i++)
        totalSum += readbackData[i].SquaredError;

    const float N = m_slots[0].GetDesc().Width * m_slots[0].GetDesc().Height;
    const float mse = totalSum / N;
    m_computedRMSE = sqrt(mse);

    CherryPrint("RMSE: " << m_computedRMSE);
}

void RmseTool::SaveTest(const char* testName) const
{
    if (strcmp(testName, "") == 0)
    {
        CherryPrint("Invalid Test Name!");
        return;
    }

    CherryAssert(m_state == RmseToolState::eIdle);
    CherryAssert(m_rmses.size() > 0);

    std::string csvData = "RMSE\n";
    for (size_t i = 0; i < m_rmses.size(); i++)
    {
        csvData += std::to_string(m_rmses[i]) + "\n";
    }

    const std::string dataFilePath = std::string(BUILD_DIR) + "/Data/Temp/" + std::string(testName) + ".bin";

    std::filesystem::create_directories(std::filesystem::path(dataFilePath).parent_path());

    std::ofstream fs(dataFilePath, std::ios::out | std::ios::binary);
    fs.write(csvData.data(), csvData.size());
    fs.close();
}

void RmseTool::PlotConvergence(const char* testName) const
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    if (strcmp(testName, "") == 0)
    {
        CherryPrint("Invalid Test Name!");
        return;
    }

    CherryAssert(m_state == RmseToolState::eIdle);
    CherryAssert(m_rmses.size() > 0);

    std::string csvData = "RMSE\n";
    for (size_t i = 0; i < m_rmses.size(); i++)
    {
        csvData += std::to_string(m_rmses[i]) + "\n";
    }

    const std::vector<std::string> args = { testName, "--show", "--save" };

    PythonExecutor::ExecutePythonWithData("Plot1D.py", testName, csvData.c_str(), csvData.size(), args);
#endif
}

void RmseTool::PlotMultiConvergence(const std::vector<std::string>& testNames, const bool logPlot)
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    // Plot graph
    {
        std::vector<std::string> args = { "--show", "--save" };
        for (int i = 0; i < testNames.size(); i++)
        {
            const std::string dataFilePath = std::string(BUILD_DIR) + "/Data/Temp/" + std::string(testNames[i]) + ".bin";
            args.emplace_back(dataFilePath);
        }

        if (logPlot)
            args.push_back("--log");

        PythonExecutor::ExecutePython("PlotMultiConvergence.py", args);
    }
#endif
}