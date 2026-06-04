//
// Created by fionaw on 01/06/2026.
//

#ifndef CHERRYSPROUT_RTASBUILDER_H
#define CHERRYSPROUT_RTASBUILDER_H

#include <d3d12.h>
#include <vector>

#include "D12Resource.h"
#include "UploadHeap.h"
#include "Scene/Scene.h"

struct BlasEntry
{
    D12Resource Scratch;
    D12Resource Result;
};

class RtasBuilder
{
public:
    void Build(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, Scene* scene);

    D12Resource* GetRtasResource() { return &m_tlasResult; }

private:
    void buildBlas(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* vertexBuffer, ID3D12Resource* indexBuffer, Object* object);
    void buildTlas(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
                   const std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& blasInstances);

    UploadHeap m_uploadHeap;
    D12Resource m_tlasScratch;
    D12Resource m_tlasResult;
    ComPtr<ID3D12Resource> m_tlasInstanceBuffer;

    std::vector<BlasEntry> m_blasList;
    std::vector<InstanceData> m_megaBufferInstanceData;
};


#endif //CHERRYSPROUT_RTASBUILDER_H