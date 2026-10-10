#ifndef H_SET_TEX_GRADIENTS_H
#define H_SET_TEX_GRADIENTS_H

void SetTexGradients(uint2 pixelCoord, Gradients gradients)
{
    if (FEAT_CORE(ScreenSpaceGradients))
        return;

    gradients.XForward /= float(gSettings.SPP);
    gradients.XBackward /= float(gSettings.SPP);
    gradients.YForward /= float(gSettings.SPP);
    gradients.YBackward /= float(gSettings.SPP);

    AccumulateGradientsAndFetch(pixelCoord, gradients);
}

#endif