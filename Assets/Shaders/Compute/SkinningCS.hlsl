#include "Utils/HlslUtils.hlsli"
#include "Utils/CBVs.h"
#include "Utils/Constants.h"
#include "Scene/Vertex.h"
#include "Scene/Animation.h"

StructuredBuffer<Vertex> gMegaBufferVertexBind : register(t0);
StructuredBuffer<BoneTransform> gMegaBufferBoneTransforms : register(t1);
StructuredBuffer<DualQuaternion> gMegaBufferDualQuaternions : register(t2);
StructuredBuffer<VertexBoneWeights> gMegaBufferBoneWeights : register(t3);

RWStructuredBuffer<Vertex> gMegaBufferVertexSkinned : register(u0);

ConstantBuffer<CbvSkinning> gCBV : register(b0);

#include "Compute/LBS.hlsli"
#include "Compute/DQS.hlsli"

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
    Vertex vertexBind = gMegaBufferVertexBind[globalVertexIdx];

    Vertex vertexSkinned;
    if (gCBV.AnimType == AnimationType::eLBS)
    {
        vertexSkinned = SkinLBS(boneWeights, vertexBind);
    }
    else if (gCBV.AnimType == AnimationType::eDQS)
    {
        vertexSkinned = SkinDQS(boneWeights, vertexBind);
    }
    else
    {
        vertexSkinned = vertexBind;
    }

    gMegaBufferVertexSkinned[globalVertexIdx] = vertexSkinned;
}