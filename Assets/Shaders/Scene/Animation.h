#ifndef H_SCENE_ANIMATION_H
#define H_SCENE_ANIMATION_H

#include "Utils/HlslGlue.h"

// TODO: Structs may need to be padded if used on GPU

struct AnimationClip
{
    float Duration;

    hlsl::uint MegaBufferKeyFrameOffset;
    hlsl::uint MegaBufferKeyFrameCount;
    hlsl::uint BoneCount;
};

struct KeyFrame
{
    float Translation[3];
    float Rotation[4];
    float Scale[3];
};

#define MAX_BONE_WEIGHTS_PER_VERTEX 4
struct VertexBoneWeights
{
    hlsl::uint BoneIndices[MAX_BONE_WEIGHTS_PER_VERTEX];
    float BoneWeights[MAX_BONE_WEIGHTS_PER_VERTEX];
};

struct BoneInfo
{
    hlsl::uint BoneParentIdxLocal;
    hlsl::float4x4 InverseBindTransform; // TODO: Can be affine3x4
};

struct BoneTransform
{
    hlsl::float4x4 Transform; // TODO: Can be affine3x4
};

struct DualQuaternion
{
    hlsl::float4 Real;
    hlsl::float4 Dual;
};

enum class AnimationType : uint32_t
{
    eNone,
    eLBS,
    eDQS,
    eCOR,

    eCount
};

#ifdef __cplusplus
static const char* s_animTypeNames[static_cast<uint32_t>(AnimationType::eCount)] = {
    "None", "LBS", "DQS", "CoR"
};
#endif

#endif