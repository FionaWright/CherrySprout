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
    m_rootSigSkinning.AddSrvRange(0, 4);
    m_rootSigSkinning.AddUavRange(0, 1);
    m_rootSigSkinning.Init(d3d->GetDevice());

    V(m_pipelineSkinning.InitCompute(d3d->GetDevice(), "Compute/SkinningCS.hlsl", m_rootSigSkinning.Get()));

    m_setSkinning.Init(heap);
    m_setSkinning.SetHasRootConstants();
}

void AnimationManager::LoadSceneData(const D3D* d3d, Scene* scene)
{
    CherryPrint("AnimationManager: LoadSceneData");

    m_cpuMegaBufferBoneTransformsLBS.resize(scene->CPU.MegaBufferBonesCount);
    m_cpuMegaBufferBoneTransformsDQS.resize(scene->CPU.MegaBufferBonesCount);

    m_gpuMegaBufferBoneTransformsLBS.Reset();
    m_gpuMegaBufferBoneTransformsDQS.Reset();

    {
        const size_t boneTransformBytes = scene->CPU.MegaBufferBonesCount * sizeof(BoneTransform);
        m_gpuMegaBufferBoneTransformsLBS.Init_Buffer("Bone Transforms", d3d->GetDevice(), boneTransformBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }

    {
        const size_t boneTransformBytes = scene->CPU.MegaBufferBonesCount * sizeof(DualQuaternion);
        m_gpuMegaBufferBoneTransformsDQS.Init_Buffer("Dual Quaternions", d3d->GetDevice(), boneTransformBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }

    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferVertexBind, scene->CPU.MegaBufferVertexCount, sizeof(Vertex));
    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 1, &m_gpuMegaBufferBoneTransformsLBS, scene->CPU.MegaBufferBonesCount, sizeof(BoneTransform));
    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 2, &m_gpuMegaBufferBoneTransformsDQS, scene->CPU.MegaBufferBonesCount, sizeof(DualQuaternion));
    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 3, &scene->GPU.MegaBufferVertexBoneWeights, scene->CPU.MegaBufferVertexCount, sizeof(VertexBoneWeights));

    m_setSkinning.SetUAV_Buffer(d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferVertexSkinned, scene->CPU.MegaBufferVertexCount, sizeof(Vertex));

    m_boneUploadHeap.Reset();
    m_boneUploadHeap.Init(d3d->GetDevice(), scene->CPU.MegaBufferBonesCount * sizeof(BoneTransform));
}

void AnimationManager::Update(const double deltaTime_s)
{
#define ANIMATION_SPEED 1.0f
    m_time_s += deltaTime_s * ANIMATION_SPEED;
}

DualQuaternion toDualQuaternion(FXMVECTOR rotation, FXMVECTOR translation)
{
    const XMVECTOR real = XMQuaternionNormalize(rotation);

    const XMVECTOR translationQuat = XMVectorSet(
        XMVectorGetX(translation),
        XMVectorGetY(translation),
        XMVectorGetZ(translation),
        0.0f
    );

    const XMVECTOR dual = XMVectorScale(
        XMQuaternionMultiply(real, translationQuat),
        0.5f
    );

    DualQuaternion result;
    XMStoreFloat4(&result.Real, real);
    XMStoreFloat4(&result.Dual, dual);
    return result;
}

