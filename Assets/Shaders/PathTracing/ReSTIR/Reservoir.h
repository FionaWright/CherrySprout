#ifndef H_RESERVOIR_H
#define H_RESERVOIR_H

template<typename TCandidate, typename TContext>
struct Reservoir
{
    TCandidate Y;
    TContext Context;

    float W_Y;
    float WeightSum;
    float Confidence;
    float PHat;
};

template<typename TCandidate, typename TContext>
Reservoir<TCandidate, TContext> CreateReservoir()
{
    return (Reservoir<TCandidate, TContext>)0;
}

#endif