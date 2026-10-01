//
// Created by fionaw on 31/05/2026.
//

#ifndef CHERRYSPROUT_SCENEMANAGER_H
#define CHERRYSPROUT_SCENEMANAGER_H

#include "Scene.h"
#include "TextureConverter.h"
#include "HWI/UploadHeap.h"
#include "Utils/D3DUtils.h"

class Heap;
class D3D;

class SceneManager
{
public:
    void LoadScene(const char* filepath, float sceneScale);
    void UploadScene(D3D* d3d);
    void AddSceneTexturesToHeap(const D3D* d3d, Heap* heap) const;

    [[nodiscard]] bool IsGpuDataDirty() const { return m_gpuDataDirty; }

    Scene& GetScene() { return m_scene; }
    SceneCPU& GetCPU() { return m_scene.CPU; }
    SceneGPU& GetGPU() { return m_scene.GPU; }

private:
    Scene m_scene;
    TextureConverter m_converter;

    bool m_gpuDataDirty = false;
};


#endif //CHERRYSPROUT_SCENEMANAGER_H