void AnimationManager::Skin(ID3D12GraphicsCommandList* cmdList, Scene* scene, Heap* heap, AnimationType animType)
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

            std::vector<XMMATRIX> topologicalTransforms;
            topologicalTransforms.resize(clip.BoneCount);

            for (uint32_t localBoneIdx = 0; localBoneIdx < clip.BoneCount; localBoneIdx++)
            {
                const uint32_t globalBoneIdx = localBoneIdx + obj.MegaBufferBoneOffset;

                const BoneInfo& boneInfo = scene->CPU.MegaBufferBoneInfos[globalBoneIdx];

                // Keyframe Interpolation
                XMMATRIX TRS;
                {
                    const uint32_t globalKeyFrameIdx0 = globalKeyFrameBaseIdx0 + localBoneIdx;
                    const uint32_t globalKeyFrameIdx1 = globalKeyFrameBaseIdx1 + localBoneIdx;

                    const KeyFrame& keyFrame0 = scene->CPU.MegaBufferKeyFrames[globalKeyFrameIdx0];
                    const KeyFrame& keyFrame1 = scene->CPU.MegaBufferKeyFrames[globalKeyFrameIdx1];

                    const float tx = std::lerp(keyFrame0.Translation[0], keyFrame1.Translation[0], timeT);
                    const float ty = std::lerp(keyFrame0.Translation[1], keyFrame1.Translation[1], timeT);
                    const float tz = std::lerp(keyFrame0.Translation[2], keyFrame1.Translation[2], timeT);
                    const XMMATRIX T = XMMatrixTranslation(tx, ty, tz);

                    const XMVECTOR q0 = XMVectorSet(keyFrame0.Rotation[0], keyFrame0.Rotation[1], keyFrame0.Rotation[2], keyFrame0.Rotation[3]);
                    const XMVECTOR q1 = XMVectorSet(keyFrame1.Rotation[0], keyFrame1.Rotation[1], keyFrame1.Rotation[2], keyFrame1.Rotation[3]);
                    const XMVECTOR r = XMQuaternionSlerp(q0, q1, static_cast<float>(timeT));
                    const XMMATRIX R = XMMatrixRotationQuaternion(r);

                    XMMATRIX S = XMMatrixIdentity();
                    if (animType != AnimationType::eDQS)
                    {
                        const float sx = std::lerp(keyFrame0.Scale[0], keyFrame1.Scale[0], timeT);
                        const float sy = std::lerp(keyFrame0.Scale[1], keyFrame1.Scale[1], timeT);
                        const float sz = std::lerp(keyFrame0.Scale[2], keyFrame1.Scale[2], timeT);
                        S = XMMatrixScaling(sx, sy, sz);
                    }

                    TRS = S * R * T;
                }

                XMMATRIX transform = TRS;

                if (boneInfo.BoneParentIdxLocal != UINT_MAX)
                {
                    CherryAssert(boneInfo.BoneParentIdxLocal < localBoneIdx);

                    transform *= topologicalTransforms[boneInfo.BoneParentIdxLocal];
                }

                topologicalTransforms[localBoneIdx] = transform;
            }

            for (uint32_t localBoneIdx = 0; localBoneIdx < clip.BoneCount; localBoneIdx++)
            {
                const uint32_t globalBoneIdx = localBoneIdx + obj.MegaBufferBoneOffset;
                const BoneInfo& boneInfo = scene->CPU.MegaBufferBoneInfos[globalBoneIdx];

                const XMMATRIX inverseBind = XMLoadFloat4x4(&boneInfo.InverseBindTransform);
                const XMMATRIX transform = inverseBind * topologicalTransforms[localBoneIdx];

                if (animType == AnimationType::eLBS)
                {
                    XMStoreFloat4x4(&m_cpuMegaBufferBoneTransformsLBS[globalBoneIdx].Transform, transform);
                }
                else if (animType == AnimationType::eDQS)
                {
                    XMVECTOR scale;
                    XMVECTOR rotation;
                    XMVECTOR translation;
                    if (!XMMatrixDecompose(&scale, &rotation, &translation, transform))
                        continue;
                    rotation = XMQuaternionNormalize(rotation);

                    m_cpuMegaBufferBoneTransformsDQS[globalBoneIdx] = toDualQuaternion(rotation, translation);
                }
            }
        }

        m_boneUploadHeap.FreeAssignedData();

        if (animType == AnimationType::eLBS)
        {
            m_gpuMegaBufferBoneTransformsLBS.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

            const size_t boneTransformBytes = scene->CPU.MegaBufferBonesCount * sizeof(BoneTransform);
            m_gpuMegaBufferBoneTransformsLBS.UploadBuffer(cmdList, &m_boneUploadHeap, m_cpuMegaBufferBoneTransformsLBS.data(), boneTransformBytes);
        }
        else if (animType == AnimationType::eDQS)
        {
            m_gpuMegaBufferBoneTransformsDQS.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

            const size_t boneTransformBytes = scene->CPU.MegaBufferBonesCount * sizeof(DualQuaternion);
            m_gpuMegaBufferBoneTransformsDQS.UploadBuffer(cmdList, &m_boneUploadHeap, m_cpuMegaBufferBoneTransformsDQS.data(), boneTransformBytes);
        }
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

            CbvSkinning cbv{};
            cbv.AnimType = animType;
            cbv.MegaBufferVertexCount = obj.VertexCount;
            cbv.MegaBufferVertexOffset = obj.MegaBufferVertexOffset;
            cbv.MegaBufferBonesOffset = obj.MegaBufferBoneOffset;

            m_rootConstantsSkinning.Bind_Compute(cmdList, &cbv);

            DispatchOverRegion(cmdList, 64, obj.VertexCount);
        }

        scene->GPU.MegaBufferVertexSkinned.UavBarrier(cmdList);
    }
}
