#ifndef H_SET_TEX_GRADIENTS_H
#define H_SET_TEX_GRADIENTS_H

void SetTexGradients(uint2 pixelCoord, float3 gradientX, float3 gradientY)
{
    if (FEATURE_ENABLED(ScreenSpaceGradients))
    {
        float3 c0 = gTexAccumulation[pixelCoord].rgb;
        float3 cX = gTexAccumulation[pixelCoord + uint2(1,0)].rgb;
        float3 cY = gTexAccumulation[pixelCoord + uint2(0,1)].rgb;

        gradientX = cX - c0;
        gradientY = cY - c0;
        AccumulateGradients(pixelCoord, gradientX, gradientY);

        DBG_OUTPUT3(gradientX, GD_GradientX);
        DBG_OUTPUT3(gradientY, GD_GradientY);
        return;
    }

    gradientX /= float(gSettings.SPP);
    gradientY /= float(gSettings.SPP);
    AccumulateGradients(pixelCoord, gradientX, gradientY);

    DBG_OUTPUT3(gradientX, GD_GradientX);
    DBG_OUTPUT3(gradientY, GD_GradientY);
}

#endif