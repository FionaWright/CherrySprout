//
// Created by fionaw on 01/06/2026.
//

#ifndef CHERRYSPROUT_PATHTRACER_H
#define CHERRYSPROUT_PATHTRACER_H

#include "IRenderBackend.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Heap.h"
#include "HWI/RootSig.h"
#include "HWI/RtasBuilder.h"
#include "HWI/Pipeline.h"
#include "Utils/CBVs.h"
#include "PathTracing/Flags.h"

enum class DebugOutputIndex : hlsl::uint;
struct TimeArgs;

class PathTracer final : public IRenderBackend
{
public:
    void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV) override;
    void LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap) override;
    void Update(D3D* d3d, TimeArgs timeArgs) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo) override;
    void UnreserveData() override;
    void UpdatePipeline(ID3D12Device* device, const PathTracerFeatureFlags& featureFlags,
                        const PathTracerDebugFlags& debugFlags, const DebugOutputIndex& debugOutputIdx);
    void Reset();

    size_t TotalCbvRequiredSize() override { return Align(sizeof(CbvPathTracingSettings), 256); }

    D12Resource* GetTexOutput() { return &m_output; }

private:
    RtasBuilder m_rtasBuilder;

    uint32_t m_frameIdx = 0;

    Pipeline m_pipeline;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
    D12Resource m_output, m_accum;
};


#endif //CHERRYSPROUT_PATHTRACER_H