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
    gReservoirBufferDI[reservoirIdxPing] = CreateReservoir<CandidateDI>();

    uint reservoirIdxPong = GetReservoirBufferIndex_Pong(pixelCoord);
    gReservoirBufferDI[reservoirIdxPong] = CreateReservoir<CandidateDI>();

    uint reservoirIdxPrev = GetReservoirBufferIndex_Previous(pixelCoord);
    gReservoirBufferDI[reservoirIdxPrev] = CreateReservoir<CandidateDI>();
}
