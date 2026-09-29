#ifndef H_BINARY_SEARCH_H
#define H_BINARY_SEARCH_H

#include "PathTracing/Debug/Assert.hlsli"

uint BinarySearch(Texture1D<float> cdf, float u)
{
    uint size;
    cdf.GetDimensions(size);

    uint left = 0;
    uint right = size - 1;

    while (left < right)
    {
        uint mid = left + (right - left) / 2;

        if (u <= cdf[mid])
        {
            right = mid;
        }
        else
        {
            left = mid + 1;
        }
    }

    return left;
}

uint BinarySearch(StructuredBuffer<ProbabilityDistributionSample> cdf, float u)
{
    uint size, _;
    cdf.GetDimensions(size, _);

    uint left = 0;
    uint right = size - 1;

    while (left < right)
    {
        uint mid = left + (right - left) / 2;

        if (u < cdf[mid].CDF)
        {
            right = mid;
        }
        else
        {
            left = mid + 1;
        }
    }

    return left;
}

uint BinarySearchX(Texture2D<float> cdf, uint y, float u)
{
    uint size, _;
    cdf.GetDimensions(size, _);
    DBG_ASSERT_EQ(size, _,      CDF_SQUARE_ENV_MAP);

    uint left = 0;
    uint right = size - 1;

    while (left < right)
    {
        uint mid = left + (right - left) / 2;

        uint2 idx = uint2(mid, y);

        if (u <= cdf[idx])
        {
            right = mid;
        }
        else
        {
            left = mid + 1;
        }
    }

    return left;
}

uint BinarySearchY(Texture2D<float> cdf, uint x, float u)
{
    uint size, _;
    cdf.GetDimensions(size, _);
    DBG_ASSERT_EQ(size, _,      CDF_SQUARE_ENV_MAP);

    uint left = 0;
    uint right = size - 1;

    while (left < right)
    {
        uint mid = left + (right - left) / 2;

        uint2 idx = uint2(x, mid);

        if (u <= cdf[idx])
        {
            right = mid;
        }
        else
        {
            left = mid + 1;
        }
    }

    return left;
}

#endif