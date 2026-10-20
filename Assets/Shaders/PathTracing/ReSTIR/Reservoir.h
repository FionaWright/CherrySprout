#ifndef H_RESERVOIR_H
#define H_RESERVOIR_H

template<typename TCandidate>
struct Reservoir
{
    TCandidate Y;

    float W_Y;
    float WeightSum;
    float Confidence;
    float PHat;
};

template<typename TCandidate>
Reservoir<TCandidate> CreateReservoir()
{
    return (Reservoir<TCandidate>)0;
}

#endif