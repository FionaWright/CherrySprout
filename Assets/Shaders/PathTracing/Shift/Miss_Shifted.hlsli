#ifndef H_GD_MISS_SHIFTED_H
#define H_GD_MISS_SHIFTED_H

void Miss_S(inout PathState pathState, PathVertexInfo v1, ReconnectionState reconnectionState)
{
    float3 Li;

    if (FEAT_DBG(FurnaceTestHHE))
        Li = 0.0f;
    else if (FEAT_DBG(FurnaceTestHDR))
        Li = 1.0f;
    else if (reconnectionState == ReconnectionState::eConnected)
    {
        Li = pathState.Beta * v1.DirectContribution;

        if (v1.Lobe != LobeTag::eNEE)
            Li *= v1.MIS;
    }
    else
    {
        float mis;
        Li = pathState.Beta * Miss(pathState, mis);

        if (v1.Lobe != LobeTag::eNEE)
            Li *= mis;
    }

    pathState.Lo += ApplyFireflyThreshold(Li, gSettings.FireflyThreshold);
}

#endif