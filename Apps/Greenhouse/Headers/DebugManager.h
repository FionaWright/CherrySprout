#ifndef H_DEBUG_MANAGER_H
#define H_DEBUG_MANAGER_H

#include "RmseTool.h"
#include "HWI/D3D.h"
#include "HWI/D12Resource.h"
#include "../../../Assets/Shaders/PathTracing/Debug/Internal/OutputColor.h"

#include "Utils/Debug/DebugID.h"
#include "Utils/Debug/DebugStructs.h"

#define PATH_DUMP_MAX_RAY_DEPTH 32

struct PathTracerConfig;
struct CbvPathTracingSettings;
struct GreenHouseRenderInfo;

enum class ScheduledRunState
{
    eIdle,
    eRecompilePipelineAndRunFrame,
    eRunFrame,
    eReadbackPathDump,
    eDisplay
};

class DebugManager
{
public:
    void Init(const D3D* d3d, Heap* heap);
    void UnreserveData();
    void PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo, uint32_t frameIdx, D12Resource* ptOutput);

    D12Resource* GetBufferGPUErrorInfoRW() { return &m_gpuErrorInfoRW; }
    D12Resource* GetBufferPathDumpRW() { return &m_pathDumpBufferRW; }
    D12Resource* GetBufferPathDumpOCRW() { return &m_pathDumpOCBufferRW; }

    bool IsScheduledRunState(const ScheduledRunState state) const { return m_scheduledRunState == state; }
    hlsl::uint2 GetScheduledRunPixelCoords() const { return m_scheduledRunPixelCoords; }
    void ScheduledRunTransition(const ScheduledRunState state) { m_scheduledRunState = state; }

    void SetScheduledRunParameters(CbvPathTracingSettings* settings) const;

    void GUI(PathTracerConfig& config, bool& ptFrameDirty);

private:
    D12Resource m_gpuErrorInfoRW, m_gpuErrorInfoReadback;
    DebugErrorInfo m_cpuErrorInfo[_countof(s_debugIdList)] = {};
    bool m_scheduleClearErrors = false;

    RayDump m_cpuPathDump[PATH_DUMP_MAX_RAY_DEPTH] = {};
    DebugOutputStruct m_cpuPathDumpOutputColor[PATH_DUMP_MAX_RAY_DEPTH] = {};
    hlsl::uint2 m_dumpedPathPixelCoords = {};
    uint32_t m_dumpedPathFrameIdx = 0;
    XMFLOAT3 m_dumpedPathCameraPosition = {};
    XMMATRIX m_dumpedPathViewMatrix = {};
    D12Resource m_pathDumpBufferRW, m_pathDumpBufferReadback;
    D12Resource m_pathDumpOCBufferRW, m_pathDumpOCBufferReadback;
    bool m_isPathDumpAutomatic = true;

    hlsl::uint2 m_scheduledRunPixelCoords = {};
    uint32_t m_scheduledRunFrameIdx = 0;
    XMFLOAT3 m_scheduledRunCameraPosition = {};
    XMMATRIX m_scheduledRunViewMatrix = {};
    ScheduledRunState m_scheduledRunState = ScheduledRunState::eIdle;

    RmseTool m_rmseTool;
};

#endif