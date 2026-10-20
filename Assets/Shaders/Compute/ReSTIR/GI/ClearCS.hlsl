#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/DI.hlsli"

#include "Compute/ReSTIR/SeedDecorrelators.h"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x > gSettings.FrameDimensions.x || pixelCoord.y > gSettings.FrameDimensions.y)
        return;

    uint reservoirIdxPing = GetReservoirBufferIndex_Ping(pixelCoord);
    gReservoirBufferGI[reservoirIdxPing] = CreateReservoir<CandidateGI>();

    uint reservoirIdxPong = GetReservoirBufferIndex_Pong(pixelCoord);
    gReservoirBufferGI[reservoirIdxPong] = CreateReservoir<CandidateGI>();

    uint reservoirIdxPrev = GetReservoirBufferIndex_Previous(pixelCoord);
    gReservoirBufferGI[reservoirIdxPrev] = CreateReservoir<CandidateGI>();
}
