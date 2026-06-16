Texture2D<float> gCDFConditional : register(t0);
RWTexture1D<float> gCDFMarginal  : register(u0);

[numthreads(1,1,1)]
void CSMain(uint3 _ : SV_DispatchThreadID)
{
    uint2 dim;
    gCDFConditional.GetDimensions(dim.x, dim.y);

    uint y = dim.y - 1;

    float rollingSum = 0.0f;

    for (int x = 0; x < dim.x; x++)
    {
        rollingSum += gCDFConditional.Load(int3(x, y, 0));
        gCDFMarginal[x] = rollingSum;
    }
}
