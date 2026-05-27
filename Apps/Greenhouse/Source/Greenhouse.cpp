
#include "System/pch.h"

#include "Greenhouse.h"

void Greenhouse::OnInit(D3D* d3d)
{
    App::OnInit(d3d);
}

void Greenhouse::OnUpdate(D3D* d3d, ID3D12GraphicsCommandList* cmdList, double deltaTime)
{
}

void Greenhouse::OnPostUpdate(D3D* d3d)
{
}

void Greenhouse::RenderGUI()
{
}

const char* Greenhouse::GetName() const
{
    return "Greenhouse";
}
