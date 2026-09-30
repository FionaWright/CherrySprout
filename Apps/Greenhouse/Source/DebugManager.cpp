#include "System/pch.h"
#include "DebugManager.h"

#include "Greenhouse.h"
#include "../../../Assets/Shaders/PathTracing/Flags/Internal/MethodsCpp.h"
#include "Utils/Helper.h"

void DebugManager::Init(const D3D* d3d, Heap* heap)
{
    {
        constexpr size_t bufferSize = _countof(s_debugIdList) * sizeof(DebugErrorInfo);
        m_gpuErrorInfoRW.Release();
        m_gpuErrorInfoReadback.Release();
        m_gpuErrorInfoRW.Init_Buffer("Error Info (RW)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
        m_gpuErrorInfoReadback.Init_Buffer("Error Info (Readback)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_NONE, true, D3D12_RESOURCE_STATE_COPY_DEST);
    }

    {
        constexpr size_t bufferSize = sizeof(RayDump) * PATH_DUMP_MAX_RAY_DEPTH;
        m_pathDumpBufferRW.Release();
        m_pathDumpBufferReadback.Release();
        m_pathDumpBufferRW.Init_Buffer("Path Dump Buffer (RW)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_pathDumpBufferReadback.Init_Buffer("Path Dump Buffer (Readback)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_NONE, true, D3D12_RESOURCE_STATE_COPY_DEST);
    }

    {
        constexpr size_t bufferSize = sizeof(DebugOutputStruct) * PATH_DUMP_MAX_RAY_DEPTH;
        m_pathDumpOCBufferRW.Release();
        m_pathDumpOCBufferReadback.Release();
        m_pathDumpOCBufferRW.Init_Buffer("Path Dump OC Buffer (RW)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_pathDumpOCBufferReadback.Init_Buffer("Path Dump OC Buffer (Readback)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_NONE, true, D3D12_RESOURCE_STATE_COPY_DEST);
    }

    m_rmseTool.Init(d3d, heap);
}

void DebugManager::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo, uint32_t frameIdx, D12Resource* ptOutput)
{
    if (renderInfo.PathTracerConfig->DebugEnabled(eDebug_Asserts))
    {
        d3d->Flush();

        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();

        m_gpuErrorInfoRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_gpuErrorInfoReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        constexpr size_t bufferSize = _countof(s_debugIdList) * sizeof(DebugErrorInfo);
        cmdList->CopyBufferRegion(m_gpuErrorInfoReadback.GetResource(), 0, m_gpuErrorInfoRW.GetResource(), 0, bufferSize);

        UploadHeap uploadHeapClear;
        if (m_scheduleClearErrors)
        {
            uploadHeapClear.Init(d3d->GetDevice(), Align(m_gpuErrorInfoRW.GetIntermediateSize(), 512));

            DebugErrorInfo debugErrorInfoClear{};
            debugErrorInfoClear.ExprCounter = 0;
            debugErrorInfoClear.NaNCounter = 0;
            debugErrorInfoClear.InfCounter = 0;

            const std::vector<DebugErrorInfo> cpuClearBuffer(_countof(s_debugIdList), debugErrorInfoClear);
            m_gpuErrorInfoRW.UploadBuffer(cmdList, &uploadHeapClear, cpuClearBuffer.data(), bufferSize);

            m_scheduleClearErrors = false;
        }

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();

        m_gpuErrorInfoReadback.Readback(&m_cpuErrorInfo);
    }

#define PATH_DUMP_UPDATE_COOLDOWN 300

    const bool scheduledRun = m_scheduledRunState == ScheduledRunState::eReadbackPathDump;
    const bool needPathDump = m_isPathDumpAutomatic || scheduledRun;
    if (renderInfo.PathTracerConfig->DebugEnabled(eDebug_PathDumper) && needPathDump)
    {
        static int s_pathDumpTimer = PATH_DUMP_UPDATE_COOLDOWN;
        if (s_pathDumpTimer > 0 && !scheduledRun)
        {
            s_pathDumpTimer--;
            return;
        }
        s_pathDumpTimer = PATH_DUMP_UPDATE_COOLDOWN;

        d3d->Flush();

        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();

        m_pathDumpBufferRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_pathDumpBufferReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        m_pathDumpOCBufferRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_pathDumpOCBufferReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        constexpr size_t bufferSize = PATH_DUMP_MAX_RAY_DEPTH * sizeof(RayDump);
        cmdList->CopyBufferRegion(m_pathDumpBufferReadback.GetResource(), 0, m_pathDumpBufferRW.GetResource(), 0, bufferSize);
        cmdList->CopyBufferRegion(m_pathDumpOCBufferReadback.GetResource(), 0, m_pathDumpOCBufferRW.GetResource(), 0, bufferSize);

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();

        m_pathDumpBufferReadback.Readback(&m_cpuPathDump);
        m_pathDumpOCBufferReadback.Readback(&m_cpuPathDumpOutputColor);

        m_dumpedPathPixelCoords = scheduledRun ? m_scheduledRunPixelCoords : renderInfo.PathTracerConfig->DebugInfo.ChosenPixelCoords;
        m_dumpedPathFrameIdx = scheduledRun ? m_scheduledRunFrameIdx : frameIdx;
        m_dumpedPathCameraPosition = scheduledRun ? m_scheduledRunCameraPosition : renderInfo.Camera->GetPosition();
        m_dumpedPathViewMatrix = scheduledRun ? m_scheduledRunViewMatrix : renderInfo.Camera->GetViewMatrix();

        if (scheduledRun)
            m_scheduledRunState = ScheduledRunState::eDisplay;
    }

    m_rmseTool.PostUpdate(d3d, renderInfo, frameIdx, ptOutput);
}

void DebugManager::SetScheduledRunParameters(CbvPathTracingSettings* settings) const
{
    settings->FrameIdx = m_scheduledRunFrameIdx;
    settings->CameraPositionWorld = m_scheduledRunCameraPosition;
    XMStoreFloat4x4(&settings->InvV, XMMatrixInverse(nullptr, m_scheduledRunViewMatrix));
}

void DebugManager::UnreserveData()
{
    m_gpuErrorInfoRW.Release();
    m_gpuErrorInfoReadback.Release();
    m_pathDumpBufferRW.Release();
    m_pathDumpBufferReadback.Release();
}
