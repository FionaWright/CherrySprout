
void Sample(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float3 f,
    out float pdf
);

void Evaluate(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    float3 wi,

    out float3 f,
    out float pdf
);