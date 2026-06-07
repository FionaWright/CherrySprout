//
// Created by fiona on 14/01/2026.
//

#ifndef CHERRYPIP_SKYBOX_H
#define CHERRYPIP_SKYBOX_H

#include "HWI/DescriptorSet.h"
#include "HWI/RootSig.h"
#include "HWI/Pipeline.h"
#include "Utils/CommonStructs.h"
#include "Utils/D3DUtils.h"

class D3D;

class Skybox
{
public:
    void Init(D3D* d3d, D12Resource* cubemap);
    void RenderForward(ID3D12GraphicsCommandList* cmdList, const XMMATRIX* vMatrix, const XMMATRIX* pMatrix) const;

    void UpdateDescriptorSet(ID3D12Device* device, D12Resource* cubemap, Heap* heap, UploadHeap* uploadHeapCBV);
    void GenerateIrradianceMap(ID3D12GraphicsCommandList* cmdList, const Heap* heap);

    D12Resource* GetIrradianceMap() { return &m_texIrradianceIBL; }

    static size_t TotalCbvRequiredSize()
    {
        return  Align(sizeof(CbvMatrices), 256);
    }

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