#include "SceneLoaderUSD.h"
#include "Scene/SceneCPU.h"

#include <iostream>
#include <ostream>
#include <iomanip>

#include <windows.h>
#include "Utils/CppUtils.h"

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

            << "Albedo=("
            << mat.Albedo.x << ", "
            << mat.Albedo.y << ", "
            << mat.Albedo.z << ", "
            << mat.Albedo.w << ") "

            << "EmissiveStrength=" << mat.EmissiveStrength << " "
            << "EmissiveColor=("
            << mat.EmissiveColor.x << ", "
            << mat.EmissiveColor.y << ", "
            << mat.EmissiveColor.z << ") "

            << "TransmissionFactor=" << mat.TransmissionFactor << " "
            << "TransmissionColor=("
            << mat.TransmissionColor.x << ", "
            << mat.TransmissionColor.y << ", "
            << mat.TransmissionColor.z << ") "

            << "Roughness=" << mat.Roughness << " "
            << "Metallic=" << mat.Metallic << " "
            << "SpecularFactor=" << mat.SpecularFactor << " "
            << "AnisoStrength=" << mat.AnisoStrength << " "

            << "IOR_N=" << mat.IOR_N << " "

            << "TexIdxAlbedo=" << mat.TexIdxAlbedo << " "
            << "TexIdxNormal=" << mat.TexIdxNormal << " "
            << "TexIdxRoughness=" << mat.TexIdxRoughness << " "

            << "TexIdxMetallic=" << mat.TexIdxMetallic << " "
            << "TexIdxEmissive=" << mat.TexIdxEmissive << " "
            << "TexIdxAnisotropy=" << mat.TexIdxAnisotropy << " "
            << "TexIdxClearcoat=" << mat.TexIdxClearcoat << " "

            << "TexIdxClearcoatRoughness=" << mat.TexIdxClearcoatRoughness << " "
            << "TexIdxClearcoatNormal=" << mat.TexIdxClearcoatNormal << " "
            << "TexIdxSheenColor=" << mat.TexIdxSheenColor << " "
            << "TexIdxSheenRoughness=" << mat.TexIdxSheenRoughness << " "

            << "TexIdxTransmissionFactor=" << mat.TexIdxTransmissionFactor << " "
            << "TexIdxORM=" << mat.TexIdxORM << " "
            << "TexIdxOGM=" << mat.TexIdxOGM

            << '\n';
    }

    std::cout << "\n----- Objects -----\n";

    for (size_t i = 0; i < scene->ObjectCount; ++i)
    {
        const Object& obj = scene->Objects[i];

        std::cout << "[" << i << "] ";

#if !NDEBUG
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
        std::cout << "[" << i << "] " << wstringToString(scene->TextureFilepaths[i]) << std::endl;
    }


    std::cout << "\n----- Punctual Lights -----\n";

    for (size_t i = 0; i < scene->MegaBufferPunctualLightsCount; ++i)
    {
        const PunctualLight& light = scene->MegaBufferPunctualLights[i];

        std::cout
            << "[" << i << "] "
            << "Type=";

        switch (light.Type)
        {
        case PunctualLightType::ePoint:
            std::cout << "Point";
            break;

        case PunctualLightType::eSpot:
            std::cout << "Spot";
            break;

        case PunctualLightType::eRect:
            std::cout << "Rect";
            break;

        default:
            std::cout << "Unknown";
            break;
        }

        std::cout
            << " Position=("
            << light.Position.x << ", "
            << light.Position.y << ", "
            << light.Position.z << ")"

            << " Color=("
            << light.Color.x << ", "
            << light.Color.y << ", "
            << light.Color.z << ")"

            << " Intensity=" << light.Intensity

            << " PointRadius=" << light.PointRadius

            << " SpotInnerAngle=" << light.SpotInnerAngle
            << " SpotOuterAngle=" << light.SpotOuterAngle

            << " SpotOrRectDirection=("
            << light.SpotOrRectDirection.x << ", "
            << light.SpotOrRectDirection.y << ", "
            << light.SpotOrRectDirection.z << ")"

            << " RectWidth=" << light.RectWidth
            << " RectHeight=" << light.RectHeight

            << '\n';
    }

    std::cout << "=================================\n";
}

int main(const int argc, char** argv)
{
    //std::string filepath = R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\assets\full_assets\OpenChessSet\chess_set.usda)";
    //std::string filepath = R"(C:\Users\fionawright\build\CherrySprout\Debug\Assets\Scenes\USD\OpenChessSet\chess_set.usda)";
    //std::string filepath = R"(C:\Users\fionawright\build\CherrySprout\Release\Assets\Scenes\USD\OpenChessSet\chess_set.usda)";
    //std::string filepath = R"(C:\Users\fionawright\build\CherrySprout\Debug\Assets\Scenes\USD\GitIgnored\CityStreet\CityStreet.usda)";
    //std::string filepath = R"(C:\Users\fionawright\build\CherrySprout\Debug\Assets\Scenes\USD\GitIgnored\CityStreet\CityStreetMin.usda)";
    //std::string filepath = R"(C:\Users\fionawright\build\CherrySprout\Debug\Assets\Scenes\USD\GitIgnored\Bedroom\Bedroom.usdc)";
    //std::string filepath = R"(C:\Users\fionawright\build\CherrySprout\Debug\Assets\Scenes\USD\GradientTest\GradientTestMin.usda)";
    std::string filepath = R"(C:\Users\fionawright\build\CherrySprout\Debug\Assets\Scenes\USD\GradientTest\GradientTest.usda)";
    //std::string filepath = R"(C:\Users\fiona\build\CherrySprout\Debug\Assets\Scenes\USD\Quad\Quad.usda)";
    //std::string filepath = R"(C:\Users\fiona\build\CherrySprout\Debug\Assets\Scenes\USD\OpenChessSet\chess_set.usda)";
    //std::string filepath = R"(C:\Users\fiona\source\repos\CherrySprout\Assets\Scenes\USD\GitIgnored\LivingRoom\Modernes Wohnzimmer.usda)";
    //std::string filepath = R"(C:\Users\fiona\source\repos\CherrySprout\Assets\Scenes\USD\GitIgnored\Sponza\Sponza.usda)";
    //std::string filepath = R"(C:\Users\fiona\source\repos\CherrySprout\Assets\Scenes\USD\Cornell\Cornell.usda)";
    //std::string filepath = R"(C:\Users\fiona\source\repos\CherrySprout\Assets\Scenes\USD\GitIgnored\Bistro\Bistro.usdc)";
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