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

    XMFLOAT3 CameraPosition{-0.42048344, 0.180789918, -0.327038676};
    XMFLOAT2 CameraPitchYaw{0.292135, 0.910922825};
};

inline std::vector<SceneConfig> s_sceneConfigs = {
    {
        .Name = "Cube",
        .Filepath = "Assets/Scenes/USD/Cube/Cube.usda"
    },

    {
        .Name = "Chess",
        .Filepath = "Assets/Scenes/USD/OpenChessSet/chess_set.usda"
    },
};

#endif
