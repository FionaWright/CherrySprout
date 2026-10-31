#ifndef H_GD_MISS_SHIFTED_H
#define H_GD_MISS_SHIFTED_H

void MissShifted(inout PathState pathState, PathVertexInfo v1, ReconnectionState reconnectionState)
{
    float3 Li;

    if (reconnectionState == ReconnectionState::eConnected)
    {
        Li = pathState.Beta * v1.DirectContribution;
    }
    else
    {
        Li = pathState.Beta * Miss(pathState);
    }

    pathState.Lo += ApplyFireflyThreshold(Li, gSettings.FireflyThreshold);
}

#endif