#ifndef H_RIS_H
#define H_RIS_H

float GetWeight(float pHat, float c, float W)
{
    return pHat * c * W;
}

template<typename TCandidate, typename TContext>
bool ReservoirUpdate(
    inout RngInfo rngInfo,
    inout Reservoir<TCandidate,TContext> reservoir,
    TCandidate X_i,
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
    if (r <= norm_w)
    {
        reservoir.Y = X_i;
        reservoir.PHat = pHat;
        return true;
    }
    return false;
}

template<typename TCandidate, typename TContext>
bool ReservoirMerge(
    inout RngInfo rngInfo,
    float ConfidenceCap,
    inout Reservoir<TCandidate,TContext> reservoir,
    Reservoir<TCandidate,TContext> other)
{
    float c = min(other.Confidence, ConfidenceCap);
    float pHat = Target(other.Y, reservoir.Context);
    float w = GetWeight(pHat, c, other.W_Y);

    reservoir.Confidence += c;

    if (w <= 0.0f || pHat <= 0.0f)
        return false;

    reservoir.WeightSum += w;

    float r = Rand01(rngInfo);
    float norm_w = w / reservoir.WeightSum;
    if (r <= norm_w)
    {
        reservoir.Y = other.Y;
        reservoir.PHat = pHat;
        return true;
    }
    return false;
}

template<typename TCandidate>
float RestirMis(TCandidate X_i, uint M)
{
    return 1 / (float)M;
}

template<typename TCandidate, typename TContext>
void WRS(
    inout RngInfo rngInfo,
    inout Reservoir<TCandidate,TContext> reservoir,
    uint M,
    TContext context)
{
    bool candidateSelected = false;

    reservoir.WeightSum = 0.0f;
    reservoir.Confidence = 0.0f;

    reservoir.Context = context;

    for (uint i = 0; i < M; i++)
    {
        TCandidate X_i = Generate(rngInfo, context);

        float pHat = Target(X_i, context);
        float pdf = PDF(X_i, context);

        float W_X_i = pdf <= 0.0f ? 0.0f : 1.0f / pdf;

        //float m = RestirMis(X_i, M);
        float c = 1;
        float w = GetWeight(pHat, c, W_X_i);

        candidateSelected |= ReservoirUpdate<TCandidate,TContext>(rngInfo, reservoir, X_i, w, c, pHat);
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