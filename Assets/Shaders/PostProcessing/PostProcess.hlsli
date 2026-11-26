#ifndef H_POST_PROCESS_H
#define H_POST_PROCESS_H

#include "PostProcessing/ToneMapping.hlsli"

float3 PostProcess(float3 v, float exposure, float whitePoint, bool tonemappingEnabled)
{
    // Exposure
    {
        v *= exposure;
    }

    // Tone-Mapping
    if (tonemappingEnabled)
    {
        TMO_ReinhardExt tmo;
        tmo.MaxWhite = whitePoint;
        v = ToneMapJodie(v, tmo);

        //TMO_Aces tmo;
        //v = ToneMap(v, tmo);
    }

    // Gamma-Correct
    {
        v = LRGB_to_SRGB(v);
    }

    return v;
}

#endif