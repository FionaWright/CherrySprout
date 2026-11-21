#ifndef H_RESERVOIR_H
#define H_RESERVOIR_H

template<typename TCandidate, typename TPixelInfo>
struct Reservoir
{
    TCandidate Y;
    TPixelInfo PixelInfo;

    float W_Y;
    float WeightSum;
    float Confidence;
    float PHat;
};

template<typename TC, typename TP>
Reservoir<TC, TP> CreateReservoir()
{
    return (Reservoir<TC, TP>)0;
}

#endif