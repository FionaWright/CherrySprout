#ifndef H_PATH_REPLAY_H
#define H_PATH_REPLAY_H

// Don't forget about making a mutable RngInfo

float3 ReplayPath(PathSample mainPathSample, float3 shiftedOrigin, float3 shiftedDirection)
{
    /*

    for each vertex:
        if vertex.IsDelta (specular):
            HalfVectorShiftMapping();
        else
            ReconnectShiftMapping();

    */
}

#endif