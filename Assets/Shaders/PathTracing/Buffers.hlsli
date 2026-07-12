#ifndef H_BUFFERS_H
#define H_BUFFERS_H

#include "Utils/CBVs.h"
#include "Scene/Vertex.h"
#include "Scene/Material.h"
#include "Scene/InstanceData.h"
#include "Utils/Debug/DebugStructs.h"

#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "PathTracing/Flags/MethodsHlsl.hlsli"

#define REGISTER_SPACE_DEFAULT            space0
#define REGISTER_SPACE_SCENE_TEXTURES     space1
#define REGISTER_SPACE_DEBUG              space2

// =================== B Registers ===============================================================================

ConstantBuffer<CbvPathTracingSettings>      gSettings               : register(b0, REGISTER_SPACE_DEFAULT);

// =================== T Registers (Default) =====================================================================

RaytracingAccelerationStructure             gTLAS                   : register(t0, REGISTER_SPACE_DEFAULT);

StructuredBuffer<Vertex>                    gMegaBufferVertex       : register(t1, REGISTER_SPACE_DEFAULT);
StructuredBuffer<uint3>                     gMegaBufferIndex        : register(t2, REGISTER_SPACE_DEFAULT);
StructuredBuffer<InstanceData>              gMegaBufferInstanceData : register(t3, REGISTER_SPACE_DEFAULT);
StructuredBuffer<Material>                  gMegaBufferMaterials    : register(t4, REGISTER_SPACE_DEFAULT);

Texture2D<float4>                           gTexEnvMap              : register(t5, REGISTER_SPACE_DEFAULT);

Texture2D<float>                            gEnvMapPmfConditional   : register(t6, REGISTER_SPACE_DEFAULT);
Texture2D<float>                            gEnvMapCdfConditional   : register(t7, REGISTER_SPACE_DEFAULT);
Texture1D<float>                            gEnvMapCdfMarginal      : register(t8, REGISTER_SPACE_DEFAULT);

Texture2D<uint>                             gGBufferMaterialIdx     : register(t9, REGISTER_SPACE_DEFAULT);
Texture2D<float4>                           gGBufferNormals         : register(t10, REGISTER_SPACE_DEFAULT);
Texture2D<float>                            gGBufferDepth           : register(t11, REGISTER_SPACE_DEFAULT);
Texture2D<float4>                           gGBufferUvMv            : register(t12, REGISTER_SPACE_DEFAULT);

// =================== T Registers (Scene Textures) ==============================================================

Texture2D<float4>                           gSceneTextures[]        : register(t0, REGISTER_SPACE_SCENE_TEXTURES);

// =================== T Registers (Debug) =======================================================================

// =================== U Registers ===============================================================================

RWTexture2D<float4>                         gTexAccumulation        : register(u0, REGISTER_SPACE_DEFAULT);
RWTexture2D<float4>                         gTexOutput              : register(u1, REGISTER_SPACE_DEFAULT);
//RWStructuredBuffer<ReservoirDI>             gReservoirBuffer        : register(u2, REGISTER_SPACE_DEFAULT);

// =================== U Registers (Debug) =======================================================================

// TODO: Handle register spaces properly in root sig. Then have all debug buffers dependent on _DEBUG
RWStructuredBuffer<DebugErrorInfo>          gDbgBufferErrorInfo     : register(u2, REGISTER_SPACE_DEFAULT);
RWStructuredBuffer<RayDump>                 gPathDump               : register(u3, REGISTER_SPACE_DEFAULT);

// =================== S Registers ===============================================================================

SamplerState                                gSampler                : register(s0);

#endif