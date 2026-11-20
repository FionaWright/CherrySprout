#include "Utils/HlslUtils.hlsli"
#include "Utils/CBVs.h"
#include "Utils/Constants.h"
#include "Scene/Vertex.h"
#include "Scene/Animation.h"

StructuredBuffer<Vertex> gMegaBufferVertexBind : register(t0);
StructuredBuffer<BoneTransform> gMegaBufferBoneTransforms : register(t1);
StructuredBuffer<VertexBoneWeights> gMegaBufferBoneWeights : register(t2);

RWStructuredBuffer<Vertex> gMegaBufferVertexSkinned : register(u0);

ConstantBuffer<CbvSkinning> gCBV : register(b0);

[numthreads(64, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint localVertexIdx = DTid.x;
    uint globalVertexIdx = localVertexIdx + gCBV.MegaBufferVertexOffset;

    uint size, _;
    gMegaBufferVertexBind.GetDimensions(size, _);

    if (localVertexIdx >= gCBV.MegaBufferVertexCount || globalVertexIdx >= size)
        return;

    VertexBoneWeights boneWeights = gMegaBufferBoneWeights[globalVertexIdx];

    float4x4 skinningTransform = (float4x4)0;

    bool foundBones = false;

    [unroll]
    for (int i = 0; i < MAX_BONE_WEIGHTS_PER_VERTEX; i++)
    {
        uint localBoneIdx = boneWeights.BoneIndices[i];
        float weight = boneWeights.BoneWeights[i];

        if (localBoneIdx == UINT_MAX || weight <= 0.0f)
            break;

        uint globalBoneIdx = localBoneIdx + gCBV.MegaBufferBonesOffset;

        uint boneCount, _2;
        gMegaBufferBoneTransforms.GetDimensions(boneCount, _2);
        if (globalBoneIdx >= boneCount)
            continue;

        float4x4 boneTransform = gMegaBufferBoneTransforms[globalBoneIdx].Transform;
        skinningTransform += boneTransform * weight;

        foundBones = true;
    }

    if (!foundBones)
        skinningTransform = float4x4(1, 0, 0, 0,
                                     0, 1, 0, 0,
                                     0, 0, 1, 0,
                                     0, 0, 0, 1);

    Vertex vertexBind = gMegaBufferVertexBind[globalVertexIdx];
    Vertex vertexSkinned;

    vertexSkinned.Position = mul(skinningTransform, float4(vertexBind.Position, 1.0f)).xyz;
    vertexSkinned.Normal = normalize(mul((float3x3)skinningTransform, vertexBind.Normal));
	vertexSkinned.UV = vertexBind.UV;

    gMegaBufferVertexSkinned[globalVertexIdx] = vertexSkinned;
}