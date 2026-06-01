//
// Created by fiona on 01/06/2026.
//

#ifndef CHERRYSPROUT_FORWARD_H
#define CHERRYSPROUT_FORWARD_H

#include "HWI/DescriptorSet.h"
#include "HWI/RootSig.h"
#include "HWI/Shader.h"
#include "Scene/Material.h"
#include "Utils/CommonStructs.h"

struct Scene;
struct TimeArgs;

class Forward
{
public:
    void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV, Scene* scene);
    void Update(D3D* d3d, TimeArgs timeArgs);
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const Heap* heap, Scene* scene, const XMMATRIX& V, const XMMATRIX& P) const;

    static size_t TotalCbvRequiredSize() { return sizeof(CbvMatrices) + sizeof(CbvForward) + sizeof(Material); }

private:
    Shader m_shader;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
};


#endif //CHERRYSPROUT_FORWARD_H