// Interface for the MicrofacetModel
// All microfacet model "subclasses" must provide implementations for these functions

void Init(float roughness, RngInfo rngInfo, float3 V); // For Sample
void Init(float roughness, float3 V); // For Eval
void InitAniso(HitInfo hitInfo);

float RoughnessToAlpha(float roughness);

float3 Sample(float u1, float u2);

float D(float3 H);
float G1(float NdW);
float G2(float NdL, float NdV);

float PDF(float D, float3 H, float3 V);

