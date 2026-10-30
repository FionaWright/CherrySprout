#ifndef H_RESERVOIR_BUFFER_H
#define H_RESERVOIR_BUFFER_H

/*

Reservoir Buffer:
W = Frame Width
H = Frame Height

N = WxH

- ReservoirxN - Reservoirs Ping
- ReservoirxN - Reservoirs Pong
- ReservoirxN - Previous Frame Reservoirs

Initial Candidates -> (Ping)
Temporal (Ping -> Pong, Prev)
Spatial (Pong -> Ping)

*/

uint GetReservoirBufferBase_Ping()
{
    return 0;
}
uint GetReservoirBufferBase_Pong()
{
    return gSettings.FrameDimensions.x * gSettings.FrameDimensions.y;
}
uint GetReservoirBufferBase_Prev()
{
    return 2 * gSettings.FrameDimensions.x * gSettings.FrameDimensions.y;
}
uint GetReservoirBufferIndexLocal(uint2 pixelCoord)
{
    return pixelCoord.y * gSettings.FrameDimensions.x + pixelCoord.x;
}

uint GetReservoirBufferIndex_Ping(uint2 pixelCoord)
{
    return GetReservoirBufferBase_Ping() + GetReservoirBufferIndexLocal(pixelCoord);
}
uint GetReservoirBufferIndex_Pong(uint2 pixelCoord)
{
    return GetReservoirBufferBase_Pong() + GetReservoirBufferIndexLocal(pixelCoord);
}
uint GetReservoirBufferIndex_Previous(uint2 pixelCoord)
{
    return GetReservoirBufferBase_Prev() + GetReservoirBufferIndexLocal(pixelCoord);
}

uint GetReservoirBufferIndex_InitialCandidatesOutput(uint2 pixelCoord)
{
    return GetReservoirBufferIndex_Ping(pixelCoord);
}

uint GetReservoirBufferIndex_TemporalInput(uint2 pixelCoord)
{
    return GetReservoirBufferIndex_Ping(pixelCoord);
}
uint GetReservoirBufferIndex_TemporalOutput(uint2 pixelCoord)
{
    return GetReservoirBufferIndex_Pong(pixelCoord);
}

uint GetReservoirBufferIndex_SpatialInput(uint2 pixelCoord)
{
    if (gRestirSettings.TemporalEnabled)
        return GetReservoirBufferIndex_Pong(pixelCoord);
    else
        return GetReservoirBufferIndex_Ping(pixelCoord);
}
uint GetReservoirBufferIndex_SpatialOutput(uint2 pixelCoord)
{
    if (gRestirSettings.TemporalEnabled)
        return GetReservoirBufferIndex_Ping(pixelCoord);
    else
        return GetReservoirBufferIndex_Pong(pixelCoord);
}

uint GetReservoirBufferIndex_Final(uint2 pixelCoord)
{
    if (gRestirSettings.TemporalEnabled)
    {
        if (gRestirSettings.SpatialEnabled)
            return GetReservoirBufferIndex_Ping(pixelCoord);
        else
            return GetReservoirBufferIndex_Pong(pixelCoord);
    }

    if (gRestirSettings.SpatialEnabled)
        return GetReservoirBufferIndex_Pong(pixelCoord);
    else
        return GetReservoirBufferIndex_Ping(pixelCoord);
}

uint GetContextBufferBase_Ping()
{
    return 0;
}
uint GetContextBufferBase_Pong()
{
    return gSettings.FrameDimensions.x * gSettings.FrameDimensions.y;
}

uint GetContextBufferIndex_Local(uint2 pixelCoord)
{
    return pixelCoord.y * gSettings.FrameDimensions.x + pixelCoord.x;
}

uint GetContextBufferIndex_Ping(uint2 pixelCoord)
{
    return GetContextBufferIndex_Local(pixelCoord) + GetContextBufferBase_Ping();
}
uint GetContextBufferIndex_Pong(uint2 pixelCoord)
{
    return GetContextBufferIndex_Local(pixelCoord) + GetContextBufferBase_Pong();
}

uint GetContextBufferIndex(uint2 pixelCoord)
{
    return (gSettings.FrameIdx & 1) == 0 ? GetContextBufferIndex_Ping(pixelCoord) : GetContextBufferIndex_Pong(pixelCoord);
}
uint GetContextBufferIndex_Prev(uint2 pixelCoord)
{
    return (gSettings.FrameIdx & 1) == 0 ? GetContextBufferIndex_Pong(pixelCoord) : GetContextBufferIndex_Ping(pixelCoord);
}

#endif