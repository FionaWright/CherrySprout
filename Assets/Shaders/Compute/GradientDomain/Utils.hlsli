#ifndef H_GD_UTILS_H
#define H_GD_UTILS_H

float3 SampleSafeZero(Texture2D tex, int2 pixelCoord, uint min, uint width, uint height)
{
    if (pixelCoord.x < min || pixelCoord.y < min)
        return 0;

    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return 0;

    return tex[pixelCoord].rgb;
}

float3 SampleSafeZeroCoef(Texture2D tex, int2 pixelCoord, uint width, uint height, inout float a_coef)
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

float3 GetTrueGradientX(Texture2D texGradientXF, Texture2D texGradientXB, int2 pixelCoord, uint primalWidth, uint primalHeight)
{
    uint w1 = primalWidth - 1;
    uint w = primalWidth;
    uint h = primalHeight;

    float3 gradientXF = SampleSafeZero(texGradientXF, pixelCoord,              0,  w1, h);
    float3 gradientXB = SampleSafeZero(texGradientXB, pixelCoord + uint2(1,0), 1,  w,  h);

    return gradientXF + gradientXB;
}

float3 GetTrueGradientY(Texture2D texGradientYF, Texture2D texGradientYB, int2 pixelCoord, uint primalWidth, uint primalHeight)
{
    uint h1 = primalHeight - 1;
    uint w = primalWidth;
    uint h = primalHeight;

    float3 gradientYF = SampleSafeZero(texGradientYF, pixelCoord,              0,  w,  h1);
    float3 gradientYB = SampleSafeZero(texGradientYB, pixelCoord + uint2(0,1), 1,  w,  h);

    return gradientYF + gradientYB;
}

#endif