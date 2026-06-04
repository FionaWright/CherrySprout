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

    XMFLOAT3 CameraPosition{0.0f, 5.0f, 5.0f};
    XMFLOAT2 CameraPitchYaw{0.0f, PI};
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
