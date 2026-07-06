#ifndef H_GBUFFER_PRE_PASS_H
#define H_GBUFFER_PRE_PASS_H

#include <d3d12.h>

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"

struct Scene;
class D3D;
class Heap;

class GBufferPrePass
{
public:
    void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV);
    void LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV);

    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Scene* scene);

private:
    D12Resource m_gbufferTexMaterialIdx;
    D12Resource m_gbufferTexNormals;
    D12Resource m_gbufferTexDepth;
    D12Resource m_gbufferTexUvMv;

    Pipeline m_pipeline;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
    RootConstants m_rootConstants;
};

#endif