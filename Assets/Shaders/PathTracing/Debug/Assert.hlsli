#ifndef H_ASSERT_H
#define H_ASSERT_H

#include "PathTracing/Debug/Globals.hlsli"

#if DEBUG_ENABLED(Asserts)

#include "Utils/Debug/DebugID.h"
#include "Utils/Debug/NaNTests.hlsli"
#include "Utils/Debug/DebugStructs.h"
#include "Utils/Constants.h"

void dbgAssert(float4 v1, float4 v2, float4 v3, bool4 expr, uint dbgID)
{
    bool updateExpr = !expr.x || !expr.y || !expr.z || !expr.w;
    bool updateNaN = IsNaN4(v1) || IsNaN4(v2) || IsNaN4(v3);
    bool updateInf = IsInf4(v1) || IsInf4(v2) || IsInf4(v3);
    bool updatedAny = updateExpr || updateNaN || updateInf;

    if (updateExpr)
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

    if (updateNaN)
        InterlockedAdd(gDbgBufferErrorInfo[dbgID].NaNCounter, 1);

    if (updateInf)
        InterlockedAdd(gDbgBufferErrorInfo[dbgID].InfCounter, 1);

    if (updatedAny && gDebugFrameIndex != UINT_MAX && gDebugPixelCoord.x != UINT_MAX && gDebugPixelCoord.y != UINT_MAX)
    {
        uint _i;
        float _f;
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].PixelCoord.x, gDebugPixelCoord.x, _i);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].PixelCoord.y, gDebugPixelCoord.y, _i);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].FrameIndex, gDebugFrameIndex, _i);

        InterlockedExchange(gDbgBufferErrorInfo[dbgID].CameraPositionWorld.x, gSettings.CameraPositionWorld.x, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].CameraPositionWorld.y, gSettings.CameraPositionWorld.y, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].CameraPositionWorld.z, gSettings.CameraPositionWorld.z, _f);

        [unroll]
        for (int r = 0; r < 4; r++)
            [unroll]
            for (int c = 0; c < 4; c++)
                InterlockedExchange(gDbgBufferErrorInfo[dbgID].InvV[r][c], gSettings.InvV[r][c], _f);
    }
}

void dbgAssert(float3 v1, float3 v2, float3 v3, bool3 expr, uint dbgID)          { dbgAssert(v1.xyzz, v2.xyzz, v3.xyzz, expr.xyzz, dbgID); }
void dbgAssert(float2 v1, float2 v2, float2 v3, bool2 expr, uint dbgID)          { dbgAssert(v1.xyyy, v2.xyyy, v3.xyyy, expr.xyyy, dbgID); }
void dbgAssert(float  v1, float  v2, float  v3, bool  expr, uint dbgID)          { dbgAssert(v1.xxxx, v2.xxxx, v3.xxxx, expr.xxxx, dbgID); }

#define DBG_ASSERT_EXPR(expr, dbgID)                  dbgAssert(0, 0, 0, expr, dbgID);
#define DBG_ASSERT_VALUE(v,    dbgID)                 dbgAssert(v, 0, 0, 1, dbgID);

#define DBG_ASSERT_EQ(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 == v2, dbgID);
#define DBG_ASSERT_NEQ(v1, v2, dbgID)                 dbgAssert(v1, v2, 0, v1 != v2, dbgID);
#define DBG_ASSERT_LT(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 <  v2, dbgID);
#define DBG_ASSERT_LE(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 <= v2, dbgID);
#define DBG_ASSERT_GT(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 >  v2, dbgID);
#define DBG_ASSERT_GE(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 >= v2, dbgID);

#define DBG_ASSERT_ZERO(v, dbgID)                     dbgAssert(v, 0, 0, abs(v) < EPSILON, dbgID);

#define DBG_ASSERT_RANGE(min, v, max, dbgID)          dbgAssert(min, v, max, min <= v && v <= max, dbgID);

#define DBG_ASSERT_APPROX(v1, v2, threshold, dbgID)       dbgAssert(v1, v2, 0, all(abs(v1 - v2) <= threshold), dbgID);

#else

#define DBG_ASSERT_EXPR(expr, dbgID)
#define DBG_ASSERT_VALUE(v,    dbgID)
#define DBG_ASSERT_EQ(v1, v2, dbgID)
#define DBG_ASSERT_NEQ(v1, v2, dbgID)
#define DBG_ASSERT_LT(v1, v2, dbgID)
#define DBG_ASSERT_LE(v1, v2, dbgID)
#define DBG_ASSERT_GT(v1, v2, dbgID)
#define DBG_ASSERT_GE(v1, v2, dbgID)
#define DBG_ASSERT_ZERO(v, dbgID)
#define DBG_ASSERT_RANGE(min, v, max, dbgID)
#define DBG_ASSERT_APPROX(v1, v2, threshold, dbgID)

#endif

#endif
