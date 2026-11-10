#ifndef H_GRADIENT_DOMAIN_STRUCTS_H
#define H_GRADIENT_DOMAIN_STRUCTS_H

#include "BxDFs/Structs.h"
#include "Utils/HlslGlue.h"

#include "PathTracing/Structs.h"
#include "PathTracing/NEE/Structs.h"

enum GradientEstimator
{
    eForward,
    eBackward
};

struct Gradients
{
    hlsl::float3 XForward;
    hlsl::float3 XBackward;
    hlsl::float3 YForward;
    hlsl::float3 YBackward;
};

#endif