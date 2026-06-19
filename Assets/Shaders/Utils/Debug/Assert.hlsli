#ifndef H_ASSERT_H
#define H_ASSERT_H

#include "Utils/Debug/DebugID.h"
#include "Utils/Debug/NaNTests.hlsli"
#include "Utils/Debug/DebugStructs.h"

void dbgAssert(float4 v1, float4 v2, float4 v3, bool4 expr, uint dbgID)
{
    if (!expr.x || !expr.y || !expr.z || !expr.w)
    {
        InterlockedAdd(gDbgBufferErrorInfo[dbgID].ExprCounter, 1);

        float _;
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value1.x, v1.x, _);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value1.y, v1.y, _);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value1.z, v1.z, _);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value1.w, v1.w, _);

        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value2.x, v2.x, _);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value2.y, v2.y, _);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value2.z, v2.z, _);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value2.w, v2.w, _);

        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value3.x, v3.x, _);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value3.y, v3.y, _);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value3.z, v3.z, _);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value3.w, v3.w, _);
    }

    if (IsNaN4(v1) || IsNaN4(v2) || IsNaN4(v3))
        InterlockedAdd(gDbgBufferErrorInfo[dbgID].NaNCounter, 1);

    if (IsInf4(v1) || IsInf4(v2) || IsInf4(v3))
        InterlockedAdd(gDbgBufferErrorInfo[dbgID].InfCounter, 1);
}

void dbgAssert(float3 v1, float3 v2, float3 v3, bool3 expr, uint dbgID)          { dbgAssert(v1.xyzz, v2.xyzz, v3.xyzz, expr.xyzz, dbgID); }
void dbgAssert(float2 v1, float2 v2, float2 v3, bool2 expr, uint dbgID)          { dbgAssert(v1.xyyy, v2.xyyy, v3.xyyy, expr.xyyy, dbgID); }
void dbgAssert(float  v1, float  v2, float  v3, bool  expr, uint dbgID)          { dbgAssert(v1.xxxx, v2.xxxx, v3.xxxx, expr.xxxx, dbgID); }

#define DBG_ASSERT_EXPR(expr, dbgID)             if (DEBUG_ENABLED(Asserts))     dbgAssert(0, 0, 0, expr, dbgID);
#define DBG_ASSERT_VALUE(v,    dbgID)            if (DEBUG_ENABLED(Asserts))     dbgAssert(v, 0, 0, 1, dbgID);

#define DBG_ASSERT_EQ(v1, v2, dbgID)             if (DEBUG_ENABLED(Asserts))     dbgAssert(v1, v2, 0, v1 == v2, dbgID);
#define DBG_ASSERT_NEQ(v1, v2, dbgID)            if (DEBUG_ENABLED(Asserts))     dbgAssert(v1, v2, 0, v1 != v2, dbgID);
#define DBG_ASSERT_LT(v1, v2, dbgID)             if (DEBUG_ENABLED(Asserts))     dbgAssert(v1, v2, 0, v1 <  v2, dbgID);
#define DBG_ASSERT_LE(v1, v2, dbgID)             if (DEBUG_ENABLED(Asserts))     dbgAssert(v1, v2, 0, v1 <= v2, dbgID);
#define DBG_ASSERT_GT(v1, v2, dbgID)             if (DEBUG_ENABLED(Asserts))     dbgAssert(v1, v2, 0, v1 >  v2, dbgID);
#define DBG_ASSERT_GE(v1, v2, dbgID)             if (DEBUG_ENABLED(Asserts))     dbgAssert(v1, v2, 0, v1 >= v2, dbgID);

#define DBG_ASSERT_ZERO(v, dbgID)                if (DEBUG_ENABLED(Asserts))     dbgAssert(v, 0, 0, abs(v) < EPSILON, dbgID);

#define DBG_ASSERT_RANGE(min, v, max, dbgID)     if (DEBUG_ENABLED(Asserts))     dbgAssert(min, v, max, min <= v && v <= max, dbgID);

#endif
