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

void AddVertexEnvMap(inout PathVertexList vList, float3 contrib)
{
    if (vList.NumVertices == MAX_RAY_DEPTH_SHIFT-1)
        return;

    PathVertexInfo vInfo = (PathVertexInfo)0;
    vInfo.Type = VertexType::eEnvironment;
    vInfo.DirectContribution = contrib;

    AddVertex(vList, vInfo);
}

void AddVertexEmissive(inout PathVertexList vList, float3 L_emisson, float3 hitPos, float3 Ng_ff, float roughness)
{
    if (vList.NumVertices == MAX_RAY_DEPTH_SHIFT-1)
        return;

    PathVertexInfo vInfo = (PathVertexInfo)0;
    vInfo.Type = VertexType::eEmission;
    vInfo.DirectContribution = L_emisson;
    vInfo.Position = hitPos;
    vInfo.Ng_ff = Ng_ff;
    vInfo.Roughness = roughness;

    AddVertex(vList, vInfo);
}

#endif