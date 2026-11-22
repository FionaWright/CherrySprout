#include "Utils/SharedUtils.h"
#include "Utils/HlslUtils.hlsli"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/Structs.h"

#include "Compute/ReSTIR/SeedDecorrelators.h"

template <typename TReservoir>
void ClearReservoir(RWStructuredBuffer< TReservoir > buffer, uint idx)
{
    buffer[idx] = INIT0(TReservoir);
}

template <typename TReservoir>
void ClearAllReservoirs(RWStructuredBuffer< TReservoir > buffer, uint idxPing, uint idxPong, uint idxPrev)
{
    ClearReservoir(buffer, idxPing);
    ClearReservoir(buffer, idxPong);
    ClearReservoir(buffer, idxPrev);
}

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
        ClearAllReservoirs(gReservoirBufferDI, reservoirIdxPing, reservoirIdxPong, reservoirIdxPrev);
    }

    if (FEAT_ADV(RestirGI))
    {
        ClearAllReservoirs(gReservoirBufferGI, reservoirIdxPing, reservoirIdxPong, reservoirIdxPrev);
    }

    if (FEAT_ADV(RestirPT))
    {
        ClearAllReservoirs(gReservoirBufferPT, reservoirIdxPing, reservoirIdxPong, reservoirIdxPrev);
    }
}
