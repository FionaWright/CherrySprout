#ifndef H_RIS_H
#define H_RIS_H

#if FEAT_ADV_D(AreaRestir)
#    include "Compute/ReSTIR/PreCandidates/Area.hlsli"
#else
#    include "Compute/ReSTIR/PreCandidates/Point.hlsli"
#endif

float GetWeight(float mis, float pHat, float W)
{
    return mis * pHat * W;
}

template<typename TCandidate, typename TPreCandidate>
bool ReservoirUpdate(
    inout RngInfo rngInfo,
    inout Reservoir<TCandidate, TPreCandidate> reservoir,
    TCandidate X_i,
	TPreCandidate preX_i,
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
		reservoir.PreCandidate = preX_i;
        return true;
    }
    return false;
}

template<typename TReservoir, typename TContext>
bool ReservoirMerge(
    inout RngInfo rngInfo,
    float ConfidenceCap,
    inout TReservoir reservoir,
    TReservoir other,
    TContext context)
{
    float c1 = reservoir.Confidence;
    float c2 = min(other.Confidence, ConfidenceCap);
    float M = c1 + c2;

    reservoir.Confidence = M;

    if (M <= 0.0f)
        return false;

    float pHat1 = reservoir.PHat;
    float pHat2 = Target(other.Y, context);

    // Naive:
    float mis1 = c1 / M;
    float mis2 = c2 / M;

    // Balance:
    //float mis1 = c1 * pHat1 / (c1 * pHat1 + c2 * pHat2);
    //float mis2 = c2 * pHat2 / (c1 * pHat1 + c2 * pHat2);

    //float mis1 = pHat1 / (pHat1 + pHat2);
    //float mis2 = pHat2 / (pHat1 + pHat2);

    // TODO: Pairwise

    float w2 = GetWeight(mis2, pHat2, other.W_Y);

    if (w2 <= 0.0f || pHat2 <= 0.0f)
        return false;

    float w1 = GetWeight(mis1, 1.0f, reservoir.WeightSum);

    reservoir.WeightSum = w1 + w2;

    float r = Rand01(rngInfo);
    float norm_w = w2 / reservoir.WeightSum;
    if (r <= norm_w)
    {
        reservoir.Y = other.Y;
        reservoir.PHat = pHat2;
		reservoir.PreCandidate = other.PreCandidate;
        return true;
    }
    return false;
}

template<typename TReservoir>
void SetUCW(inout TReservoir reservoir)
{
    if (reservoir.Confidence <= 0.0f || reservoir.PHat <= 0.0f)
    {
        reservoir.Confidence = 0.0f;
        reservoir.W_Y = 0.0f;
        return;
    }

    reservoir.W_Y = reservoir.WeightSum / (reservoir.PHat);
}

template<typename TCandidate, typename TContext, typename TPreCandidate>
void WRS(
    inout RngInfo rngInfo,
    inout Reservoir<TCandidate, TPreCandidate> reservoir,
    uint2 pixelCoord,
    uint M,
    TContext context)
{
    bool candidateSelected = false;

    reservoir.WeightSum = 0.0f;
    reservoir.Confidence = 0.0f;

    for (uint i = 0; i < M; i++)
    {
		TPreCandidate preX_i = GeneratePreCandidate(rngInfo, pixelCoord, context);

		TCandidate X_i;
		float pHat;

#if FEAT_ADV_D(AreaRestir)
		{
			Generate(rngInfo, preX_i, X_i);
			pHat = Target(X_i, preX_i);
		}
#else
		{
			Generate(rngInfo, context, X_i);
			pHat = Target(X_i, context);
		}
#endif

        float pdf = PDF(X_i);

        DBG_ASSERT_GE(pHat, 0.0f, RESTIR_PHAT);
        DBG_ASSERT_GE(pdf, 0.0f, RESTIR_PDF);

        float W_X_i = pdf <= 0.0f ? 0.0f : 1.0f / pdf;

        float c = 1;
        float mis = c / (float)M;
        float w = GetWeight(mis, pHat, W_X_i);

        candidateSelected |= ReservoirUpdate(rngInfo, reservoir, X_i, preX_i, w, c, pHat);
    }

    if (!candidateSelected)
    {
        reservoir.W_Y = 0.0f;
        return;
    }

    SetUCW(reservoir);
}

#endif