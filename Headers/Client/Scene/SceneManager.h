//
// Created by fionaw on 31/05/2026.
//

#ifndef CHERRYSPROUT_SCENEMANAGER_H
#define CHERRYSPROUT_SCENEMANAGER_H

#include "Scene.h"

class SceneManager
{
public:
    void LoadScene(const char* filepath);

    SceneCPU& GetCPU() { return m_scene.CPU; }
    SceneGPU& GetGPU() { return m_scene.GPU; }

private:
    Scene m_scene;
};


#endif //CHERRYSPROUT_SCENEMANAGER_H