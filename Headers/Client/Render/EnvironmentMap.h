//
// Created by fionaw on 14/11/2025.
//

#ifndef CHERRYPIP_ENVMAP_H
#define CHERRYPIP_ENVMAP_H

#include <DirectXMath.h>
#include <string>

#include "HWI/DescriptorSet.h"
#include "HWI/RootSig.h"
#include "HWI/Pipeline.h"
#include "../../../Assets/Shaders/Utils/CBVs.h"
#include "HWI/UploadHeap.h"
#include "Utils/D3DUtils.h"

class D3D;

class EnvironmentMap
{
public:
    void CreateCubemapResource(ID3D12Device* device);

    void Init(D3D* d3d, Heap* heap, const std::string& filePath, float rotation);
    void InitCubemap(D3D* d3d, Heap* heap);

    XMFLOAT3 GetDirectionOfHighestIntensity(D3D* d3d, Heap* heap);
    static size_t GetCbvRequiredSize() { return Align(sizeof(CbvPanoToEA), 256) + Align(sizeof(CbvPanoToCM), 256); }

    D12Resource* GetPano() { return &m_pano; }
    D12Resource* GetEA() { return &m_ea; }
    D12Resource* GetCubemap() { return &m_cubemap; }

private:
    void initResources(ID3D12Device* device);

    D12Resource m_pano, m_ea, m_cubemap;
    float m_rotation = 0.0f;
    std::string m_currentPanoFilepath = "";
    bool m_resourcesInitialized = false;

    RootSig m_rootSigPanoToEA, m_rootSigPanoToCM;
    Pipeline m_pipelinePanoToEA, m_pipelinePanoToCM;
    DescriptorSet m_dsPanoToEA, m_dsPanoToCM;

    //RootSig m_rootSigMaxLumRedSearch;
    //Shader m_shaderMaxLumRedSearch;
    //DescriptorSet m_matMaxLumRedSearch;
    //D12Resource m_bufferMaxLumRedSearch;
    //D12Resource m_readbackBufferMaxLumRedSearch;
};


#endif //CHERRYPIP_ENVMAP_H