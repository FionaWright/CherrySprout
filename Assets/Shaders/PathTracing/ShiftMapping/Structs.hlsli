#ifndef H_SHIFT_MAP_STRUCTS_H
#define H_SHIFT_MAP_STRUCTS_H

struct ShiftResult
{
    bool IsSuccessful;
    float Jacobian;
    float3 Wi;
};

#endif