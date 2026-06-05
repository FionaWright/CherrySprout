#ifndef H_MATH_UTILS_H
#define H_MATH_UTILS_H

#include "System/pch.h"

inline float Clamp(float val, float min, float max)
{
    return std::min(std::max(val, min), max);
}

inline XMFLOAT3 Add(XMFLOAT3 a, XMFLOAT3 b)
{
    return XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline XMFLOAT4 Add(XMFLOAT4 a, XMFLOAT4 b)
{
    return XMFLOAT4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline XMFLOAT3 Subtract(XMFLOAT3 a, XMFLOAT3 b)
{
	return XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline XMFLOAT2 Subtract(XMFLOAT2 a, XMFLOAT2 b)
{
	return XMFLOAT2(a.x - b.x, a.y - b.y);
}

inline XMFLOAT2 Frac(const XMFLOAT2& a)
{
    float fracX = a.x - std::floor(a.x);
    float fracY = a.y - std::floor(a.y);
    return XMFLOAT2(fracX, fracY);
}

inline XMFLOAT2 NaiveFrac(XMFLOAT2 a)
{
    a.x -= a.x < 0 ? std::ceil(a.x) : std::floor(a.x);
    a.y -= a.y < 0 ? std::ceil(a.y) : std::floor(a.y);
    return a;
}

inline XMFLOAT2 Abs(const XMFLOAT2& a)
{
    return XMFLOAT2(std::abs(a.x), std::abs(a.y));
}

inline XMFLOAT3 Abs(const XMFLOAT3& a)
{
    return XMFLOAT3(std::abs(a.x), std::abs(a.y), std::abs(a.z));
}

inline XMFLOAT3 Normalize(const XMFLOAT3& v)
{
	float length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	return XMFLOAT3(v.x / length, v.y / length, v.z / length);
}

inline XMFLOAT3 Divide(const XMFLOAT3& a, float d)
{
    return XMFLOAT3(a.x / d, a.y / d, a.z / d);
}

inline XMFLOAT3 Mult(const XMFLOAT3& a, float d)
{
    return XMFLOAT3(a.x * d, a.y * d, a.z * d);
}

inline XMFLOAT4 Mult(const XMFLOAT4& a, float d)
{
    return XMFLOAT4(a.x * d, a.y * d, a.z * d, a.w * d);
}

inline XMFLOAT3 Mult(const XMFLOAT3& a, const XMFLOAT3& b)
{
    return XMFLOAT3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline XMFLOAT3 Normalize(const XMFLOAT3& v, float& length)
{
    length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return XMFLOAT3(v.x / length, v.y / length, v.z / length);
}

inline XMFLOAT3 Negate(const XMFLOAT3& v)
{
    return XMFLOAT3(-v.x, -v.y, -v.z);
}

inline XMFLOAT3 Saturate(const XMFLOAT3& v)
{
    return XMFLOAT3(std::clamp(v.x, 0.0f, 1.0f), std::clamp(v.y, 0.0f, 1.0f), std::clamp(v.z, 0.0f, 1.0f));
}

inline float Magnitude(const XMFLOAT3& v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float Dot(const XMFLOAT3& a, const XMFLOAT3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float SqDist(const XMFLOAT3& a, const XMFLOAT3& b)
{
    XMFLOAT3 a_b = Subtract(a, b);
    return Dot(a_b, a_b);
}

inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b)
{
    XMFLOAT3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

inline bool Equals(XMFLOAT3 a, XMFLOAT3 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline bool Equals(XMFLOAT2 a, XMFLOAT2 b)
{
    return a.x == b.x && a.y == b.y;
}

inline std::string ToString(const XMFLOAT3& v)
{
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
}

inline bool NextCharactersMatch(std::ifstream& file, const std::string& expected, bool resetPos)
{
    std::streampos startPosition = file.tellg();

    for (char c : expected)
    {
        if (file.peek() != c)
        {
            if (resetPos)
                file.seekg(startPosition);
            return false;
        }

        file.get();
    }

    if (resetPos)
        file.seekg(startPosition);
    return true;
}

inline bool XOR(bool a, bool b)
{
    return !a != !b;
}

inline bool Approx(float a, float b)
{
    return abs(a - b) < 0.001f;
}

inline float Rand01()
{
    return std::rand() / static_cast<float>(RAND_MAX);
}

inline float Rand(float min, float max)
{
    return min + (max - min) * Rand01();
}

inline uint32_t PackColor(XMFLOAT4 color)
{
    const uint32_t r = (uint32_t)(color.x * 255.0f) & 0xFF;
    const uint32_t g = (uint32_t)(color.y * 255.0f) & 0xFF;
    const uint32_t b = (uint32_t)(color.z * 255.0f) & 0xFF;
    const uint32_t a = (uint32_t)(color.w * 255.0f) & 0xFF;

    return (a << 24) | (b << 16) | (g << 8) | r;
}

inline uint32_t PackColor(XMFLOAT3 color)
{
    const uint32_t r = (uint32_t)(color.x * 255.0f) & 0xFF;
    const uint32_t g = (uint32_t)(color.y * 255.0f) & 0xFF;
    const uint32_t b = (uint32_t)(color.z * 255.0f) & 0xFF;

    return (b << 16) | (g << 8) | r;
}

inline XMFLOAT4 UnpackColor4(uint32_t color)
{
    return XMFLOAT4((color & 0xFF) / 255.0f, ((color >> 8) & 0xFF) / 255.0f, ((color >> 16) & 0xFF) / 255.0f, ((color >> 24) & 0xFF) / 255.0f);
}

inline XMFLOAT3 UnpackColor3(uint32_t color)
{
    return XMFLOAT3((color & 0xFF) / 255.0f, ((color >> 8) & 0xFF) / 255.0f, ((color >> 16) & 0xFF) / 255.0f);
}

inline void Fill1000Primes(uint32_t* primesArray)
{
    constexpr uint32_t limit = 8000; // enough for first 1000 primes
    bool isPrime[limit + 1] = {};
    for (uint32_t i = 2; i <= limit; ++i)
        isPrime[i] = true;

    for (uint32_t i = 2; i * i <= limit; ++i)
        if (isPrime[i])
            for (uint32_t j = i * i; j <= limit; j += i)
                isPrime[j] = false;

    uint32_t idx = 0;
    for (uint32_t i = 2; idx < 1000; ++i)
        if (isPrime[i])
            primesArray[idx++] = i;
}

#endif