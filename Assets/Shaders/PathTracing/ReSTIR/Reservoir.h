#ifndef H_RESERVOIR_H
#define H_RESERVOIR_H

template<typename T>
struct Reservoir
{
    T Y;

    float W_Y;
    float WeightSum;
    float Confidence;
    float padding;
};

template<typename T>
Reservoir<T> CreateReservoir()
{
    Reservoir<T> r;
    r.W_Y = 0;
    r.WeightSum = 0;
    r.Confidence = 0;
    return r;
}

#endif