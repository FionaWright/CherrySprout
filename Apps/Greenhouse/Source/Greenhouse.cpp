
#include "System/pch.h"

#include "Greenhouse.h"

#include "HWI/D3D.h"
#include "Scene/SceneManager.h"
#include "System/HighResolutionClock.h"
#include "Utils/Helper.h"

void Greenhouse::Init(D3D* d3d)
{
    App::Init(d3d);

    m_sceneManager.LoadScene(R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\assets\full_assets\OpenChessSet\chess_set.usda)");


}

void Greenhouse::Update(D3D* d3d, const TimeArgs timeArgs)
{
}

void Greenhouse::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    if (m_sceneManager.IsGpuDataDirty())
    {
        m_sceneManager.UploadScene(d3d, cmdList);
    }
}

void Greenhouse::PostUpdate(D3D* d3d)
{
}

void Greenhouse::RenderGUI()
{
}
