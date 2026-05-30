//
// Created by fionaw on 30/05/2026.
//

#include "Processor.h"

#include <iostream>
#include <ostream>

void SceneLoaderUSD::Process(const ImporterContext& importerContext, SceneCPU* scene)
{
    std::cout << "Processing scene..." << std::endl;

    scene->MegaBufferVertex.clear();
    scene->MegaBufferIndex.clear();
    scene->MegaBufferMaterials.clear();

    // TODO: Load images here? Makes sense to me

    scene->MegaBufferMaterials = importerContext.Materials;
    scene->TextureFilepaths = importerContext.TextureFilePaths;

    for (const auto& [Name, Vertices, Indices, M, MaterialIndex] : importerContext.Objects)
    {
        Object obj;

#ifdef _DEBUG
        obj.DebugName = Name;
#endif

        obj.MegaBufferVertexOffset = static_cast<uint32_t>(scene->MegaBufferVertex.size());
        obj.MegaBufferIndexOffset = static_cast<uint32_t>(scene->MegaBufferIndex.size());

        scene->MegaBufferVertex.insert(std::end(scene->MegaBufferVertex), std::begin(Vertices), std::end(Vertices));
        scene->MegaBufferIndex.insert(std::end(scene->MegaBufferIndex), std::begin(Indices), std::end(Indices));

        obj.MegaBufferIndexCount = static_cast<uint32_t>(scene->MegaBufferIndex.size()) - obj.MegaBufferIndexOffset;

        memcpy(obj.M, M, sizeof(float)*16);

        obj.MaterialIndex = MaterialIndex;

        scene->Objects.emplace_back(std::move(obj));
    }

    std::cout << "Scene processed" << std::endl;
}
