//
// Created by fionaw on 02/06/2026.
//

#ifndef CHERRYSPROUT_IRENDERBACKEND_H
#define CHERRYSPROUT_IRENDERBACKEND_H

#include "HWI/D3D.h"
#include "HWI/Heap.h"
#include "HWI/UploadHeap.h"
#include "Scene/Scene.h"
#include "System/HighResolutionClock.h"
#include "Utils/D3DUtils.h"

struct GreenHouseRenderInfo;

interface IRenderBackend
{
    virtual ~IRenderBackend() = default;

    virtual void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV) { m_isInitialized = true; }
    virtual void LoadSceneData(ID3D12Device* device, Scene* scene) = 0;
    virtual void Update(D3D* d3d, TimeArgs timeArgs) = 0;
    virtual void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo) = 0;
    virtual void UnreserveData() = 0;

    virtual size_t TotalCbvRequiredSize() = 0;

    bool IsInitialized() const { return m_isInitialized; }

protected:
    bool m_isInitialized = false;
};


#endif //CHERRYSPROUT_IRENDERBACKEND_H