#include "SceneLoaderUSD.h"
#include "Scene/SceneCPU.h"

#include <iostream>
#include <ostream>
#include <iomanip>

void PrintSceneDebug(const SceneCPU* scene)
{
    std::cout << "\n========== Scene Debug ==========\n";

    std::cout << "Objects:      " << scene->ObjectCount << '\n';
    std::cout << "Materials:    " << scene->MegaBufferMaterialsCount << '\n';
    std::cout << "Textures:     " << scene->TextureFilepathCount << '\n';
    std::cout << "Vertices:     " << scene->MegaBufferVertexCount << '\n';
    std::cout << "Indices:      " << scene->MegaBufferIndexCount << '\n';

    std::cout << "\n----- Materials -----\n";

    for (size_t i = 0; i < scene->MegaBufferMaterialsCount; ++i)
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

    for (size_t i = 0; i < scene->ObjectCount; ++i)
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

        if (obj.MaterialIndex >= scene->MegaBufferMaterialsCount)
        {
            std::cout
                << "    WARNING: Invalid material index ("
                << obj.MaterialIndex << ")\n";
        }
    }

    std::cout << "\n----- Textures -----\n";

    for (size_t i = 0; i < scene->TextureFilepathCount; ++i)
    {
        std::cout << "[" << i << "] "
                  << scene->TextureFilepaths[i]
                  << '\n';
    }

    std::cout << "=================================\n";
}

int main(const int argc, char** argv)
{
    //std::string filepath = R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\assets\full_assets\OpenChessSet\chess_set.usda)";
    //std::string filepath = R"(C:\Users\fionawright\build\CherrySprout\Debug\Assets\Scenes\USD\OpenChessSet\chess_set.usda)";
    //std::string filepath = R"(C:\Users\fiona\source\repos\CherrySprout\Assets\Scenes\USD\GitIgnored\LivingRoom\Modernes Wohnzimmer.usda)";
    //std::string filepath = R"(C:\Users\fiona\source\repos\CherrySprout\Assets\Scenes\USD\GitIgnored\Sponza\Sponza.usda)";
    std::string filepath = R"(C:\Users\fiona\source\repos\CherrySprout\Assets\Scenes\USD\GitIgnored\Bistro\Bistro.usdc)";
    //std::string filepath = R"(C:\Users\fiona\source\repos\CherrySprout\Assets\Scenes\USD\LightTest\LightTest.usda)";
    //std::string filepath = R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\Cube\Cube.usda)";
    SceneCPU scene;
    LoadUSD(filepath.c_str(), 1.0f, &scene);

    for (size_t i = 0; i < scene.ObjectCount; ++i)
    {
        const Object& obj = scene.Objects[i];

        assert(obj.MegaBufferVertexOffset < scene.MegaBufferVertexCount);
        assert(obj.MegaBufferIndexOffset < scene.MegaBufferIndexCount);
        assert(obj.MegaBufferIndexOffset + obj.MegaBufferIndexCount <= scene.MegaBufferIndexCount);
    }

    PrintSceneDebug(&scene);

    FreeScene(&scene);
}