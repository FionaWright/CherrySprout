#ifndef H_SET_TEX_GRADIENTS_H
#define H_SET_TEX_GRADIENTS_H

void SetTexGradients(uint2 pixelCoord, float3 gradientX, float3 gradientY)
{
    if (FEATURE_ENABLED(ScreenSpaceGradients))
        return;

    gradientX /= float(gSettings.SPP);
    gradientY /= float(gSettings.SPP);

    float3 gSumX, gSumY;
    AccumulateGradientsAndFetch(pixelCoord, gradientX, gradientY, gSumX, gSumY);

    DBG_OUTPUT3(gSumX, GD_GradientX);
    DBG_OUTPUT3(gSumY, GD_GradientY);
}

#endif