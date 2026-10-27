#ifndef H_SHIFT_MAPS_FAIL_H
#define H_SHIFT_MAPS_FAIL_H

#include "PathTracing/ShiftMapping/Structs.hlsli"

ShiftResult ShiftFail()
{
    DBG_ASSERT_FAIL(GD_INVALID_VERTEX_TYPE);

    ShiftResult shiftResult = (ShiftResult)0;
    shiftResult.IsSuccessful = false;
    return shiftResult;
}

#endif