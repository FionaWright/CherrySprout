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

ConstantBuffer<CbvMatrices_M> gMatricesM : register(b0); // Push Constants
ConstantBuffer<CbvMatrices_VP> gMatricesVP : register(b1);

VsOut VSMain(VsIn input)
{
    VsOut output;

    output.normal = normalize(mul((float3x3)gMatricesM.MTI, (float3)input.normal));
    output.uv = input.uv;

    float4 pos = float4(input.position, 1.0f);
    float4 worldPos = mul(gMatricesM.M, pos);
    pos = mul(gMatricesVP.V, worldPos);
    output.position = mul(gMatricesVP.P, pos);

    //float4 pos = float4(input.position, 1.0f);
    //float4 worldPos = mul(pos, gMatrices.M);
    //pos = mul(worldPos, gMatrices.V);
    //output.position = mul(pos, gMatrices.P);

    //output.position = float4(input.position * 0.5f, 1);
    //output.uv = float2(input.uv);
    //output.normal = float3(input.normal);

    return output;
}