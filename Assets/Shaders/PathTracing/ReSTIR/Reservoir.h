#ifndef H_RESERVOIR_H
#define H_RESERVOIR_H

template<typename TCandidate, typename TPreCandidate>
struct Reservoir
{
    TCandidate Y;
    TPreCandidate PreCandidate;

    float W_Y;
    float WeightSum;
    float Confidence;
    float PHat;
};

#endif