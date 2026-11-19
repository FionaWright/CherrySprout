//
// Created by fionaw on 18/11/2026.
//

#ifndef CHERRYSPROUT_ANIMATIONMANAGER_H
#define CHERRYSPROUT_ANIMATIONMANAGER_H

#include "Scene.h"
#include "HWI/D12Resource.h"
#include "HWI/D3D.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "HWI/UploadHeap.h"
#include "Scene/Animation.h"

class AnimationManager
{
public:
    void Init(const D3D* d3d, Heap* heap);
    void LoadSceneData(const D3D* d3d, Scene* scene);
    void Update(double deltaTime_s);
    void Skin(ID3D12GraphicsCommandList* cmdList, Scene* scene, Heap* heap);

private:
    UploadHeap m_boneUploadHeap;

    RootSig m_rootSigSkinning;
    Pipeline m_pipelineSkinning;
    RootConstants m_rootConstantsSkinning;
    DescriptorSet m_setSkinning;

    std::vector<BoneTransform> m_cpuMegaBufferBoneTransforms;
    D12Resource m_gpuMegaBufferBoneTransforms;

    double m_time_s = 0.0f;
};


#endif //CHERRYSPROUT_ANIMATIONMANAGER_H