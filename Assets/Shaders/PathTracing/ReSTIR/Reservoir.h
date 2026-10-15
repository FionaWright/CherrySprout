#ifndef H_RESERVOIR_H
#define H_RESERVOIR_H

template<typename T, typename TInfo>
struct Reservoir
{
    T Y;
    TInfo Info;

    float W_Y;
    float WeightSum;
    float Confidence;
    float PHat;
};

template<typename T, typename TInfo>
Reservoir<T, TInfo> CreateReservoir()
{
    Reservoir<T, TInfo> r = (Reservoir<T, TInfo>)0;
    r.W_Y = 0;
    r.WeightSum = 0;
    r.Confidence = 0;
    return r;
}

#endif