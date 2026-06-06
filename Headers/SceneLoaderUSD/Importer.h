//
// Created by fionaw on 30/05/2026.
//

#ifndef CHERRYSPROUT_IMPORTER_H
#define CHERRYSPROUT_IMPORTER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "Scene/Material.h"
#include "Scene/Vertex.h"

namespace SceneLoaderUSD
{
    struct ImporterObject
    {
        std::string Name = "Unnamed Object";

        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;

        float M[16];
        int MaterialIndex = -1;
    };

    struct ImporterContext
    {
        std::vector<ImporterObject> Objects;
        std::vector<Material> Materials;
        std::vector<const char*> TextureFilePaths;
    };

    ImporterContext Import(const char* usdPath);
}


#endif //CHERRYSPROUT_IMPORTER_H