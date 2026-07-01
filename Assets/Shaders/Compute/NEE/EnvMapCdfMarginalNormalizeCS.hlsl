RWTexture1D<float> gCDFMarginal  : register(u0);

[numthreads(64,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint dim;
    gCDFMarginal.GetDimensions(dim);

    uint y = DTid.x;

    if (y >= dim)
        return;

    float val = gCDFMarginal[y];
    float max = gCDFMarginal[dim - 1];

    gCDFMarginal[y] = val / max;
}
