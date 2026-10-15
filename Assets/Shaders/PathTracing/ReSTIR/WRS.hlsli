#ifndef H_RIS_H
#define H_RIS_H

template<typename T, typename TInfo>
bool ReservoirUpdate(
    inout RngInfo rngInfo,
    inout Reservoir<T,TInfo> reservoir,
    T X_i,
    float w,
    float c,
    float pHat
)
{
    if (w <= 0.0f)
        return false;

    reservoir.Confidence += c;
    reservoir.WeightSum += w;

    float r = Rand01(rngInfo);
    float norm_w = w / reservoir.WeightSum;
    if (r < norm_w)
    {
        reservoir.Y = X_i;
        reservoir.PHat = pHat;
        return true;
    }
    return false;
}

template<typename T, typename TInfo>
void ReservoirMerge(
    inout RngInfo rngInfo,
    uint ConfidenceCap,
    inout Reservoir<T,TInfo> reservoir,
    Reservoir<T,TInfo> other,
    float w
)
{
    if (w <= 0.0f)
        return;

    reservoir.WeightSum += w;

    float r = Rand01(rngInfo);
    float norm_w = w / reservoir.WeightSum;
    if (r < norm_w)
    {
        reservoir.Y = other.Y;
        reservoir.PHat = other.PHat;
    }

    reservoir.W_Y = reservoir.PHat <= 0.0f ? 0.0f : reservoir.WeightSum / reservoir.PHat;

    reservoir.Confidence += other.Confidence;
    reservoir.Confidence = min(reservoir.Confidence, ConfidenceCap);
}

template<typename T>
float RestirMis(T X_i, uint M)
{
    return 1 / (float)M;
}

template<typename T, typename TInfo>
void WRS(
    inout RngInfo rngInfo,
    inout Reservoir<T,TInfo> reservoir,
    uint M,
    uint ConfidenceCap,
    TInfo tInfo
)
{
    bool candidateSelected = false;

    reservoir.WeightSum = 0.0f;
    reservoir.Confidence = 0.0f;

    reservoir.Info = tInfo;

    for (uint i = 0; i < M; i++)
    {
        T X_i = Generate(rngInfo, tInfo);

        float pHat = Target(X_i, tInfo);
        float pdf = PDF(X_i, tInfo);
        if (pdf <= 0.0f)
            continue;

        float W_X_i = 1.0f / pdf;

        float m = RestirMis(X_i, M);
        float w = pHat * W_X_i * m;
        float c = 1;

        candidateSelected |= ReservoirUpdate<T,TInfo>(rngInfo, reservoir, X_i, w, c, pHat);
    }

    if (!candidateSelected)
    {
        reservoir.Confidence = 0.0f;
        reservoir.W_Y = 0.0f;
        return;
    }

    reservoir.Confidence = min(reservoir.Confidence, ConfidenceCap);
    reservoir.W_Y = reservoir.WeightSum / reservoir.PHat;
}

#endif