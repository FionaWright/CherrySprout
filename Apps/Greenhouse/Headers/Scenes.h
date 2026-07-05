#ifndef H_SCENES_H
#define H_SCENES_H

#include <DirectXMath.h>
#include <string>
#include <vector>

#include "Utils/Constants.h"

using namespace DirectX;

struct SceneConfig
{
    std::string Name;
    std::string Filepath;

    XMFLOAT3 CameraPosition{0, 0, 0};
    XMFLOAT2 CameraPitchYaw{0, PI};
    float SceneScale = 1.0f;
};

inline std::vector<SceneConfig> s_sceneConfigs = {
    {
        .Name = "Cube",
        .Filepath = "Scenes/USD/Cube/Cube.usda",
        .CameraPosition = {-0.42048344, 0.180789918, -0.327038676},
        .CameraPitchYaw = {0.292135, 0.910922825},
        .SceneScale = 10.0f
    },

    {
        .Name = "Chess",
        .Filepath = "Scenes/USD/OpenChessSet/chess_set.usda",
        .CameraPosition = {-0.114393, 0.159540, -0.291603},
        .CameraPitchYaw = {0.251301, 0.375632},
    },

    {
        .Name = "LightTest",
        .Filepath = "Scenes/USD/LightTest/LightTest.usda",
        .CameraPosition = {-0.114393, 0.159540, -0.291603},
        .CameraPitchYaw = {0.251301, 0.375632},
    },

    {
        .Name = "Living Room",
        .Filepath = "Scenes/USD/GitIgnored/LivingRoom/Modernes Wohnzimmer.usda",
        .CameraPosition = {-0.114393, 0.159540, -0.291603},
        .CameraPitchYaw = {0.251301, 0.375632},
    },
};

#endif
