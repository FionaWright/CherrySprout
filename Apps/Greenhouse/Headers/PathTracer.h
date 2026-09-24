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
#include "PathTracing/Flags/MethodsCpp.h"
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
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* RTV, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle) override;
    void UnreserveData() override;
    void UpdatePipeline(ID3D12Device* device, const PathTracerFeatureFlags& featureFlags, const PathTracingDebugInfo& debugInfo, const BxdfMode& bxdfMode, const MicrofacetModelType& microfacetModelType);
    void Reset();

    size_t TotalCbvRequiredSize() override
    {
        return Align(sizeof(CbvPathTracingSettings), 256) +
            Align(sizeof(CbvPathTracingDebugSettings), 256) +
            RestirManager::TotalCbvRequiredSize();
    }

    D12Resource* GetTexPrimal() { return &m_primal; }
    D12Resource* GetTexAccum() { return &m_accum; }
    [[nodiscard]] uint32_t GetCurrentFrameIdx() const { return m_frameIdx; }

    void GUI(GreenhouseConfig* config) override;

private:
    RtasBuilder m_rtasBuilder;
    RestirManager m_restirManager;
    GradientManager m_gradientManager;

    uint32_t m_frameIdx = 0;

    Pipeline m_pipeline;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
    D12Resource m_primal, m_accum;
    D12Resource m_output;

    Pipeline m_pipelineBlit;
    RootSig m_rootSigBlit;
    DescriptorSet m_setBlit;

#if CHERRY_DEBUG_FEATURES_ENABLED
    DebugManager m_debugManager;
#endif
};


#endif //CHERRYSPROUT_PATHTRACER_H