#ifndef H_SCENE_ANIMATION_H
#define H_SCENE_ANIMATION_H

#include "Utils/HlslGlue.h"

// TODO: Structs may need to be padded if used on GPU

struct AnimationClip
{
    float Duration;

    hlsl::uint MegaBufferKeyFrameOffset;
    hlsl::uint MegaBufferKeyCount;
    hlsl::uint BoneCount;
};

struct KeyFrame
{
    float Translation[3];
    float Rotation[4];
    float Scale[3];
};

struct VertexBoneWeights
{
    hlsl::uint BoneIndices[4];
    float BoneWeights[4];
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

#endif