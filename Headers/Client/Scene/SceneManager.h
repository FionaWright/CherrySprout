//
// Created by fionaw on 31/05/2026.
//

#ifndef CHERRYSPROUT_SCENEMANAGER_H
#define CHERRYSPROUT_SCENEMANAGER_H

#include "Scene.h"
#include "HWI/UploadHeap.h"

class D3D;

class SceneManager
{
public:
    void LoadScene(const char* filepath);
    void UploadScene(const D3D* d3d, ID3D12GraphicsCommandList* cmdList);

    bool IsGpuDataDirty() const { return m_gpuDataDirty; }

    Scene& GetScene() { return m_scene; }
    SceneCPU& GetCPU() { return m_scene.CPU; }
    SceneGPU& GetGPU() { return m_scene.GPU; }

private:
    Scene m_scene;
    bool m_gpuDataDirty = false;

    UploadHeap m_uploadHeap;
};


#endif //CHERRYSPROUT_SCENEMANAGER_H