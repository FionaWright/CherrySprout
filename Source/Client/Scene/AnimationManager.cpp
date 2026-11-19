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
    m_cpuMegaBufferBoneTransforms.resize(scene->CPU.MegaBufferBonesCount);

    m_gpuMegaBufferBoneTransforms.Reset();

    const size_t boneTransformBytes = scene->CPU.MegaBufferBonesCount * sizeof(BoneTransform);
    m_gpuMegaBufferBoneTransforms.Init_Buffer("Bone Transforms", d3d->GetDevice(), boneTransformBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferVertexBind, scene->CPU.MegaBufferVertexCount, sizeof(Vertex));
    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 1, &m_gpuMegaBufferBoneTransforms, scene->CPU.MegaBufferBonesCount, sizeof(BoneTransform));
    m_setSkinning.SetSRV_Buffer(d3d->GetDevice(), 0, 2, &scene->GPU.MegaBufferVertexBoneWeights, scene->CPU.MegaBufferVertexCount, sizeof(VertexBoneWeights));

    m_setSkinning.SetUAV_Buffer(d3d->GetDevice(), 0, 0, &scene->GPU.MegaBufferVertexSkinned, scene->CPU.MegaBufferVertexCount, sizeof(Vertex));

    m_boneUploadHeap.Init(d3d->GetDevice(), scene->CPU.MegaBufferBonesCount * sizeof(BoneTransform));
}

void AnimationManager::Update(const double deltaTime_s)
{
    m_time_s += deltaTime_s;
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

            const double timeWrapped = fmod(m_time_s, clip.Duration);
            const double clipPercent = timeWrapped / clip.Duration;

            const uint32_t localIdx0 = std::floor(clipPercent * clip.MegaBufferKeyFrameCount);
            const uint32_t globalIdx0 = localIdx0 + clip.MegaBufferKeyFrameOffset;
            const uint32_t globalIdx1 = (localIdx0 == clip.MegaBufferKeyFrameCount - 1) ? 0 : globalIdx0 + clip.BoneCount;

            const double timeAt0 = (localIdx0 / float(clip.MegaBufferKeyFrameCount)) * clip.Duration;
            const double timeAt1 = ((localIdx0+1) / float(clip.MegaBufferKeyFrameCount)) * clip.Duration;
            const double timeT = (timeWrapped - timeAt0) / (timeAt1 - timeAt0);

            CherryAssert(timeAt0 != timeAt1);

            // TODO: Need to reorder joints topologically in scene loader
            for (uint32_t localBoneIdx = 0; localBoneIdx < clip.BoneCount; localBoneIdx++)
            {
                const uint32_t globalBoneIdx = localBoneIdx + obj.MegaBufferBoneOffset;

                const BoneInfo& boneInfo = scene->CPU.MegaBufferBoneInfos[localBoneIdx];

                const uint32_t globalKeyFrameIdx0 = globalIdx0 + localBoneIdx;
                const uint32_t globalKeyFrameIdx1 = globalIdx1 + localBoneIdx;

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

                const float sx = std::lerp(keyFrame0.Scale[0], keyFrame1.Scale[0], timeT);
                const float sy = std::lerp(keyFrame0.Scale[1], keyFrame1.Scale[1], timeT);
                const float sz = std::lerp(keyFrame0.Scale[2], keyFrame1.Scale[2], timeT);
                const XMMATRIX S = XMMatrixScaling(sx, sy, sz);

                const XMMATRIX TRS = T * R * S; // TODO: Order?

                XMMATRIX finalMatrix = TRS;

                if (boneInfo.BoneParentIdxLocal != UINT_MAX)
                {
                    const uint32_t globalParentBoneIdx = boneInfo.BoneParentIdxLocal + obj.MegaBufferBoneOffset;

                    const XMMATRIX parentTransform = XMLoadFloat4x4(&m_cpuMegaBufferBoneTransforms[globalParentBoneIdx].Transform);
                    finalMatrix = finalMatrix * parentTransform;
                }

                const XMMATRIX inverseBind = XMLoadFloat4x4(&boneInfo.InverseBindTransform);
                finalMatrix = inverseBind * finalMatrix;

                XMStoreFloat4x4(&m_cpuMegaBufferBoneTransforms[globalBoneIdx].Transform, finalMatrix);
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
    }
}
