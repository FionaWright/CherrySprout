#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/PT.hlsli"

#if FEAT_ADV_D(AreaRestir)
#	include "Compute/ReSTIR/Contexts/SSArea.hlsli"
#else
#	include "Compute/ReSTIR/Contexts/SSPoint.hlsli"
#endif

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gSettings.FrameDimensions.x || pixelCoord.y >= gSettings.FrameDimensions.y)
        return;

    DBG_SET_PIXEL_INFO(pixelCoord, gSettings.FrameIdx);

    SpatialReuse(gReservoirBufferPT, pixelCoord, SEED_DECORRELATOR_PT_SPATIAL);
}
