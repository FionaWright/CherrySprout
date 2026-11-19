//
// Created by fionaw on 01/06/2026.
//

#ifndef CHERRYSPROUT_RTASBUILDER_H
#define CHERRYSPROUT_RTASBUILDER_H

#include <d3d12.h>
#include <vector>

#include "D12Resource.h"
#include "D3D.h"
#include "UploadHeap.h"
#include "Scene/Scene.h"
#include "Scene/InstanceData.h"

struct BlasEntry
{
    D12Resource Scratch;
    D12Resource Result;

    D3D12_RAYTRACING_GEOMETRY_DESC GeomDesc;
};

class RtasBuilder
{
public:
    void FlushAndBuild(D3D* d3d, Scene* scene, bool isAnimationEnabled);
    void Build(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, Scene* scene, bool isAnimationEnabled);
    void Update(ID3D12GraphicsCommandList4* cmdList, Scene* scene);

    D12Resource* GetRtasResource() { return &m_tlasResult; }

private:
    void buildBlas(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* vertexBuffer, ID3D12Resource* indexBuffer, Object
                   * object, bool isAnimationEnabled);
    void buildTlas(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
                   const std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& blasInstances);
    void updateBlas(ID3D12GraphicsCommandList4* cmdList, const BlasEntry& entry);

    UploadHeap m_uploadHeap;
    D12Resource m_tlasScratch;
    D12Resource m_tlasResult;
    D12Resource m_tlasInstanceBuffer;

    std::vector<BlasEntry> m_blasList;
};


#endif //CHERRYSPROUT_RTASBUILDER_H