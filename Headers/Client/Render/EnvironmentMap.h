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

class D3D;

struct CBV_PanoToEA
{
    uint32_t OutputWidth;
    uint32_t OutputHeight;
    uint32_t InputWidth;
    uint32_t InputHeight;

    float Rotation;
    float p[3];
};

struct CBV_PanoToCM
{
    uint32_t OutputWidth;
    uint32_t InputWidth;
    uint32_t InputHeight;
    float Rotation;
};

class EnvironmentMap
{
public:
    void CreateCubemapResource(ID3D12Device* device);
    void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Heap* heap, UploadHeap* uploadHeap, const std::string& filePath, float rotation);
    void InitCubemap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Heap* heap, UploadHeap* uploadHeap);

    XMFLOAT3 GetDirectionOfHighestIntensity(D3D* d3d, Heap* heap);

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
    Pipeline m_shaderPanoToEA, m_shaderPanoToCM;
    DescriptorSet m_dsPanoToEA, m_dsPanoToCM;

    //RootSig m_rootSigMaxLumRedSearch;
    //Shader m_shaderMaxLumRedSearch;
    //DescriptorSet m_matMaxLumRedSearch;
    //D12Resource m_bufferMaxLumRedSearch;
    //D12Resource m_readbackBufferMaxLumRedSearch;
};


#endif //CHERRYPIP_ENVMAP_H