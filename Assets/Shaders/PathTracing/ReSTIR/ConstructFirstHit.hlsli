#ifndef H_CONSTRUCT_FIRST_HIT_H
#define H_CONSTRUCT_FIRST_HIT_H

void ConstructFirstHitFromGBuffer(float3 cameraPos, uint2 pixelCoord, out HitInfo hitInfo, out bool isMiss)
{
    hitInfo.Ng_ff = NAN;
    hitInfo.IsEntering = false;

    // Get WorldPos, Normal, UV, MaterialIdx
    // Normals already bumped by GBuffer

    if (matIdx == 0)
    {
        isMiss = true;
        return;
    }

    hitInfo.Mat = 0;
    hitInfo.UV = 0;
    hitInfo.Ns_ff = 0;

    hitInfo.RayT = length(worldPos - cameraPos);

    hitInfo.SFrame = CreateShadingFrame(hitInfo.Ns_ff);

    ApplyMaterialTextures(hitInfo);

    isMiss = false;
}

#endif