//
// Created by fiona on 01/06/2026.
//

#ifndef CHERRYSPROUT_FORWARD_H
#define CHERRYSPROUT_FORWARD_H

#include "IRenderBackend.h"
#include "HWI/DescriptorSet.h"
#include "HWI/RootSig.h"
#include "HWI/Pipeline.h"
#include "Scene/Material.h"
#include "Utils/CommonStructs.h"

struct Scene;
struct TimeArgs;

class Forward : public IRenderBackend
{
public:
    void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV) override;
    void LoadSceneData(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Scene* scene) override {};
    void Update(D3D* d3d, TimeArgs timeArgs) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo) override;
    void UnreserveData() override {};

    size_t TotalCbvRequiredSize() override
    {
        return  Align(sizeof(CbvMatrices), 256) +
                Align(sizeof(CbvForward), 256) +
                Align(sizeof(Material), 256);
    }

private:
    Pipeline m_pipeline;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
};


#endif //CHERRYSPROUT_FORWARD_H