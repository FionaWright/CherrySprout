#ifndef H_RIS_H
#define H_RIS_H

float GetWeight(float pHat, float c, float W)
{
    return pHat * c * W;
}

template<typename T, typename TInfo>
bool ReservoirUpdate(
    inout RngInfo rngInfo,
    inout Reservoir<T,TInfo> reservoir,
    T X_i,
    float w,
    float c,
    float pHat)
{
    reservoir.Confidence += c;

    if (w <= 0.0f)
        return false;

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
bool ReservoirMerge(
    inout RngInfo rngInfo,
    float ConfidenceCap,
    inout Reservoir<T,TInfo> reservoir,
    Reservoir<T,TInfo> other)
{
    //float c = min(other.Confidence, reservoir.Confidence * ConfidenceCap);
    float c = min(other.Confidence, ConfidenceCap);
    float pHat = Target(other.Y, reservoir.Info);
    float w = GetWeight(pHat, c, other.W_Y);

    //float m = BalanceHeuristic(reservoir.PHat, pHat, reservoir.Confidence, c);
    //w *= m;

    reservoir.Confidence += c;

    if (w <= 0.0f)
        return false;

    reservoir.WeightSum += w;

    float r = Rand01(rngInfo);
    float norm_w = w / reservoir.WeightSum;
    if (r < norm_w)
    {
        reservoir.Y = other.Y;
        reservoir.PHat = pHat;
        return true;
    }
    return false;
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
    TInfo tInfo)
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

        float W_X_i = pdf <= 0.0f ? 0.0f : 1.0f / pdf;

        //float m = RestirMis(X_i, M);
        float c = 1;
        float w = GetWeight(pHat, c, W_X_i);

        candidateSelected |= ReservoirUpdate<T,TInfo>(rngInfo, reservoir, X_i, w, c, pHat);
    }

    if (!candidateSelected)
    {
        reservoir.Confidence = 0.0f;
        reservoir.W_Y = 0.0f;
        return;
    }

    reservoir.W_Y = reservoir.WeightSum / (reservoir.PHat * reservoir.Confidence);
}

#endif