#ifndef H_DQS_H
#define H_DQS_H

float4 QMul(float4 a, float4 b)
{
    return float4(
        a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz),
        a.w * b.w - dot(a.xyz, b.xyz)
    );
}

float3 DQRotate(float4 q, float3 v)
{
    return v + 2.0f * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

float3 DQTranslation(DualQuaternion dq)
{
    float4 conjugateReal = float4(-dq.Real.xyz, dq.Real.w);
    float4 translationQuat = 2.0f * QMul(dq.Dual, conjugateReal);

    return translationQuat.xyz;
}

Vertex SkinDQS(VertexBoneWeights boneWeights, Vertex vertexBind)
{
    DualQuaternion skinningQuats = (DualQuaternion)0;
    float4 dualQuatReference = 0.0f;

    bool foundBones = false;

    uint boneCount, _2;
    gMegaBufferDualQuaternions.GetDimensions(boneCount, _2);

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

        DualQuaternion dq = gMegaBufferDualQuaternions[globalBoneIdx];

        if (!foundBones)
        {
            dualQuatReference = dq.Real;
        }
        else if (dot(dualQuatReference, dq.Real) < 0.0f)
        {
            dq.Real = -dq.Real;
            dq.Dual = -dq.Dual;
        }

        skinningQuats.Real += dq.Real * weight;
        skinningQuats.Dual += dq.Dual * weight;

        foundBones = true;
    }

    float len = length(skinningQuats.Real);

    if (!foundBones || len < 1e-6f)
        return vertexBind;

    skinningQuats.Real /= len;
    skinningQuats.Dual /= len;

    skinningQuats.Dual -= skinningQuats.Real * dot(skinningQuats.Real, skinningQuats.Dual);

    Vertex vertexSkinned;
    vertexSkinned.UV = vertexBind.UV;

    float3 translation = DQTranslation(skinningQuats);

    vertexSkinned.Position = DQRotate(skinningQuats.Real, vertexBind.Position) + translation;
    vertexSkinned.Normal = normalize(DQRotate(skinningQuats.Real, vertexBind.Normal));

    return vertexSkinned;
}

#endif