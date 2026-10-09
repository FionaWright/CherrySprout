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
        .SceneScale = 0.1f
    },

    {
        .Name = "Cornell",
        .Filepath = "Scenes/USD/Cornell/Cornell.usda",
        .CameraPosition = {0.093079, 1.183231, -2.560323},
        .CameraPitchYaw = {0.111301, 0.025632},
        .SceneScale = 1.0f
    },

    {
        .Name = "Quad",
        .Filepath = "Scenes/USD/Quad/Quad.usda",
        //.CameraPosition = {1.5, 0.0, 0.0},
        //.CameraPitchYaw = {0.0, -PI/2},
        .CameraPosition = {0.144306, 0.518994, 0.201417},
        .CameraPitchYaw = {0.020000, -0.860796},
        .SceneScale = 1.0f
    },

    {
        .Name = "LightTest2",
        .Filepath = "Scenes/USD/LightTest/LightTest2.usda",
        .CameraPosition = {1.644621, 0.579378, 0.984122},
        .CameraPitchYaw = {0.351301, -2.264367},
        .SceneScale = 0.1f
    },

    {
        .Name = "LightTest3",
        .Filepath = "Scenes/USD/LightTest/LightTest3.usda",
        .CameraPosition = {1.644621, 0.579378, 0.984122},
        .CameraPitchYaw = {0.351301, -2.264367},
        .SceneScale = 0.1f
    },

    {
        .Name = "LightTest4",
        .Filepath = "Scenes/USD/LightTest/LightTest4.usda",
        .CameraPosition = {1.644621, 0.579378, 0.984122},
        .CameraPitchYaw = {0.351301, -2.264367},
        .SceneScale = 0.1f
    },

    {
        .Name = "MC",
        .Filepath = "Scenes/USD/MC/McUsd.usda",
        .CameraPosition = {42.258442, 40.445187, 4.229376},
        .CameraPitchYaw = {0.661301, 4.455630},
        .SceneScale = 0.1f
    },

    {
        .Name = "Sponza",
        .Filepath = "Scenes/USD/GitIgnored/Sponza/Sponza.usda",
        .CameraPosition = {-0.114393, 0.159540, -0.291603},
        .CameraPitchYaw = {0.251301, 0.375632},
    },

    {
        .Name = "Bistro",
        .Filepath = "Scenes/USD/GitIgnored/Bistro/Bistro.usdc",
        .CameraPosition = {-19.381216, 3.343480, 1.085874},
        .CameraPitchYaw = {0.231301, 1.125632},
    },
};

#endif
