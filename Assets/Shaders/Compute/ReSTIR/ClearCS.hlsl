#include "Utils/SharedUtils.h"
#include "Utils/HlslUtils.hlsli"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/Structs.h"

#include "Compute/ReSTIR/SeedDecorrelators.h"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x > gSettings.FrameDimensions.x || pixelCoord.y > gSettings.FrameDimensions.y)
        return;

    uint reservoirIdxPing = GetReservoirBufferIndex_Ping(pixelCoord);
    uint reservoirIdxPong = GetReservoirBufferIndex_Pong(pixelCoord);
    uint reservoirIdxPrev = GetReservoirBufferIndex_Previous(pixelCoord);

    if (FEAT_ADV(RestirDI))
    {
        gReservoirBufferDI[reservoirIdxPing] = INIT0(ReservoirDI);
        gReservoirBufferDI[reservoirIdxPong] = INIT0(ReservoirDI);
        gReservoirBufferDI[reservoirIdxPrev] = INIT0(ReservoirDI);
    }

    if (FEAT_ADV(RestirGI))
    {
        gReservoirBufferGI[reservoirIdxPing] = INIT0(ReservoirGI);
        gReservoirBufferGI[reservoirIdxPong] = INIT0(ReservoirGI);
        gReservoirBufferGI[reservoirIdxPrev] = INIT0(ReservoirGI);
    }

    if (FEAT_ADV(RestirPT))
    {
        gReservoirBufferPT[reservoirIdxPing] = INIT0(ReservoirPT);
        gReservoirBufferPT[reservoirIdxPong] = INIT0(ReservoirPT);
        gReservoirBufferPT[reservoirIdxPrev] = INIT0(ReservoirPT);
    }
}
