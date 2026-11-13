#ifndef H_GDPT_UTILS_H
#define H_GDPT_UTILS_H

#include "PathTracing/Shift/Structs.h"

void AddVertex(inout PathVertexList vList, PathVertexInfo vInfo)
{
    if (vList.NumVertices == MAX_RAY_DEPTH_SHIFT-1)
        return;

    vList.Array[vList.NumVertices] = vInfo;
    vList.NumVertices++;

    DBG_PATH_DUMP_PATH_VERTEX_INFO(vInfo);
}

void AddVertexEnvMap(inout PathVertexList vList, float3 contrib, float mis_nee)
{
    if (vList.NumVertices == MAX_RAY_DEPTH_SHIFT-1)
        return;

    PathVertexInfo vInfo = (PathVertexInfo)0;
    vInfo.Type = VertexType::eEnvironment;
    vInfo.DirectContribution = contrib;
    vInfo.MIS_NEE = mis_nee;

    AddVertex(vList, vInfo);
}

void AddVertexEmissive(inout PathVertexList vList, float3 emission, float mis_nee, float3 hitPos, float3 Ng_ff, float roughness)
{
    if (vList.NumVertices == MAX_RAY_DEPTH_SHIFT-1)
        return;

    PathVertexInfo vInfo = (PathVertexInfo)0;
    vInfo.Type = VertexType::eEmission;
    vInfo.DirectContribution = emission;
    vInfo.MIS_NEE = mis_nee;
    vInfo.Position = hitPos;
    vInfo.Ng_ff = Ng_ff;
    vInfo.Roughness = roughness;

    AddVertex(vList, vInfo);
}

#endif