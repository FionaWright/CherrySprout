#ifndef H_ASSERT_H
#define H_ASSERT_H

#include "PathTracing/Flags/Internal/MethodsHlsl.hlsli"

#if DEBUG_ENABLED_PP(Asserts)

#include "PathTracing/Debug/Globals.hlsli"
#include "PathTracing/Debug/Mutex.hlsli"

#include "Utils/Debug/DebugID.h"
#include "Utils/Debug/NaNTests.hlsli"
#include "Utils/Debug/DebugStructs.h"
#include "Utils/Constants.h"

#define MAX_ASSERTS_PER_PATH 3

// Note: Using scalar inputs to avoid DXC bug with [noinline]
[noinline]
void dbgAssert(
    float v1_x, float v1_y, float v1_z, float v1_w,
    float v2_x, float v2_y, float v2_z, float v2_w,
    float v3_x, float v3_y, float v3_z, float v3_w,
    bool expr_x, float expr_y, float expr_z, float expr_w,
    uint dbgID)
{
    if (!DEBUG_ENABLED(Asserts))
        return;

    float4 v1 = float4(v1_x, v1_y, v1_z, v1_w);
    float4 v2 = float4(v2_x, v2_y, v2_z, v2_w);
    float4 v3 = float4(v3_x, v3_y, v3_z, v3_w);
    bool4 expr = float4(expr_x, expr_y, expr_z, expr_w);

    bool updateExpr = !expr.x || !expr.y || !expr.z || !expr.w;
    bool updateNaN = IsNaN4(v1) || IsNaN4(v2) || IsNaN4(v3);
    bool updateInf = IsInf4(v1) || IsInf4(v2) || IsInf4(v3);
    bool updatedAny = updateExpr || updateNaN || updateInf;

    if (!updatedAny)
        return;

    if (updateExpr)
        InterlockedAdd(gDbgBufferErrorInfo[dbgID].ExprCounter, 1);

    if (updateNaN)
        InterlockedAdd(gDbgBufferErrorInfo[dbgID].NaNCounter, 1);

    if (updateInf)
        InterlockedAdd(gDbgBufferErrorInfo[dbgID].InfCounter, 1);

    if (gDebugFrameIndex == UINT_MAX || gDebugPixelCoord.x == UINT_MAX || gDebugPixelCoord.y == UINT_MAX)
        return;

    if (gDebugNumAssertsTriggered >= MAX_ASSERTS_PER_PATH)
        return;

    gDebugNumAssertsTriggered++;

    if (TryAcquireMutex(dbgID))
    {
        uint _i;
        float _f;

        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value1.x, v1.x, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value1.y, v1.y, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value1.z, v1.z, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value1.w, v1.w, _f);

        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value2.x, v2.x, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value2.y, v2.y, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value2.z, v2.z, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value2.w, v2.w, _f);

        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value3.x, v3.x, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value3.y, v3.y, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value3.z, v3.z, _f);
        InterlockedExchange(gDbgBufferErrorInfo[dbgID].Value3.w, v3.w, _f);

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

        ReleaseMutex(dbgID);
    }
}

// TODO: Does this belong here? Maybe under a different name?
[noinline]
void dbgMarkForDump()
{
    if (!DEBUG_ENABLED(Asserts))
        return;

    InterlockedAdd(gDbgBufferErrorInfo[_PATH_DUMP].ExprCounter, 1);

    if (gDebugFrameIndex == UINT_MAX || gDebugPixelCoord.x == UINT_MAX || gDebugPixelCoord.y == UINT_MAX)
        return;

    if (TryAcquireMutex(_PATH_DUMP))
    {
        uint _i;
        float _f;

        InterlockedExchange(gDbgBufferErrorInfo[_PATH_DUMP].PixelCoord.x, gDebugPixelCoord.x, _i);
        InterlockedExchange(gDbgBufferErrorInfo[_PATH_DUMP].PixelCoord.y, gDebugPixelCoord.y, _i);
        InterlockedExchange(gDbgBufferErrorInfo[_PATH_DUMP].FrameIndex, gDebugFrameIndex, _i);

        InterlockedExchange(gDbgBufferErrorInfo[_PATH_DUMP].CameraPositionWorld.x, gSettings.CameraPositionWorld.x, _f);
        InterlockedExchange(gDbgBufferErrorInfo[_PATH_DUMP].CameraPositionWorld.y, gSettings.CameraPositionWorld.y, _f);
        InterlockedExchange(gDbgBufferErrorInfo[_PATH_DUMP].CameraPositionWorld.z, gSettings.CameraPositionWorld.z, _f);

        [unroll]
        for (int r = 0; r < 4; r++)
            [unroll]
            for (int c = 0; c < 4; c++)
                InterlockedExchange(gDbgBufferErrorInfo[_PATH_DUMP].InvV[r][c], gSettings.InvV[r][c], _f);

        ReleaseMutex(_PATH_DUMP);
    }
}

