#ifndef H_TONE_MAPPING_H
#define H_TONE_MAPPING_H

// https://64.github.io/tonemapping

struct TMO_Saturate
{
    template <typename T>
    T Apply(T v)
    {
        return saturate(v);
    }
};

struct TMO_Reinhard
{
    template <typename T>
    T Apply(T v)
    {
        return v / (1 + v);
    }
};

struct TMO_ReinhardExt
{
    float MaxWhite;

    template <typename T>
    T Apply(T v)
    {
        float3 ext = (1 + (v / (MaxWhite * MaxWhite)));
        return (v / (1 + v)) * ext;
    }
};

struct TMO_Hable
{
    template <typename T>
    T Partial(T x)
    {
        float A = 0.15f;
        float B = 0.50f;
        float C = 0.10f;
        float D = 0.20f;
        float E = 0.02f;
        float F = 0.30f;
        return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
    }

    template <typename T>
    T Apply(T v)
    {
        static const float EXPOSURE_BIAS = 2.0f;
        static const T W = T(11.2f);

        return Partial(v * EXPOSURE_BIAS) / Partial(W);
    }
};

// Note: float3 only
struct TMO_ACES
{
    float3 Fit(float3 v)
    {
        float3 a = v * (v + 0.0245786) - 0.000090537;
        float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
        return a / b;
    }

    float3 Apply(float3 v)
    {
        static const float3x3 ACES_IN = float3x3(
            0.59719,  0.35458,  0.04823,
            0.07600,  0.90834,  0.01566,
            0.02840,  0.13383,  0.83777
        );

        static const float3x3 ACES_OUT = float3x3(
             1.60475, -0.53108, -0.07367,
            -0.10208,  1.10813, -0.00605,
            -0.00327, -0.07276,  1.07602
        );

        float3 v2 = mul(ACES_IN, v);
        v2 = Fit(v2);
        return mul(ACES_OUT, v2);
    }
};

template <typename TMO>
float3 ToneMap(float3 v, TMO tmo)
{
    return tmo.Apply(v);
}

template <typename TMO>
float3 ToneMapLum(float3 v, TMO tmo)
{
    float inLum = Luminance(v);
    float outLum =  tmo.Apply(inLum);

    return v * (outLum / max(1e-6f, inLum));
}

template <typename TMO>
float3 ToneMapJodie(float3 v, TMO tmo)
{
    float lum = Luminance(v);
    float3 tv = tmo.Apply(v);
    float3 k = v / (1 + lum);
    return lerp(k, tv, tv);
}

#endif
