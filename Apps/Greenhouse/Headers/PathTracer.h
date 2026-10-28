//
// Created by fionaw on 01/06/2026.
//

#ifndef CHERRYSPROUT_PATHTRACER_H
#define CHERRYSPROUT_PATHTRACER_H

#include "DebugManager.h"
#include "GBufferPrePass.h"
#include "GradientManager.h"
#include "IRenderBackend.h"
#include "PathTracer.h"
#include "HWI/DescriptorSet.h"
#include "HWI/RootSig.h"
#include "HWI/RtasBuilder.h"
#include "HWI/Pipeline.h"
#include "MicrofacetModels/MMTypes.h"
#include "Utils/CBVs.h"
#include "Render/RestirManager.h"

class Camera;
struct PathTracingDebugInfo;
struct PathTracerConfig;
enum class BxdfMode : hlsl::uint;
enum class DebugOutputIndex : hlsl::uint;
struct TimeArgs;

class PathTracer final : public IRenderBackend
{
public:
    void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV) override;
    void LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap, LightImportanceSampler* lightImportanceSampler, GBufferPrePass* gbuffer) override;
    void Update(D3D* d3d, Heap* heap, TimeArgs timeArgs) override;
    void PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* RTV, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle) override;
    void UnreserveData() override;
    void UpdatePipeline(ID3D12Device* device, const FeatFlagsCore& featureFlags, const PathTracingDebugInfo& debugInfo, const BxdfMode& bxdfMode, const MicrofacetModelType& microfacetModelType);
    void Reset();

    size_t TotalCbvRequiredSize() override
    {
        return Align(sizeof(CbvPathTracingSettings), 256) +
            Align(sizeof(CbvPathTracingDebugSettings), 256) +
            Align(sizeof(CbvRestirSettings), 256) +
            RestirManager::TotalCbvRequiredSize();
    }

    D12Resource* GetTexPrimal() { return &m_primal; }
    D12Resource* GetTexAccum() { return &m_accum; }
    D12Resource* GetHdrOutput() const { return m_finalHdrOutput; }

    void ScheduleRestirClear() { m_restirClearScheduled = true; }

    void GUI(GreenhouseConfig* config, bool& frameDirty) override;

private:
    RtasBuilder m_rtasBuilder;
    RestirManager m_restirManager;
    GradientManager m_gradientManager;

    Pipeline m_pipeline;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
    D12Resource m_primal, m_accum;
    D12Resource m_outputRgba8Unorm;
    D12Resource* m_finalHdrOutput = nullptr;

    Pipeline m_pipelineBlit;
    RootSig m_rootSigBlit;
    DescriptorSet m_setBlit;

    bool m_restirClearScheduled = false;

#if CHERRY_DEBUG_FEATURES_ENABLED
    DebugManager m_debugManager;
#endif
};


#endif //CHERRYSPROUT_PATHTRACER_H