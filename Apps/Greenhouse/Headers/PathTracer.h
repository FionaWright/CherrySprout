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
#include "HWI/Shader.h"
#include "HWI/UploadHeap.h"
#include "PathTracing/CBVs.h"

struct TimeArgs;

class PathTracer : public IRenderBackend
{
public:
    void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV, Scene* scene) override;
    void Update(D3D* d3d, TimeArgs timeArgs) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const Heap* heap, Scene* scene, const XMMATRIX& V, const XMMATRIX& P) override;
    void UnreserveData() override;

    size_t TotalCbvRequiredSize() override { return Align(sizeof(CbvPathTracingSettings), 256); }

    D12Resource* GetTexOutput() { return &m_output; }

private:
    RtasBuilder m_rtasBuilder;
    bool m_rtasDirty = true;
    UploadHeap m_rtasUploadHeap;

    uint32_t m_frameIdx = 0;

    Shader m_shader;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
    D12Resource m_output, m_accum;
};


#endif //CHERRYSPROUT_PATHTRACER_H