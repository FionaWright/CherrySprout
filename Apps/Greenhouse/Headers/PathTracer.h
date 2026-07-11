//
// Created by fionaw on 01/06/2026.
//

#ifndef CHERRYSPROUT_PATHTRACER_H
#define CHERRYSPROUT_PATHTRACER_H

#include "GBufferPrePass.h"
#include "IRenderBackend.h"
#include "PathTracer.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Heap.h"
#include "HWI/RootSig.h"
#include "HWI/RtasBuilder.h"
#include "HWI/Pipeline.h"
#include "PathTracing/Debug/OutputColor.h"
#include "Utils/CBVs.h"
#include "PathTracing/Flags/MethodsCpp.h"
#include "Utils/Debug/DebugID.h"
#include "Utils/Debug/DebugStructs.h"

class Camera;
struct PathTracingDebugInfo;
struct PathTracerConfig;
enum class BxdfMode : hlsl::uint;
enum class DebugOutputIndex : hlsl::uint;
struct TimeArgs;

#define PATH_DUMP_MAX_RAY_DEPTH 32

class PathTracer final : public IRenderBackend
{
public:
    void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV) override;
    void LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap, LightImportanceSampler* lightImportanceSampler) override;
    void Update(D3D* d3d, TimeArgs timeArgs) override;
    void PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo) override;
    void UnreserveData() override;
    bool GBufferRequired(const PathTracerFeatureFlags& featureFlags, const PathTracerDebugFlags& debugFlags) const;
    void UpdatePipeline(ID3D12Device* device, const PathTracerFeatureFlags& featureFlags, const PathTracingDebugInfo& debugInfo, const BxdfMode& bxdfMode);
    void Reset();

    size_t TotalCbvRequiredSize() override { return Align(sizeof(CbvPathTracingSettings), 256); }

    D12Resource* GetTexOutput() { return &m_output; }
    D12Resource* GetTexAccum() { return &m_accum; }
    uint32_t GetCurrentFrameIdx() const { return m_frameIdx; }

#ifdef _DEBUG
    void RenderGUI_DebugInfo(const PathTracerConfig& config);
#endif

private:
    RtasBuilder m_rtasBuilder;
    GBufferPrePass m_gbufferPrePass;

    uint32_t m_frameIdx = 0;

    Pipeline m_pipeline;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
    D12Resource m_output, m_accum;

#ifdef _DEBUG
    D12Resource m_gpuErrorInfoRW, m_gpuErrorInfoReadback;
    DebugErrorInfo m_cpuErrorInfo[_countof(s_debugIdList)] = {};
    bool m_scheduleClearErrors = false;

    RayDump m_cpuPathDump[PATH_DUMP_MAX_RAY_DEPTH] = {};
    hlsl::uint2 m_dumpedPathPixelCoords = {};
    uint32_t m_dumpedPathFrameIdx = 0;
    XMFLOAT3 m_dumpedPathCameraPosition = {};
    XMMATRIX m_dumpedPathViewMatrix = {};
    D12Resource m_pathDumpBufferRW, m_pathDumpBufferReadback;
    bool m_isPathDumpAutomatic = true;

    enum class ScheduledRunState
    {
        eIdle,
        eRunFrame,
        eReadbackPathDump,
        eDisplay
    };

    hlsl::uint2 m_scheduledRunPixelCoords = {};
    uint32_t m_scheduledRunFrameIdx = 0;
    XMFLOAT3 m_scheduledRunCameraPosition = {};
    XMMATRIX m_scheduledRunViewMatrix = {};
    ScheduledRunState m_scheduledRunState = ScheduledRunState::eIdle;
#endif
};


#endif //CHERRYSPROUT_PATHTRACER_H