Texture2D<float> gCDFConditional : register(t0);
RWTexture1D<float> gCDFMarginal  : register(u0);

[numthreads(1,1,1)]
void CSMain()
{
    uint2 dim;
    gCDFConditional.GetDimensions(dim.x, dim.y);

    uint x = dim.x - 1;

    float rollingSum = 0.0f;

    for (int y = 0; y < dim.y; y++)
    {
        rollingSum += gCDFConditional.Load(uint3(x, y, 0));
        gCDFMarginal[y] = rollingSum;
    }
}
