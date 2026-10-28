// Interface for the MicrofacetModel
// All microfacet model "subclasses" must provide implementations for these functions

void Init(float roughness, float3 V);
void InitAniso(HitInfo hitInfo);

uint GetRngCount();

float RoughnessToAlpha(float roughness);

float3 Sample(float3 rng);

float D(float3 H);
float G1(float3 W);
float G2(float3 L, float3 V);

float PDF(float D, float3 H, float3 V);

