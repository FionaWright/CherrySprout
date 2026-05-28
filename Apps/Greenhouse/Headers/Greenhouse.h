#ifndef H_GREENHOUSE_H
#define H_GREENHOUSE_H
#include "System/App.h"

class Greenhouse final : public App
{
public:
    [[nodiscard]] const char* GetName() const override { return "Greenhouse"; };

    void Init(D3D* d3d) override;
    void Update(D3D* d3d, TimeArgs timeArgs) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList) override;
    void PostUpdate(D3D* d3d) override;
    void RenderGUI() override;
};

#endif