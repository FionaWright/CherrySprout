#ifndef H_GIZMO_MANAGER_H
#define H_GIZMO_MANAGER_H

#if !CHERRY_DEBUG_FEATURES_ENABLED
#   error
#endif

#include <DirectXMath.h>
#include <unordered_map>

#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "HWI/UploadHeap.h"

struct CD3DX12_CPU_DESCRIPTOR_HANDLE;
using namespace DirectX;

class Heap;
class D3D;

struct Gizmo
{
    XMFLOAT3 Position;
    std::string TextureFilepath;
    DescriptorSet DescSet;
};

class GizmoManager
{
public:
    void AddGizmo(D3D* d3d, Heap* heap, XMFLOAT3 position, XMFLOAT4 color, const char* texFilepath);
    void Render(D3D* d3d, const Heap* heap, ID3D12GraphicsCommandList* cmdList, const XMMATRIX& V, const XMMATRIX& P);

    void ClearGizmos() { m_gizmos.clear(); }

private:
    void initResources(D3D* d3d, ID3D12GraphicsCommandList* cmdList);

    std::vector<Gizmo> m_gizmos;
    std::unordered_map<std::string, D12Resource> m_textureCache;

    D12Resource m_quadVertexBuffer;
    UploadHeap m_uploadHeap;

    Pipeline m_pipeline;
    RootSig m_rootSig;
    RootConstants m_rootConstants;
    bool m_initializedResources = false;
};

#endif