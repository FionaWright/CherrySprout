#ifndef H_GREENHOUSE_H
#define H_GREENHOUSE_H
#include "System/App.h"

class Greenhouse final : public App
{
public:
    void OnInit(D3D* d3d) override;
    void OnUpdate(D3D* d3d, ID3D12GraphicsCommandList* cmdList, double deltaTime) override;
    void OnPostUpdate(D3D* d3d) override;
    void RenderGUI() override;
    [[nodiscard]] const char* GetName() const override;
};

#endif