#ifndef H_JACOBI_BUFFERS_H
#define H_JACOBI_BUFFERS_H

Texture2D<float4> gTexPrimal : register(t0);
Texture2D<float4> gTexGradientX : register(t1);
Texture2D<float4> gTexGradientY : register(t2);
Texture2D<float4> gPreviousIteration : register(t3);

RWTexture2D<float4> gGradientTerms : register(u0); // b_{i,j}
RWTexture2D<float4> gNextIteration : register(u1);

ConstantBuffer<CbvSprJacobi> gCBV : register(b0);

#endif