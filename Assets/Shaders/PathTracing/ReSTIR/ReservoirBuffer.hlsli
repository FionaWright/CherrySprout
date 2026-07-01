#ifndef H_RESERVOIR_BUFFER_H
#define H_RESERVOIR_BUFFER_H

uint GetReservoirBufferIndex_Current(uint2 pixelCoord)
{
    return pixelCoord.y * gSettings.FrameDimensions.x + pixelCoord.x;
}

uint GetReservoirBufferIndex_Previous(uint2 pixelCoord)
{
    uint bufferIdx = pixelCoord.y * gSettings.FrameDimensions.x + pixelCoord.x;
    return bufferIdx + gSettings.FrameDimensions.x * gSettings.FrameDimensions.y;
}

#endif