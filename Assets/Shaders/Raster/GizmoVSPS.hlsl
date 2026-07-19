#include "Utils/CBVs.h"

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
};

ConstantBuffer<CbvMatrices_MVP_Lean> gMatrices : register(b0); // Push Constants
ConstantBuffer<CbvColor> gColor : register(b1);

Texture2D<float4> gTex : register(t0);

SamplerState gSampler : register(s0);

VsOut VSMain(VsIn input)
{
    VsOut output;

    output.uv = input.uv;

    // Billboard center (translation only)
    float3 center = mul(gMatrices.M, float4(0, 0, 0, 1)).xyz;

    float3 camRight = float3(
        gMatrices.V[0][0],
        gMatrices.V[0][1],
        gMatrices.V[0][2]);

    float3 camUp = float3(
        gMatrices.V[1][0],
        gMatrices.V[1][1],
        gMatrices.V[1][2]);

    // Scale from model matrix (optional)
    float scaleX = length(gMatrices.M[0].xyz);
    float scaleY = length(gMatrices.M[1].xyz);

    scaleX *= 0.15f;
    scaleY *= 0.15f;

    float3 worldPos =
        center +
        camRight * (input.position.x * scaleX) +
        camUp    * (input.position.y * scaleY);

    output.position = mul(
        gMatrices.P,
        mul(gMatrices.V, float4(worldPos, 1.0f)));

    return output;
}

float4 PSMain(VsOut input) : SV_TARGET
{
    float4 texSample = gTex.SampleLevel(gSampler, input.uv, 0);
    texSample *= gColor.Color;
    if (texSample.a < 0.5f)
        discard;
    return texSample;
}