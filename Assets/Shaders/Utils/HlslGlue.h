#ifndef H_HLSLGLUE_H
#define H_HLSLGLUE_H

#ifdef __cplusplus

#include <algorithm>
#include <cmath>
#include <DirectXMath.h>

namespace hlsl
{
    typedef DirectX::XMFLOAT4X4 float4x4;
    typedef DirectX::XMFLOAT2 float2;
    typedef DirectX::XMFLOAT3 float3;
    typedef DirectX::XMFLOAT4 float4;
    typedef uint32_t uint;

    typedef struct uint2
    {
        uint32_t x;
        uint32_t y;
    } uint2;

    typedef struct uint3
    {
        uint32_t x;
        uint32_t y;
        uint32_t z;
    } uint3;

    typedef struct uint4
    {
        uint32_t x;
        uint32_t y;
        uint32_t z;
        uint32_t w;
    } uint4;
}

#define GLUE_IN(T)
#define GLUE_OUT(T) T&
#define GLUE_INOUT(T) T&

template <typename T>
constexpr auto max(T&& a, T&& b)
{
    return std::max(std::forward<T>(a), std::forward<T>(b));
}

template <typename T>
constexpr auto min(T&& a, T&& b)
{
    return std::min(std::forward<T>(a), std::forward<T>(b));
}

template <typename T>
constexpr auto abs(T&& x)
{
    return std::abs(std::forward<T>(x));
}

template <typename T>
constexpr auto sqrt(T&& x)
{
    return std::sqrt(std::forward<T>(x));
}

template <typename T>
constexpr auto cos(T&& x)
{
    return std::cos(std::forward<T>(x));
}

template <typename T>
constexpr auto sin(T&& x)
{
    return std::sin(std::forward<T>(x));
}

template <typename T>
constexpr auto atan(T&& x)
{
    return std::atan(std::forward<T>(x));
}

template <typename T, typename U>
constexpr auto atan2(T&& y, U&& x)
{
    return std::atan2(
        std::forward<T>(y),
        std::forward<U>(x)
    );
}

template <typename T>
constexpr auto asin(T&& x)
{
    return std::asin(std::forward<T>(x));
}

template <typename T>
constexpr auto log(T&& x)
{
    return std::log(std::forward<T>(x));
}

template <typename T>
constexpr T clamp(const T& x, const T& xmin, const T& xmax)
{
    return max(xmin, min(xmax, x));
}

template <typename T>
constexpr T saturate(const T& x)
{
    return clamp(x, T(0), T(1));
}

template <typename T>
inline T frac(const T& x)
{
    return std::fmod(x, T(1));
}

template <typename T>
inline auto normalize(const T& v)
{
    const auto mag = sqrt(dot(v, v));
    return v / mag;
}

template <typename T>
constexpr auto dot(const T& a, const T& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename T, typename U>
constexpr auto lerp(const T& s0, const T& s1, const U& t)
{
    return (T(1) - t) * s0 + t * s1;
}

#else

#define GLUE_IN(T) in T
#define GLUE_OUT(T) out T
#define GLUE_INOUT(T) inout T

namespace hlsl
{
    typedef float4x4 float4x4;

    typedef float2 float2;
    typedef float3 float3;
    typedef float4 float4;
    typedef uint uint;
    typedef uint2 uint2;
    typedef uint3 uint3;
    typedef uint4 uint4;
    typedef int2 int2;
    typedef int3 int3;
    typedef int4 int4;
}

//#define const
#define inline

#endif

#endif
