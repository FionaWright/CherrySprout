//
// Created by fionaw on 30/05/2026.
//

#include "Processor.h"

#include <iostream>
#include <ostream>

void SceneLoaderUSD::Process(const ImporterContext& importerContext, SceneCPU* scene)
{
    std::cout << "Processing scene..." << std::endl;

    *scene = {};

    scene->MegaBufferMaterials = new Material[importerContext.Materials.size()];
    scene->MegaBufferMaterialsCount = importerContext.Materials.size();
    memcpy(scene->MegaBufferMaterials, importerContext.Materials.data(), scene->MegaBufferMaterialsCount * sizeof(Material));

    scene->TextureFilepathCount = importerContext.TextureFilePaths.size();
    scene->TextureFilepaths = static_cast<char**>(malloc(scene->TextureFilepathCount * sizeof(char*)));
    for (size_t i = 0; i < scene->TextureFilepathCount; i++)
    {
        scene->TextureFilepaths[i] = _strdup(importerContext.TextureFilePaths[i]);
    }

    size_t totalVertexCount = 0;
    size_t totalIndexCount = 0;
    for (const auto & object : importerContext.Objects)
    {
        totalVertexCount += object.Vertices.size();
        totalIndexCount += object.Indices.size();
    }
    scene->MegaBufferVertex = new Vertex[totalVertexCount];
    scene->MegaBufferIndex = new uint32_t[totalIndexCount];

    scene->ObjectCount = importerContext.Objects.size();
    scene->Objects = new Object[scene->ObjectCount];
    for (size_t i = 0; i < scene->ObjectCount; i++)
    {
        const ImporterObject& impObj = importerContext.Objects[i];
        Object obj;

#ifdef _DEBUG
        // TODO
        //obj.DebugName = Name.c_str();
#endif

        obj.MegaBufferVertexOffset  = static_cast<uint32_t>(scene->MegaBufferVertexCount);
        obj.MegaBufferIndexOffset   = static_cast<uint32_t>(scene->MegaBufferIndexCount);
        obj.MegaBufferVertexCount   = impObj.Vertices.size();
        obj.MegaBufferIndexCount    = impObj.Indices.size();

        Vertex* vAddr = scene->MegaBufferVertex + obj.MegaBufferVertexOffset;
        uint32_t* iAddr = scene->MegaBufferIndex + obj.MegaBufferIndexOffset;
        memcpy(vAddr, impObj.Vertices.data(), impObj.Vertices.size() * sizeof(Vertex));
        memcpy(iAddr, impObj.Indices.data(), impObj.Indices.size() * sizeof(uint32_t));
        scene->MegaBufferVertexCount += impObj.Vertices.size();
        scene->MegaBufferIndexCount += impObj.Indices.size();

        memcpy(obj.M, impObj.M, sizeof(float)*16);

        obj.MaterialIndex = impObj.MaterialIndex;

        scene->Objects[i] = obj;
    }

    assert(totalVertexCount == scene->MegaBufferVertexCount);
    assert(totalIndexCount == scene->MegaBufferIndexCount);

    if (importerContext.PunctualLights.empty())
    {
        scene->MegaBufferPunctualLightsCount = 1;
        scene->MegaBufferPunctualLights = new PunctualLight[1];
        scene->MegaBufferPunctualLights[0] = PunctualLight();
    }
    else
    {
        scene->MegaBufferPunctualLightsCount = importerContext.PunctualLights.size();
        scene->MegaBufferPunctualLights = new PunctualLight[importerContext.PunctualLights.size()];
        memcpy(scene->MegaBufferPunctualLights, importerContext.PunctualLights.data(), importerContext.PunctualLights.size() * sizeof(PunctualLight));
    }

    std::cout << "Scene processed" << std::endl;
}
