//
// Created by fiona on 25/09/2025.
//

#ifndef PT_APP_H
#define PT_APP_H

struct TimeArgs;
class D3D;

class App
{
public:
    [[nodiscard]] virtual const char* GetName() const = 0;

    virtual void Init(D3D* d3d) { m_initialized = true; }
    virtual void Update(D3D* d3d, TimeArgs timeArgs) = 0;
    virtual void PostUpdate(D3D* d3d) = 0;
    virtual void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList) = 0;
    virtual void RenderGUI() = 0;

    [[nodiscard]] bool GetIsInitialized() const { return m_initialized; }

protected:
    ~App() = default;

private:
    bool m_initialized = false;
};

#endif //PT_APP_H