#ifndef H_EXTRACT_MESH_H
#define H_EXTRACT_MESH_H

#pragma warning(push)
#pragma warning(disable : 4244 4305 4996 4267 4003)

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/assetPath.h"

#pragma warning(pop)

#include "Importer.h"
#include "SceneLoaderUSD.h"

namespace SceneLoaderUSD
{
    struct VertexHasher
    {
        size_t operator()(const Vertex& v) const
        {
            size_t h = 0;

            auto hashCombine = [&](size_t value)
            {
                h ^= value + 0x9e3779b9 + (h << 6) + (h >> 2);
            };

            hashCombine(std::hash<float>{}(v.Position.x));
            hashCombine(std::hash<float>{}(v.Position.y));
            hashCombine(std::hash<float>{}(v.Position.z));

            hashCombine(std::hash<float>{}(v.Normal.x));
            hashCombine(std::hash<float>{}(v.Normal.y));
            hashCombine(std::hash<float>{}(v.Normal.z));

            hashCombine(std::hash<float>{}(v.UV.x));
            hashCombine(std::hash<float>{}(v.UV.y));

            return h;
        }
    };

    inline void ExtractGeometry(const pxr::UsdPrim& prim, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {
        pxr::UsdGeomMesh mesh(prim);
        pxr::VtArray<pxr::GfVec3f> points;
        mesh.GetPointsAttr().Get(&points);

        pxr::VtArray<int> faceVertexCounts;
        mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);

        pxr::VtArray<int> faceVertexIndices;
        mesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

        pxr::VtArray<pxr::GfVec3f> normals;
        mesh.GetNormalsAttr().Get(&normals);
        pxr::TfToken normalsInterp = mesh.GetNormalsInterpolation();

        pxr::UsdGeomPrimvarsAPI primvarsAPI(prim);
        pxr::UsdGeomPrimvar stPrimvar = primvarsAPI.GetPrimvar(pxr::TfToken("st"));

        pxr::VtArray<pxr::GfVec2f> uvs;
        pxr::VtArray<int> uvIndices;
        pxr::TfToken uvInterp;
        bool hasUV = false;

        if (stPrimvar && stPrimvar.IsDefined())
        {
            stPrimvar.Get(&uvs);
            stPrimvar.Get(&uvIndices);
            uvInterp = stPrimvar.GetInterpolation();
            hasUV = !uvs.empty();
        }

        struct PxrVertex
        {
            pxr::GfVec3f Position;
            pxr::GfVec3f Normal;
            pxr::GfVec2f UV;
        };

        // Ignoring flipping for now

        std::vector<pxr::GfVec3d> sumNormals(points.size(), pxr::GfVec3d(0,0,0));
        std::vector<bool> hasExplicitNormals(points.size(), false);

        const size_t maxVertexCount = faceVertexCounts.size() * 4; // Assuming all faces are quads
        std::vector<PxrVertex> pxrVertices(maxVertexCount);

        const int normalsCount = static_cast<int>(normals.size());

        int currVertexIndex = 0;

        for (size_t faceIdx = 0; faceIdx < faceVertexCounts.size(); ++faceIdx)
        {
            const int verticesInFace = faceVertexCounts[faceIdx];

            for (int v = 0; v < verticesInFace; ++v)
            {
                const int pvIdx = currVertexIndex + v;
                const int ptIdx = faceVertexIndices[pvIdx];

                pxrVertices.at(pvIdx).Position = points[ptIdx];

                if (hasUV)
                {
                    int uvIdx = -1;
                    if (uvInterp == pxr::UsdGeomTokens->faceVarying)
                    {
                        uvIdx = uvIndices.empty() ? pvIdx : uvIndices[pvIdx];
                    }
                    else if (uvInterp == pxr::UsdGeomTokens->vertex)
                    {
                        uvIdx = uvIndices.empty() ? ptIdx : uvIndices[pvIdx];
                    }

                    if (uvIdx >= 0 && uvIdx < uvs.size())
                        pxrVertices.at(pvIdx).UV = uvs[uvIdx];
                }

                if (!normals.empty() && normalsInterp == pxr::UsdGeomTokens->faceVarying)
                {
                    if (pvIdx < normalsCount)
                    {
                        pxrVertices.at(pvIdx).Normal = normals[pvIdx];
                        hasExplicitNormals.at(ptIdx) = true;
                    }
                }
                else if (!normals.empty() && (normalsInterp == pxr::UsdGeomTokens->vertex || normalsInterp == pxr::UsdGeomTokens->varying))
                {
                    if (ptIdx < normalsCount)
                    {
                        pxrVertices.at(pvIdx).Normal = normals[ptIdx];
                        hasExplicitNormals.at(ptIdx) = true;
                    }
                }
            }

            const int v0 = currVertexIndex + 0;
            const int v1 = currVertexIndex + 1;
            const int v2 = currVertexIndex + 2;

            pxr::GfVec3f e1 = pxrVertices[v1].Position - pxrVertices[v0].Position;
            pxr::GfVec3f e2 = pxrVertices[v2].Position - pxrVertices[v0].Position;
            pxr::GfVec3d faceN = pxr::GfCross(e1, e2);

            for (int v = 0; v < verticesInFace; ++v)
            {
                const int pvIdx = currVertexIndex + v;
                const int ptIdx = faceVertexIndices[pvIdx];
                if (!hasExplicitNormals.at(ptIdx))
                {
                    sumNormals.at(ptIdx) += faceN;
                }
            }

            currVertexIndex += verticesInFace;
        }

        currVertexIndex = 0;

        std::unordered_map<Vertex, uint32_t, VertexHasher> vertexLookup;
        vertexLookup.reserve(maxVertexCount);

        for (size_t faceIdx = 0; faceIdx < faceVertexCounts.size(); ++faceIdx)
        {
            const int verticesInFace = faceVertexCounts[faceIdx];

            for (int v = 1; v + 1 < verticesInFace; ++v)
            {
                int v1 = v;
                int v2 = v + 1;
                for (int vi : { 0, v1, v2 })
                {
                    const int pvIdx = currVertexIndex + vi;
                    const int ptIdx = faceVertexIndices[pvIdx];

                    const PxrVertex pxrv = pxrVertices.at(pvIdx);

                    Vertex vertex;
                    memcpy(&vertex.Position, &pxrv.Position, sizeof(float) * 3);
                    memcpy(&vertex.UV, &pxrv.UV, sizeof(float) * 2);

                    pxr::GfVec3d N;
                    if (hasExplicitNormals.at(ptIdx))
                    {
                        N = pxrv.Normal;
                    }
                    else
                    {
                        N = sumNormals.at(ptIdx);
                    }
                    N.Normalize();

                    vertex.Normal = { (float)N[0], (float)N[1], (float)N[2]};

                    auto it = vertexLookup.find(vertex);

                    if (it != vertexLookup.end())
                    {
                        indices.emplace_back(it->second);
                        continue;
                    }

                    const uint32_t newIndex =
                            static_cast<uint32_t>(vertices.size());

                    vertices.emplace_back(vertex);
                    vertexLookup.emplace(vertex, newIndex);

                    indices.emplace_back(newIndex);
                }
            }

            currVertexIndex += verticesInFace;
        }
    }

    inline ImporterObject ExtractMesh(const pxr::UsdPrim& prim, pxr::UsdGeomXformCache& xformCache, const std::unordered_map<std::string, size_t>& matPathToIdxMap)
    {
        ImporterObject obj;
        ExtractGeometry(prim, obj.Vertices, obj.Indices);

        pxr::GfMatrix4d xform = xformCache.GetLocalToWorldTransform(pxr::UsdGeomMesh(prim).GetPrim());

        // TODO: Guessing
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
            {
                const int i = r * 4 + c;
                obj.M[i] = static_cast<float>(xform[r][c]);
            }

        const pxr::UsdShadeMaterial boundMat = pxr::UsdShadeMaterialBindingAPI(prim).ComputeBoundMaterial(pxr::UsdShadeTokens->full);
        if (boundMat)
        {
            const std::string& path = boundMat.GetPrim().GetPath().GetString();
            const auto it = matPathToIdxMap.find(path);
            if (it != matPathToIdxMap.end())
                obj.MaterialIndex = static_cast<int>(it->second);
        }

        obj.Name = boundMat ? boundMat.GetPrim().GetName().GetString() : prim.GetName().GetString();

        return obj;
    }
}

#endif