#ifndef H_POPULATE_PATH_VERTEX_INFO_H
#define H_POPULATE_PATH_VERTEX_INFO_H

// TODO: It's probably possible to just use array[i+1] and skip this
void BackPropogateVertexInfo(inout PathVertexList vertexList)
{
    for (uint i = vertexList.NumVertices-2; i >= 0; i--)
    {
        PathVertexInfo nextVertexInfo = vertexList.Array[i+1];

        vertexList.Array[i].NextVertexType = nextVertexInfo.Type;
        vertexList.Array[i].NextVertexPosition = nextVertexInfo.Position;
        vertexList.Array[i].NextVertexNormal = nextVertexInfo.SFrame.N;
    }
}

#endif