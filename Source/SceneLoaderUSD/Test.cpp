#include "SceneLoaderUSD.h"
#include "Scene/SceneCPU.h"

#include <iostream>
#include <ostream>
#include <iomanip>

void PrintSceneDebug(const SceneCPU* scene)
{
    std::cout << "\n========== Scene Debug ==========\n";

    std::cout << "Objects:      " << scene->Objects.size() << '\n';
    std::cout << "Materials:    " << scene->MegaBufferMaterials.size() << '\n';
    std::cout << "Textures:     " << scene->TextureFilepaths.size() << '\n';
    std::cout << "Vertices:     " << scene->MegaBufferVertex.size() << '\n';
    std::cout << "Indices:      " << scene->MegaBufferIndex.size() << '\n';

    std::cout << "\n----- Materials -----\n";

    for (size_t i = 0; i < scene->MegaBufferMaterials.size(); ++i)
    {
        const Material& mat = scene->MegaBufferMaterials[i];

        std::cout
            << "[" << i << "] "
            << "BaseColor=("
            << mat.Albedo.x << ", "
            << mat.Albedo.y << ", "
            << mat.Albedo.z << ", "
            << mat.Albedo.w << ") "
            << "Roughness=" << mat.Roughness << " "
            << "Metallic=" << mat.Metallic << " "
            << "Emission=" << mat.EmissiveStrength << " "
            << "AlbedoTex=" << mat.TexIdxAlbedo << " "
            << "NormalTex=" << mat.TexIdxNormal
            << '\n';
    }

    std::cout << "\n----- Objects -----\n";

    for (size_t i = 0; i < scene->Objects.size(); ++i)
    {
        const Object& obj = scene->Objects[i];

        std::cout << "[" << i << "] ";

#ifdef _DEBUG
        std::cout << "\"" << obj.DebugName << "\" ";
#endif

        std::cout
            << "Material=" << obj.MaterialIndex
            << " VertOffset=" << obj.MegaBufferVertexOffset
            << " IdxOffset=" << obj.MegaBufferIndexOffset
            << " IdxCount=" << obj.MegaBufferIndexCount
            << '\n';

        if (obj.MaterialIndex >= scene->MegaBufferMaterials.size())
        {
            std::cout
                << "    WARNING: Invalid material index ("
                << obj.MaterialIndex << ")\n";
        }
    }

    std::cout << "\n----- Textures -----\n";

    for (size_t i = 0; i < scene->TextureFilepaths.size(); ++i)
    {
        std::cout << "[" << i << "] "
                  << scene->TextureFilepaths[i]
                  << '\n';
    }

    std::cout << "=================================\n";
}

int main(const int argc, char** argv)
{
    std::string filepath = R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\assets\full_assets\OpenChessSet\chess_set.usda)";
    //std::string filepath = R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\Cube\Cube.usda)";
    SceneCPU scene;
    LoadUSD(filepath.c_str(), &scene);

    for (const Object& obj : scene.Objects)
    {
        assert(obj.MegaBufferVertexOffset < scene.MegaBufferVertex.size());
        assert(obj.MegaBufferIndexOffset < scene.MegaBufferIndex.size());
        assert(obj.MegaBufferIndexOffset + obj.MegaBufferIndexCount <= scene.MegaBufferIndex.size());
    }

    PrintSceneDebug(&scene);
}