#ifndef H_RIS_H
#define H_RIS_H

float GetWeight(float pHat, float c, float W)
{
    return pHat * c * W;
}

template<typename TCandidate>
bool ReservoirUpdate(
    inout RngInfo rngInfo,
    inout Reservoir<TCandidate> reservoir,
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
    inout Reservoir<TCandidate> reservoir,
    Reservoir<TCandidate> other,
    TContext context)
{
    float c = min(other.Confidence, ConfidenceCap);
    float pHat = Target(other.Y, context);
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

template<typename TCandidate>
void NormalizeUnbiasedContributionWeight(inout Reservoir<TCandidate> reservoir)
{
    if (reservoir.Confidence <= 0.0f || reservoir.PHat <= 0.0f)
    {
        reservoir.Confidence = 0.0f;
        reservoir.W_Y = 0.0f;
        return;
    }

    reservoir.W_Y = reservoir.WeightSum / (reservoir.PHat * reservoir.Confidence);
}

template<typename TCandidate, typename TContext>
void WRS(
    inout RngInfo rngInfo,
    inout Reservoir<TCandidate> reservoir,
    uint M,
    TContext context)
{
    bool candidateSelected = false;

    reservoir.WeightSum = 0.0f;
    reservoir.Confidence = 0.0f;

    for (uint i = 0; i < M; i++)
    {
        TCandidate X_i;
        Generate(rngInfo, context, X_i);

        float pHat = Target(X_i, context);
        float pdf = PDF(X_i, context);

        DBG_ASSERT_GE(pHat, 0.0f, RESTIR_PHAT);
        DBG_ASSERT_GE(pdf, 0.0f, RESTIR_PDF);

        float W_X_i = pdf <= 0.0f ? 0.0f : 1.0f / pdf;

        //float m = RestirMis(X_i, M);
        float c = 1;
        float w = GetWeight(pHat, c, W_X_i);

        candidateSelected |= ReservoirUpdate<TCandidate>(rngInfo, reservoir, X_i, w, c, pHat);
    }

    if (!candidateSelected)
    {
        reservoir.W_Y = 0.0f;
        return;
    }

    NormalizeUnbiasedContributionWeight(reservoir);
}

#endif