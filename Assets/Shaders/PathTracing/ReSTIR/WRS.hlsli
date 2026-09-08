#ifndef H_RIS_H
#define H_RIS_H

template<typename T>
bool ReservoirUpdate(
    inout RngInfo rngInfo,
    inout Reservoir<T> reservoir,
    T X_i,
    float m_i,
    float w_i,
    float c_i
)
{
    reservoir.Confidence += c_i;
    reservoir.WeightSum += w_i;

    float r = Rand01(rngInfo);
    float norm_w_i = w_i / reservoir.WeightSum;
    if (r < norm_w_i)
    {
        reservoir.Y = X_i;
        return true;
    }
    return false;
}

template<typename T>
void ReservoirMerge(
    inout RngInfo rngInfo,
    inout Reservoir<T> reservoir,
    Reservoir<T> other,
    float w_i
)
{
    reservoir.WeightSum += w_i;

    float r = Rand01(rngInfo);
    float norm_w_i = w_i / reservoir.WeightSum;
    if (r < norm_w_i)
    {
        reservoir.Y = other.Y;
    }

    reservoir.Confidence += other.Confidence;
}

template<typename T>
float RestirMis(T X_i, uint M)
{
    return 1 / M; // TODO
}

template<typename T, typename TInfo>
void WRS(
    inout RngInfo rngInfo,
    inout Reservoir<T> reservoir,
    uint M,
    uint ConfidenceCap,
    TInfo tInfo
)
{
    for (uint i = 0; i < M; i++)
    {
        T X_i = Generate(rngInfo, tInfo);

        float pHat = Target(X_i, tInfo);
        //float m_i = RestirMis(X_i, M);
        float W_X_i = 1.0f / PDF(X_i, tInfo);

        //float w_i = pHat * m_i * W_X_i;
        float w_i = pHat * W_X_i;
        float c_i = 1;

        ReservoirUpdate(rngInfo, reservoir, X_i, 1.0f, w_i, c_i);
    }

    reservoir.W_Y = reservoir.WeightSum / Target(reservoir.Y, tInfo);
    reservoir.Confidence = min(reservoir.Confidence, ConfidenceCap);
}

#endif