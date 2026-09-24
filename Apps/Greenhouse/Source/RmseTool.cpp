//
// Created by fiona on 03/02/2026.
//

#include "System/pch.h"
#include "../Headers/RmseTool.h"

#include <iostream>

#include "Greenhouse.h"
#include "Debug/GPUEventScoped.h"
#include "Debug/PythonExecutor.h"
#include "HWI/D3D.h"
#include "HWI/Heap.h"
#include "System/FileHelper.h"
#include "Utils/Helper.h"
#include "Utils/Debug/DebugStructs.h"

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

void RmseTool::SaveSlotToFile(const char* path, uint32_t slotIdx)
{
}

void RmseTool::LoadSlotFromFile(const char* path, uint32_t slotIdx)
{
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

void RmseTool::TriggerStoreNextOutput()
{
    CherryAssert(m_state == RmseToolState::eIdle);
    TransitionState(RmseToolState::eStoreNextOutput);
}

void RmseTool::TriggerStoreAtMaxFrames(uint32_t maxFrames)
{
    CherryAssert(m_state == RmseToolState::eIdle);
}

void RmseTool::TriggerComputeSingleRMSE()
{
    CherryAssert(m_state == RmseToolState::eIdle);
    TransitionState(RmseToolState::eComputeSingleRMSE);
}

void RmseTool::TriggerComputeConvergence()
{
    CherryAssert(m_state == RmseToolState::eIdle);
}

void RmseTool::TriggerPlotConvergence()
{
    CherryAssert(m_state == RmseToolState::eIdle);
}

void RmseTool::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo, uint32_t frameIdx, D12Resource* ptOutput)
{
    if (m_state == RmseToolState::eComputeSingleRMSE)
    {
        ComputeSingleRMSE(d3d, renderInfo.Heap);
        TransitionState(RmseToolState::eIdle);
        return;
    }

    if (m_state == RmseToolState::eStoreNextOutput)
    {
        const std::string nameSuffix = " (Frame=" + std::to_string(frameIdx) + ")";
        StoreTextureToSelectedSlot(d3d, renderInfo.Heap, ptOutput, nameSuffix.c_str());
        TransitionState(RmseToolState::eIdle);
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

/*

void RmseTool::BeginComputeGolden(uint32_t maxFrames, const char* path)
{
    m_runningComputeGolden = true;
    m_maxFrames = maxFrames;
    m_taskName = path;
}

void RmseTool::UpdateComputeGolden(D3D* d3d, const uint32_t currFrame, D12Resource* finalRTV)
{
    if (currFrame < m_maxFrames || !m_runningComputeGolden)
        return;

    if (!m_goldenReadbackBuffer.IsInitialized())
    {
        m_goldenReadbackBuffer.Init(d3d, finalRTV);
    }
    m_goldenReadbackBuffer.Readback(d3d, finalRTV);
    const std::vector<uint8_t> readbackData = m_goldenReadbackBuffer.GetData();

    SaveGolden(readbackData.data(), readbackData.size(), finalRTV->GetDesc().Width, finalRTV->GetDesc().Height);
    m_runningComputeGolden = false;
}

void RmseTool::SaveGolden(const uint8_t* data, const size_t bufferSize, const int width, const int height) const
{
    const std::string filePath = wstringToString(ASSETS_SOURCE_DIR) + "/../Data/GoldenImages/" + m_taskName + ".png";

    const std::filesystem::path path = filePath;
    std::filesystem::create_directories(path.parent_path());

    FILE* file;
    fopen_s(&file, filePath.c_str(), "wb");
    if (!file)
        throw std::exception("I/O Error");

    int ret;
    spng_ctx* ctx = spng_ctx_new(SPNG_CTX_ENCODER);

    spng_ihdr ihdr = {};
    ihdr.width = width;
    ihdr.height = height;
    ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
    ihdr.bit_depth = 8;
    ret = spng_set_ihdr(ctx, &ihdr);
    assert(ret == 0);
    ret = spng_set_png_file(ctx, file);
    assert(ret == 0);
    ret = spng_encode_image(ctx, data, bufferSize, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
    assert(ret == 0);
    spng_ctx_free(ctx);

    fclose(file);
}

void RmseTool::PrepareLoadGolden(const char* path)
{
    m_taskName = path;
    m_loadGoldenNextFrame = true;
}

void RmseTool::LoadGolden(D3D* d3d, const uint32_t slot)
{
    auto* slotTex = slot == 0 ? &m_slotA : &m_slotB;

    const std::string filePath = wstringToString(ASSETS_SOURCE_DIR) + "/../Data/GoldenImages/" + m_taskName + ".png";

    d3d->Flush();
    const auto cmdList = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    slotTex->Init(d3d->GetDevice(), cmdList.Get(), filePath);
    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList.Get());
    d3d->Flush();

    if (slot == 0)
        m_slotAFilled = true;
    else
        m_slotBFilled = true;

    m_loadGoldenNextFrame = false;
}

void RmseTool::BeginConvergenceTest(const uint32_t maxFrames, const char* testName, const uint32_t frameInc, const bool plotAndShow)
{
    m_maxFrames = maxFrames;
    m_taskName = testName;
    m_frameIncrement = frameInc;
    m_plotAndShow = plotAndShow;
    m_lastFrameConvergenceTested = 0;
    m_rmses.clear();
    m_runningConvergenceTest = true;
}

void RmseTool::UpdateConvergenceTest(D3D* d3d, const uint32_t currFrame, Heap* heap, D12Resource* finalRTV)
{
    if (currFrame - m_lastFrameConvergenceTested < m_frameIncrement)
        return;

    TakeSnapshot(d3d, 1, finalRTV);
    ComputeRMSE(d3d, heap);
    m_rmses.emplace_back(m_lastComputedRMSE);
    m_lastFrameConvergenceTested = currFrame;

    if (currFrame < m_maxFrames)
        return;

    const std::string filePath = wstringToString(ASSETS_SOURCE_DIR) + "/../Data/RMSEs/" + m_taskName + ".csv";

    // Write RMSEs to file
    {
        const std::filesystem::path path = filePath;
        std::filesystem::create_directories(path.parent_path());

        std::fstream f;
        f.open(filePath.c_str(), std::ios::binary | std::fstream::out | std::ios::trunc);
        f.clear();
        f << "RMSE" << std::endl;
        for (int i = 0; i < m_rmses.size(); i++)
            f << std::to_string(m_rmses[i]) << std::endl;
        f.close();
    }

    // Plot graph
    if (m_plotAndShow)
    {
        std::vector<const char*> args = {
            filePath.c_str(),
            "--show", "--save"
        };
        PythonExecutor::ExecutePython("PlotConvergence.py", args);
    }

    m_runningConvergenceTest = false;
}

void RmseTool::CompareTests(const std::vector<std::string>& testNames, const bool logPlot)
{
    // Plot graph
    {
        std::vector<const char*> args;
        for (int i = 0; i < testNames.size(); i++)
        {
            const std::string filePath = "\"" + wstringToString(ASSETS_SOURCE_DIR) + "/../Data/RMSEs/" + testNames[i] + ".csv\"";
            args.push_back(_strdup(filePath.c_str()));
        }
        if (logPlot)
            args.push_back("--log");
        PythonExecutor::ExecutePython("PlotMultiConvergence.py", args);
    }
}

*/