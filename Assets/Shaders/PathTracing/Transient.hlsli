#ifndef H_TRANSIENT_H
#define H_TRANSIENT_H

#if FEAT_CORE_D(Transient)

enum TransientMode : uint
{
    eUniform,
    eGaussian,
};

#define TRANSIENT_MODE TransientMode::eGaussian

float GetTransientFactor(float pathDistance)
{
    if (TRANSIENT_MODE == TransientMode::eUniform)
    {
        return pathDistance >= gSettings.TransientDistanceSinceStart &&
             pathDistance < gSettings.TransientDistanceSinceStart + gSettings.TransientPulseDistance;
    }

    float halfRange = gSettings.TransientPulseDistance * 0.5f;
    float mid = gSettings.TransientDistanceSinceStart + halfRange;
    float sigma = halfRange * 0.35f;
    float d = pathDistance - mid;
    return exp(-0.5f * d * d / (sigma * sigma));
}

#else

float GetTransientFactor(float pathDistance) { return NAN; }

#endif

#endif