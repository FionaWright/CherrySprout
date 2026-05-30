#ifndef H_BUFFERS_H
#define H_BUFFERS_H

#define REGISTER_SPACE_DEFAULT 0
#define REGISTER_SPACE_SCENE_TEXTURES 1
#define REGISTER_SPACE_DEBUG 2

// =================== C Registers ===============================



// =================== U Registers ===============================

RWTexture2D<float4>                 gTexAccumulation        : register(u0, REGISTER_SPACE_DEFAULT);

// =================== T Registers (Default) =====================

RaytracingAccelerationStructure     gTLAS                   : register(t0, REGISTER_SPACE_DEFAULT);

StructuredBuffer<Vertex>            gMegaBufferVertex       : register(t1, REGISTER_SPACE_DEFAULT);
StructuredBuffer<uint>              gMegaBufferIndex        : register(t2, REGISTER_SPACE_DEFAULT);
StructuredBuffer<InstanceData>      gMegaBufferInstanceData : register(t3, REGISTER_SPACE_DEFAULT);
StructuredBuffer<Material>          gMegaBufferMaterials    : register(t4, REGISTER_SPACE_DEFAULT);

Texture2D<float4>                   gTexEnvMap              : register(t5, REGISTER_SPACE_DEFAULT);

// =================== T Registers (Scene Textures) ==============

Texture2D<float4>                   gSceneTextures[]        : register(t0, REGISTER_SPACE_SCENE_TEXTURES);

// =================== T Registers (Debug) =======================



// =================== S Registers ===============================

SamplerState                        gSampler                : register(s0);

#endif