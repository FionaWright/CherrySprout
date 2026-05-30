//
// Created by fionaw on 29/05/2026.
//

#ifndef CHERRYSPROUT_SCENE_H
#define CHERRYSPROUT_SCENE_H

#include "Scene/SceneCPU.h"
#include "Scene/SceneGPU.h"

struct Scene
{
    SceneCPU CPU;
    SceneGPU GPU;
};

#endif //CHERRYSPROUT_SCENE_H
