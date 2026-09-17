#ifndef H_PATH_DUMP_OUTPUT_COLOR_H
#define H_PATH_DUMP_OUTPUT_COLOR_H

uint GetByteOffset(uint rayIdx, uint dbgIdx)
{
    return rayIdx * sizeof(DebugOutputStruct) + dbgIdx * sizeof(DebugOutputPair);
}

void LoadOutputColor(uint rayIdx, uint dbgIdx, out float3 value, out uint isAssignedValue)
{
    uint offset = GetByteOffset(rayIdx, dbgIdx);
    uint4 bytes = gPathDumpDebugOutput.Load4(offset);

    value = float3(asfloat(bytes.x), asfloat(bytes.y), asfloat(bytes.z));
    isAssignedValue = bytes.w;
}

void StoreOutputColor(uint rayIdx, uint dbgIdx, float3 value, uint isAssignedValue)
{
    uint4 bytes = uint4(asuint(value.x), asuint(value.y), asuint(value.z), isAssignedValue);

    uint offset = GetByteOffset(rayIdx, dbgIdx);
    gPathDumpDebugOutput.Store4(offset, bytes);
}

[noinline]
void AssignDebugOutput(float value_x, float value_y, float value_z, uint dbgIdx)
{
    if (!DEBUG_ENABLED(PathDumper))
        return;

    if (any(gDebugPixelCoord != gDebugSettings.ChosenPixelCoords))
        return;

    float3 _;
    uint isAlreadyAssigned;
    LoadOutputColor(gDebugCurrentRayDepth, dbgIdx, _, isAlreadyAssigned);

    if (isAlreadyAssigned)
        return;

    float3 value = float3(value_x, value_y, value_z);

    StoreOutputColor(gDebugCurrentRayDepth, dbgIdx, value, true);
}

#endif