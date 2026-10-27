#ifndef H_GDPT_UTILS_H
#define H_GDPT_UTILS_H

#include "PathTracing/GradientDomain/Structs.h"

void AddVertex(inout PathVertexList vList, PathVertexInfo vInfo)
{
    vList.Array[vList.NumVertices] = vInfo;
    vList.NumVertices++;

    DBG_PATH_DUMP_PATH_VERTEX_INFO(vInfo);
}

void AddVertexEnvMap(inout PathVertexList vList, float3 contrib)
{
    PathVertexInfo vInfo = (PathVertexInfo)0;
    vInfo.Type = VertexType::eEnvironment;
    vInfo.DirectContribution = contrib;

    AddVertex(vList, vInfo);
}

void AddVertexEmissive(inout PathVertexList vList, float3 hitPos, float3 Ng_ff, float roughness)
{
    PathVertexInfo vInfo = (PathVertexInfo)0;
    vInfo.Type = VertexType::eEmission;
    vInfo.Position = hitPos;
    vInfo.Ng_ff = Ng_ff;
    vInfo.Roughness = roughness;

    AddVertex(vList, vInfo);
}

#endif