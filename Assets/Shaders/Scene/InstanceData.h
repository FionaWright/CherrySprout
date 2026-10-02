#ifndef H_INSTANCE_DATA_H
#define H_INSTANCE_DATA_H

#include "Utils/HlslGlue.h"

struct InstanceData
{
    hlsl::float4x4 M, MTI;

    hlsl::uint MegaBufferOffsetVertex;
    hlsl::uint MegaBufferOffsetIndex;
    hlsl::uint MegaBufferCountIndex;
    hlsl::uint MaterialIndex;
};

#endif