//
// Created by fionaw on 01/06/2026.
//

#ifndef CHERRYSPROUT_PATHTRACER_H
#define CHERRYSPROUT_PATHTRACER_H

#include "HWI/DescriptorSet.h"
#include "HWI/Heap.h"
#include "HWI/RootSig.h"
#include "HWI/RtasBuilder.h"
#include "HWI/Shader.h"
#include "HWI/UploadHeap.h"
#include "PathTracing/CBVs.h"

struct TimeArgs;

class PathTracer
{
public:
    void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV, Scene* scene);
    void Update(D3D* d3d, TimeArgs timeArgs);
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const Heap* heap, Scene* scene);

    static size_t TotalCbvRequiredSize() { return sizeof(CbvPathTracingSettings); }

    D12Resource* GetTexOutput() { return &m_output; }

private:
    RtasBuilder m_rtasBuilder;
    bool m_rtasDirty = true;
    UploadHeap m_rtasUploadHeap;

    uint32_t m_frameIdx = 0;

    Shader m_shaderPT;
    RootSig m_rootSigPT;
    DescriptorSet m_descriptorSet;
    D12Resource m_output, m_accum;
};


#endif //CHERRYSPROUT_PATHTRACER_H