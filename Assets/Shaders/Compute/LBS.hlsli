#ifndef H_LBS_H
#define H_LBS_H

Vertex SkinLBS(VertexBoneWeights boneWeights, Vertex vertexBind)
{
    float4x4 skinningTransform = (float4x4)0;

    bool foundBones = false;

    uint boneCount, _2;
    gMegaBufferBoneTransforms.GetDimensions(boneCount, _2);

    [unroll]
    for (int i = 0; i < MAX_BONE_WEIGHTS_PER_VERTEX; i++)
    {
        uint localBoneIdx = boneWeights.BoneIndices[i];
        float weight = boneWeights.BoneWeights[i];

        if (localBoneIdx == UINT_MAX || weight <= 0.0f)
            break;

        uint globalBoneIdx = localBoneIdx + gCBV.MegaBufferBonesOffset;
        if (globalBoneIdx >= boneCount)
            continue;

        float4x4 boneTransform = gMegaBufferBoneTransforms[globalBoneIdx].Transform;
        skinningTransform += boneTransform * weight;

        foundBones = true;
    }

    if (!foundBones)
        return vertexBind;

    Vertex vertexSkinned;

    vertexSkinned.Position = mul(skinningTransform, float4(vertexBind.Position, 1.0f)).xyz;
    vertexSkinned.Normal = normalize(mul((float3x3)skinningTransform, vertexBind.Normal));
	vertexSkinned.UV = vertexBind.UV;

    return vertexSkinned;
}

#endif