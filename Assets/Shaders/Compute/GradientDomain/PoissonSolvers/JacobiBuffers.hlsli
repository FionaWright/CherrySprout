#ifndef H_JACOBI_BUFFERS_H
#define H_JACOBI_BUFFERS_H

Texture2D<float4> gTexPrimal : register(t0);
Texture2D<float4> gTexGradientXF : register(t1);
Texture2D<float4> gTexGradientXB : register(t2);
Texture2D<float4> gTexGradientYF : register(t3);
Texture2D<float4> gTexGradientYB : register(t4);
Texture2D<float4> gPreviousIteration : register(t5);

RWTexture2D<float4> gGradientTerms : register(u0); // b_{i,j}
RWTexture2D<float4> gNextIteration : register(u1);

ConstantBuffer<CbvSprJacobi> gCBV : register(b0);

#endif