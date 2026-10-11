#ifndef H_GD_UTILS_H
#define H_GD_UTILS_H

float3 SampleSafeMirror(Texture2D tex, float3 primal, int2 pixelCoord, int2 min, int width, int height)
{
    if (pixelCoord.x < min.x || pixelCoord.y < min.y)
        return primal;

    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return primal;

    return tex[pixelCoord].rgb;
}

float3 SampleSafeZero(Texture2D tex, int2 pixelCoord, int2 min, int width, int height)
{
    if (pixelCoord.x < min.x || pixelCoord.y < min.y)
        return 0;

    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return 0;

    return tex[pixelCoord].rgb;
}

float3 SampleSafeZeroCoef(Texture2D tex, int2 pixelCoord, int width, int height, inout float a_coef)
{
    bool invalidPixel = false;
    if (pixelCoord.x < 0 || pixelCoord.x == width)
    {
        a_coef += 1.0f;
        invalidPixel = true;
    }

    if (pixelCoord.y < 0 || pixelCoord.y == height)
    {
        a_coef += 1.0f;
        invalidPixel = true;
    }

    if (invalidPixel)
        return 0;

    return tex[pixelCoord].rgb;
}

float3 GetTrueGradientX(Texture2D texGradientXF, Texture2D texGradientXB, int2 pixelCoord, int primalWidth, int primalHeight)
{
    int w1 = primalWidth - 1;
    int w = primalWidth;
    int h = primalHeight;

    float3 gradientXF = SampleSafeZero(texGradientXF, pixelCoord,             int2(0,0),  w1, h);
    float3 gradientXB = SampleSafeZero(texGradientXB, pixelCoord + int2(1,0), int2(1,0),  w,  h);

    return gradientXF + gradientXB;
}

float3 GetTrueGradientY(Texture2D texGradientYF, Texture2D texGradientYB, int2 pixelCoord, int primalWidth, int primalHeight)
{
    int h1 = primalHeight - 1;
    int w = primalWidth;
    int h = primalHeight;

    float3 gradientYF = SampleSafeZero(texGradientYF, pixelCoord,             int2(0,0),  w,  h1);
    float3 gradientYB = SampleSafeZero(texGradientYB, pixelCoord + int2(0,1), int2(0,1),  w,  h);

    return gradientYF + gradientYB;
}

#endif