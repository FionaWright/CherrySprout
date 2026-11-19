//
// Created by fionaw on 18/11/2026.
//

#include "System/pch.h"
#include "Scene/AnimationManager.h"

#include <unordered_set>

#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

void AnimationManager::Init(const D3D* d3d, Heap* heap)
{
    m_rootConstantsSkinning.Init(0, 0, sizeof(CbvSkinning));

    m_rootSigSkinning.AddRootConstants(&m_rootConstantsSkinning);
    m_rootSigSkinning.AddSrvRange(0, 3);
    m_rootSigSkinning.AddUavRange(0, 1);
    m_rootSigSkinning.Init(d3d->GetDevice());

    V(m_pipelineSkinning.InitCompute(d3d->GetDevice(), "Compute/SkinningCS.hlsl", m_rootSigSkinning.Get()));

    m_setSkinning.Init(heap);
    m_setSkinning.SetHasRootConstants();
}

void AnimationManager::LoadSceneData(const D3D* d3d, Scene* scene)
{
    CherryPrint("AnimationManager: LoadSceneData");

    m_cpuMegaBufferBoneTransforms.resize(scene->CPU.MegaBufferBonesCount);

    m_gpuMegaBufferBoneTransforms.Reset();

    const size_t boneTransformBytes = scene->CPU.MegaBufferBonesCount * sizeof(BoneTransform);
    m_gpuMegaBufferBoneTransforms.Init_Buffer("Bone Transforms", d3d->GetDevice(), boneTransformBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferVertexBind, scene->CPU.MegaBufferVertexCount, sizeof(Vertex));
    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 1, &m_gpuMegaBufferBoneTransforms, scene->CPU.MegaBufferBonesCount, sizeof(BoneTransform));
    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 2, &scene->GPU.MegaBufferVertexBoneWeights, scene->CPU.MegaBufferVertexCount, sizeof(VertexBoneWeights));

    m_setSkinning.SetUAV_Buffer(d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferVertexSkinned, scene->CPU.MegaBufferVertexCount, sizeof(Vertex));

    m_boneUploadHeap.Reset();
    m_boneUploadHeap.Init(d3d->GetDevice(), scene->CPU.MegaBufferBonesCount * sizeof(BoneTransform));
}

void AnimationManager::Update(const double deltaTime_s)
{
#define ANIMATION_SPEED 1.0f
    m_time_s += deltaTime_s * ANIMATION_SPEED;
}

