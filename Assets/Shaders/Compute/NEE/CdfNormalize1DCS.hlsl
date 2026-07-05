RWTexture1D<float> gCDF  : register(u0);

[numthreads(64,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint dim;
    gCDF.GetDimensions(dim);

    uint y = DTid.x;

    if (y >= dim)
        return;

    float val = gCDF[y];
    float max = gCDF[dim - 1];

    gCDF[y] = val / max;
}
