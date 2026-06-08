#include "Utils/CBVs.h"

struct VsIn
{
    float3 Position : POSITION;
};

struct VsOut
{
    float4 Position : SV_Position;
    float3 ViewDirection : TEXCOORD0;
};

ConstantBuffer<CbvMatrices_MVP> gMatrices : register(b0);

VsOut VSMain(VsIn input)
{
    VsOut o;

    float4 pos = float4(input.Position, 1.0f);
    float4x4 V_rot = gMatrices.V;
    V_rot._14 = V_rot._24 = V_rot._34 = 0.0f;
    o.Position = mul(gMatrices.P, mul(V_rot, pos));

    o.Position.z = o.Position.w;

    o.ViewDirection = normalize(input.Position.xyz);

    return o;
}