void dbgAssert(float4 v1, float4 v2, float4 v3, bool4 expr, uint dbgID)          { dbgAssert(v1.x, v1.y, v1.z, v1.w, v2.x, v2.y, v2.z, v2.w, v3.x, v3.y, v3.z, v3.w, expr.x, expr.y, expr.z, expr.w, dbgID); }
void dbgAssert(float3 v1, float3 v2, float3 v3, bool3 expr, uint dbgID)          { dbgAssert(v1.x, v1.y, v1.z, 0, v2.x, v2.y, v2.z, 0, v3.x, v3.y, v3.z, 0, expr.x, expr.y, expr.z, 1, dbgID); }
void dbgAssert(float2 v1, float2 v2, float2 v3, bool2 expr, uint dbgID)          { dbgAssert(v1.x, v1.y, 0, 0, v2.x, v2.y, 0, 0, v3.x, v3.y, 0, 0, expr.x, expr.y, 1, 1, dbgID); }
void dbgAssert(float  v1, float  v2, float  v3, bool  expr, uint dbgID)          { dbgAssert(v1.x, 0, 0, 0, v2.x, 0, 0, 0, v3.x, 0, 0, 0, expr.x, 1, 1, 1, dbgID); }

#define DBG_ASSERT_EXPR(expr, dbgID)                  dbgAssert(0, 0, 0, expr, dbgID);
#define DBG_ASSERT_VALUE(v,    dbgID)                 dbgAssert(v, 0, 0, 1, dbgID);
#define DBG_ASSERT_FAIL(dbgID)                        dbgAssert(0, 0, 0, 0, dbgID);

#define DBG_ASSERT_EQ(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 == v2, dbgID);
#define DBG_ASSERT_NEQ(v1, v2, dbgID)                 dbgAssert(v1, v2, 0, v1 != v2, dbgID);
#define DBG_ASSERT_LT(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 <  v2, dbgID);
#define DBG_ASSERT_LE(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 <= v2, dbgID);
#define DBG_ASSERT_GT(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 >  v2, dbgID);
#define DBG_ASSERT_GE(v1, v2, dbgID)                  dbgAssert(v1, v2, 0, v1 >= v2, dbgID);

#define DBG_ASSERT_ZERO(v, dbgID)                     dbgAssert(v, 0, 0, abs(v) < EPSILON, dbgID);

#define DBG_ASSERT_RANGE(min, v, max, dbgID)          dbgAssert(min, v, max, min <= v && v <= max, dbgID);

#define DBG_ASSERT_APPROX(v1, v2, threshold, dbgID)       dbgAssert(v1, v2, 0, all(abs(v1 - v2) <= threshold), dbgID);

#define DBG_DUMP_PATH() dbgMarkForDump();

#else

#define DBG_ASSERT_EXPR(expr, dbgID)
#define DBG_ASSERT_VALUE(v,    dbgID)
#define DBG_ASSERT_FAIL(dbgID)
#define DBG_ASSERT_EQ(v1, v2, dbgID)
#define DBG_ASSERT_NEQ(v1, v2, dbgID)
#define DBG_ASSERT_LT(v1, v2, dbgID)
#define DBG_ASSERT_LE(v1, v2, dbgID)
#define DBG_ASSERT_GT(v1, v2, dbgID)
#define DBG_ASSERT_GE(v1, v2, dbgID)
#define DBG_ASSERT_ZERO(v, dbgID)
#define DBG_ASSERT_RANGE(min, v, max, dbgID)
#define DBG_ASSERT_APPROX(v1, v2, threshold, dbgID)
#define DBG_DUMP_PATH()

#endif

#endif
