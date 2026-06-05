//
// Created by fiona on 14/01/2026.
//

#ifndef CHERRYPIP_SKYBOX_H
#define CHERRYPIP_SKYBOX_H

#include "HWI/DescriptorSet.h"
#include "HWI/RootSig.h"
#include "HWI/Pipeline.h"

class D3D;

class Skybox
{
public:
    void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Heap* heap, UploadHeap* uploadHeap, D12Resource* cubemap);
    void RenderForward(const D3D* d3d, ID3D12GraphicsCommandList* cmdList, Heap* heap, const XMMATRIX& vMatrix, const XMMATRIX& pMatrix) const;

    void UpdateCubemap(ID3D12Device* device, D12Resource* cubemap, Heap* heap, UploadHeap* uploadHeap);
    void GenerateIrradianceMap(ID3D12GraphicsCommandList* cmdList, const Heap* heap);

    D12Resource* GetIrradianceMap() { return &m_texIrradianceIBL; }

private:
    D12Resource m_cubeVertexBuffer;
    D12Resource m_cubeIndexBuffer;

    RootSig m_rootSig;
    Pipeline m_shaderForward;
    DescriptorSet m_dsForwardRender;

    RootSig m_rootSigGenIrr;
    Pipeline m_shaderGenIrr;
    DescriptorSet m_dsGenIrr;
    D12Resource m_texIrradianceIBL;
};


#endif //CHERRYPIP_SKYBOX_H