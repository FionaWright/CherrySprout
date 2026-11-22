#ifndef H_BUFFERS_H
#define H_BUFFERS_H

#include "Utils/CBVs.h"
#include "Scene/Vertex.h"
#include "Scene/Material.h"
#include "Scene/InstanceData.h"
#include "Scene/PunctualLight.h"
#include "PathTracing/Spaces.h"
#include "PathTracing/ReSTIR/Structs.h"
#include "PathTracing/NEE/LSD/Alias.h"

#define PT_BUFFERS_DEFINED

// =================== B Registers ===============================================================================

ConstantBuffer<CbvPathTracingSettings>              gSettings               : register(b0, SPACE_B_DEFAULT);
ConstantBuffer<CbvRestirSettings>                   gRestirSettings         : register(b1, SPACE_B_DEFAULT);

// =================== B Registers (Debug) =======================================================================
#if (FEAT_FLAGS_COMP_DBG != 0)

ConstantBuffer<CbvPathTracingDebugSettings>         gDebugSettings          : register(b0, SPACE_B_DEBUG);

#endif
// ===============================================================================================================

#include "PathTracing/FeatFlags/FeatFlags.hlsli"
#include "Utils/Debug/DebugStructs.h"

// =================== T Registers (Default) =====================================================================

RaytracingAccelerationStructure                     gTLAS                   : register(t0, SPACE_T_DEFAULT);

StructuredBuffer<Vertex>                            gMegaBufferVertex       : register(t1, SPACE_T_DEFAULT);
StructuredBuffer<uint3>                             gMegaBufferIndex        : register(t2, SPACE_T_DEFAULT);
StructuredBuffer<InstanceData>                      gMegaBufferInstanceData : register(t3, SPACE_T_DEFAULT);
StructuredBuffer<Material>                          gMegaBufferMaterials    : register(t4, SPACE_T_DEFAULT);
StructuredBuffer<PunctualLight>                     gMegaBufferPunctuals    : register(t5, SPACE_T_DEFAULT);

Texture2D<float4>                                   gTexEnvMap              : register(t6, SPACE_T_DEFAULT);

Texture2D<float>                                    gEnvMapPmfConditional   : register(t7, SPACE_T_DEFAULT);
Texture2D<float>                                    gEnvMapCdfConditional   : register(t8, SPACE_T_DEFAULT);
Texture1D<float>                                    gEnvMapCdfMarginal      : register(t9, SPACE_T_DEFAULT);
StructuredBuffer<uint>                              gEmissiveInstanceToInstanceMap : register(t10, SPACE_T_DEFAULT);
StructuredBuffer<int>                               gInstanceToEmissiveInstanceMap : register(t11, SPACE_T_DEFAULT);

#if FEAT_CORE_D(AliasTables)
StructuredBuffer<AliasEntry>                        gLightAliasTable        : register(t12, SPACE_T_DEFAULT);
#else
StructuredBuffer<ProbabilityDistributionSample>     gLightCDF               : register(t12, SPACE_T_DEFAULT);
#endif

Texture2D<uint>                                     gGBufferMaterialIdx     : register(t13, SPACE_T_DEFAULT);
Texture2D<float4>                                   gGBufferNormalsUv       : register(t14, SPACE_T_DEFAULT);
Texture2D<float>                                    gGBufferDepth           : register(t15, SPACE_T_DEFAULT);
Texture2D<float4>                                   gGBufferMv              : register(t16, SPACE_T_DEFAULT);

// =================== T Registers (Scene Textures) ==============================================================

Texture2D<float4>                                   gSceneTextures[]        : register(t0, SPACE_T_SCENE_TEXTURES);

// =================== T Registers (Debug) =======================================================================

// =================== U Registers ===============================================================================

RWTexture2D<float4>                                 gTexPrimal              : register(u0, SPACE_U_DEFAULT);
RWTexture2D<float4>                                 gTexGradientXF          : register(u1, SPACE_U_DEFAULT);
RWTexture2D<float4>                                 gTexGradientXB          : register(u2, SPACE_U_DEFAULT);
RWTexture2D<float4>                                 gTexGradientYF          : register(u3, SPACE_U_DEFAULT);
RWTexture2D<float4>                                 gTexGradientYB          : register(u4, SPACE_U_DEFAULT);

#if FEAT_CORE_D(AreaRestir)
RWStructuredBuffer<ReservoirDI_Area>                gReservoirBufferDI      : register(u5, SPACE_U_DEFAULT);
RWStructuredBuffer<ReservoirGI_Area>                gReservoirBufferGI      : register(u6, SPACE_U_DEFAULT);
RWStructuredBuffer<ReservoirPT_Area>                gReservoirBufferPT      : register(u7, SPACE_U_DEFAULT);
#else
RWStructuredBuffer<ReservoirDI>                     gReservoirBufferDI      : register(u5, SPACE_U_DEFAULT);
RWStructuredBuffer<ReservoirGI>                     gReservoirBufferGI      : register(u6, SPACE_U_DEFAULT);
RWStructuredBuffer<ReservoirPT>                     gReservoirBufferPT      : register(u7, SPACE_U_DEFAULT);
RWStructuredBuffer<ContextSSPoint>                  gContextsPrimaryRay     : register(u8, SPACE_U_DEFAULT);
#endif

// =================== U Registers (Debug) =======================================================================
#if (FEAT_FLAGS_COMP_DBG != 0)

RWStructuredBuffer<DebugErrorInfo>                  gDbgBufferErrorInfo     : register(u0, SPACE_U_DEBUG);
RWStructuredBuffer<RayDump>                         gPathDump               : register(u1, SPACE_U_DEBUG);
RWByteAddressBuffer                                 gPathDumpDebugOutput    : register(u2, SPACE_U_DEBUG);
RWStructuredBuffer<uint>                            gPerPixelMutex          : register(u3, SPACE_U_DEBUG);

#endif
// =================== S Registers ===============================================================================

SamplerState                                        gSamplerLinearClamp     : register(s0);
SamplerState                                        gSamplerLinearWrap      : register(s1);

#endif