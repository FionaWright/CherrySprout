#include "Utils/CommonStructs.h"

struct VsIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VsOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
};

ConstantBuffer<CbvMatrices> gMatrices : register(b0);

VsOut VSMain(VsIn input)
{
    VsOut output;

    float4 pos = float4(input.position, 1.0f);
    float4 worldPos = mul(gMatrices.M, pos);

    output.normal = normalize(mul((float3x3)gMatrices.MTI, (float3)input.normal));

    pos = mul(gMatrices.V, worldPos);

    output.position = mul(gMatrices.P, pos);
    output.uv = input.uv;

    return output;
}