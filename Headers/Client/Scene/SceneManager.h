//
// Created by fionaw on 31/05/2026.
//

#ifndef CHERRYSPROUT_SCENEMANAGER_H
#define CHERRYSPROUT_SCENEMANAGER_H

#include "Scene.h"
#include "HWI/UploadHeap.h"

class Heap;
class D3D;

class SceneManager
{
public:
    void LoadScene(const char* filepath, float sceneScale);
    void UploadScene(D3D* d3d);
    void AddSceneTexturesToHeap(const D3D* d3d, Heap* heap) const;

    bool IsGpuDataDirty() const { return m_gpuDataDirty; }
    void UnreserveData() { m_uploadHeap.FreeAssignedData(); }

    Scene& GetScene() { return m_scene; }
    SceneCPU& GetCPU() { return m_scene.CPU; }
    SceneGPU& GetGPU() { return m_scene.GPU; }

private:
    Scene m_scene;
    bool m_gpuDataDirty = false;

    UploadHeap m_uploadHeap;
};


#endif //CHERRYSPROUT_SCENEMANAGER_H