void AnimationManager::Skin(ID3D12GraphicsCommandList* cmdList, Scene* scene, Heap* heap)
{
    // Bone Pass
    {
        std::unordered_set<uint32_t> coveredSkeletons;

        for (uint32_t i = 0; i < scene->CPU.ObjectCount; i++)
        {
            const Object& obj = scene->CPU.Objects[i];
            if (obj.SelectedAnimationClipIdx < 0)
                continue;

            if (coveredSkeletons.contains(obj.MegaBufferBoneOffset))
                continue;
            coveredSkeletons.insert(obj.MegaBufferBoneOffset);

            const uint32_t globalClipIdx = obj.SelectedAnimationClipIdx + obj.MegaBufferAnimationClipOffset;
            const AnimationClip& clip = scene->CPU.MegaBufferAnimationClips[globalClipIdx];

            CherryAssert(clip.BoneCount == obj.BoneCount);

            double timeWrapped = fmod(m_time_s, clip.Duration);
            if (timeWrapped < 0.0)
                timeWrapped += clip.Duration;

            const double clipPercent = timeWrapped / clip.Duration;
            const double framePosition = clipPercent * clip.MegaBufferKeyFrameCount;

            const uint32_t localKeyFrameIdx0 = static_cast<uint32_t>(std::floor(framePosition));

            const uint32_t globalKeyFrameBaseIdx0 = (localKeyFrameIdx0 * clip.BoneCount) + clip.MegaBufferKeyFrameOffset;

            const uint32_t globalKeyFrameBaseIdx1 =
                (localKeyFrameIdx0 == clip.MegaBufferKeyFrameCount - 1) ? clip.MegaBufferKeyFrameOffset
                : globalKeyFrameBaseIdx0 + clip.BoneCount;

            const float timeT = framePosition - static_cast<float>(localKeyFrameIdx0);

            //CherryPrint("Time: " << timeWrapped << ", percent: " << clipPercent << ", timeT: " << timeT);
            //CherryPrint("Local KeyFrame Idx (0): " << localKeyFrameIdx0);
            //CherryPrint("Global KeyFrame Base (0)" << globalKeyFrameBaseIdx0);
            //CherryPrint("Global KeyFrame Base (1)" << globalKeyFrameBaseIdx1);

            std::vector<XMMATRIX> topologicalTransforms;
            topologicalTransforms.resize(clip.BoneCount);

            // TODO: Need to reorder joints topologically in scene loader
            for (uint32_t localBoneIdx = 0; localBoneIdx < clip.BoneCount; localBoneIdx++)
            {
                //CherryPrint("Bone: " << localBoneIdx);

                //PrintMatrix("Model Matrix", obj.M);

                const uint32_t globalBoneIdx = localBoneIdx + obj.MegaBufferBoneOffset;

                const BoneInfo& boneInfo = scene->CPU.MegaBufferBoneInfos[globalBoneIdx];

                // Keyframe Interpolation (LBS)
                XMMATRIX TRS;
                {
                    const uint32_t globalKeyFrameIdx0 = globalKeyFrameBaseIdx0 + localBoneIdx;
                    const uint32_t globalKeyFrameIdx1 = globalKeyFrameBaseIdx1 + localBoneIdx;

                    const KeyFrame& keyFrame0 = scene->CPU.MegaBufferKeyFrames[globalKeyFrameIdx0];
                    const KeyFrame& keyFrame1 = scene->CPU.MegaBufferKeyFrames[globalKeyFrameIdx1];

                    //CherryPrint("KeyFrame 0 Rotation: " << keyFrame0.Rotation[0] << ", " << keyFrame0.Rotation[1] << ", " << keyFrame0.Rotation[2] << ", " << keyFrame0.Rotation[3]);
                    //CherryPrint("KeyFrame 1 Rotation: " << keyFrame1.Rotation[0] << ", " << keyFrame1.Rotation[1] << ", " << keyFrame1.Rotation[2] << ", " << keyFrame1.Rotation[3]);

                    const float tx = std::lerp(keyFrame0.Translation[0], keyFrame1.Translation[0], timeT);
                    const float ty = std::lerp(keyFrame0.Translation[1], keyFrame1.Translation[1], timeT);
                    const float tz = std::lerp(keyFrame0.Translation[2], keyFrame1.Translation[2], timeT);
                    const XMMATRIX T = XMMatrixTranslation(tx, ty, tz);

                    const XMVECTOR q0 = XMVectorSet(keyFrame0.Rotation[0], keyFrame0.Rotation[1], keyFrame0.Rotation[2], keyFrame0.Rotation[3]);
                    const XMVECTOR q1 = XMVectorSet(keyFrame1.Rotation[0], keyFrame1.Rotation[1], keyFrame1.Rotation[2], keyFrame1.Rotation[3]);
                    const XMVECTOR r = XMQuaternionSlerp(q0, q1, static_cast<float>(timeT));
                    const XMMATRIX R = XMMatrixRotationQuaternion(r);

                    const float sx = std::lerp(keyFrame0.Scale[0], keyFrame1.Scale[0], timeT);
                    const float sy = std::lerp(keyFrame0.Scale[1], keyFrame1.Scale[1], timeT);
                    const float sz = std::lerp(keyFrame0.Scale[2], keyFrame1.Scale[2], timeT);
                    const XMMATRIX S = XMMatrixScaling(sx, sy, sz);

                    //PrintMatrix("Translation", T);
                    //PrintMatrix("Rotation", R);
                    //PrintMatrix("Scale", S);

                    TRS = S * R * T; // Correct
                }

                //PrintMatrix("TRS", TRS);

                XMMATRIX transform = TRS;

                if (boneInfo.BoneParentIdxLocal != UINT_MAX)
                {
                    transform = transform * topologicalTransforms[boneInfo.BoneParentIdxLocal]; // Correct

                    //PrintMatrix("Parent", topologicalTransforms[boneInfo.BoneParentIdxLocal]);
                    //PrintMatrix("TRS * Parent", transform);
                }

                topologicalTransforms[localBoneIdx] = transform;
            }

            for (uint32_t localBoneIdx = 0; localBoneIdx < clip.BoneCount; localBoneIdx++)
            {
                //CherryPrint("Bone: " << localBoneIdx);

                const uint32_t globalBoneIdx = localBoneIdx + obj.MegaBufferBoneOffset;
                const BoneInfo& boneInfo = scene->CPU.MegaBufferBoneInfos[globalBoneIdx];

                const XMMATRIX inverseBind = XMLoadFloat4x4(&boneInfo.InverseBindTransform);
                const XMMATRIX transform = inverseBind * topologicalTransforms[localBoneIdx]; // Correct

                //PrintMatrix("Inverse Bind", inverseBind);
                //PrintMatrix("Final Bone Transform", transform);

                XMStoreFloat4x4(&m_cpuMegaBufferBoneTransforms[globalBoneIdx].Transform, transform);
            }
        }

        m_boneUploadHeap.FreeAssignedData();

        m_gpuMegaBufferBoneTransforms.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        const size_t boneTransformBytes = scene->CPU.MegaBufferBonesCount * sizeof(BoneTransform);
        m_gpuMegaBufferBoneTransforms.UploadBuffer(cmdList, &m_boneUploadHeap, m_cpuMegaBufferBoneTransforms.data(), boneTransformBytes);
    }

    // Skinning Pass
    {
        heap->Bind(cmdList);

        cmdList->SetComputeRootSignature(m_rootSigSkinning.Get());
        cmdList->SetPipelineState(m_pipelineSkinning.GetPSO());

        scene->GPU.MegaBufferVertexSkinned.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_setSkinning.TransitionAllSRVToShaderResource(cmdList);
        m_setSkinning.SetDescriptorTables_Compute(cmdList);

        for (uint32_t i = 0; i < scene->CPU.ObjectCount; i++)
        {
            const Object& obj = scene->CPU.Objects[i];

            CbvSkinning cbv;
            cbv.MegaBufferVertexCount = obj.VertexCount;
            cbv.MegaBufferVertexOffset = obj.MegaBufferVertexOffset;
            cbv.MegaBufferBonesOffset = obj.MegaBufferBoneOffset;

            m_rootConstantsSkinning.Bind_Compute(cmdList, &cbv);

            DispatchOverRegion(cmdList, 64, obj.VertexCount);
        }

        scene->GPU.MegaBufferVertexSkinned.UavBarrier(cmdList);
    }
}
