//
// Created by fionaw on 26/10/2025.
//

#include "System/pch.h"
#include "System/Engine.h"
#include "Utils/Helper.h"
#include "imgui.h"
#include "System/App.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "System/Config.h"
#include "System/FileHelper.h"
#include "System/Gui.h"
#include "System/Input.h"

#ifdef _DEBUG
#include "Debug/HotReloader.h"
#endif

Engine::Engine(App* app, const HWND hWnd, const UINT windowWidth, const UINT windowHeight)
{
    m_d3d = std::make_unique<D3D>();
    m_d3d->Init(windowWidth, windowHeight);

    m_app = app;
    if (!m_app->GetIsInitialized())
    {
        m_d3d->Flush();
        CherryPrint("Initializing App: " << m_app->GetName() << "...");
        m_app->Init(m_d3d.get());
        CherryPrint("Initialized App: " << m_app->GetName());
    }

    Gui::Init(hWnd, m_d3d->GetDevice(), 3);
}

void Engine::Frame()
{
    // Update
    {
        const TimeArgs timeArgs = m_clock.GetTimeArgs();
        CalculateFPS(timeArgs.ElapsedTime_ms / 1000);

        m_app->Update(m_d3d.get(), timeArgs);

        m_clock.Tick();
    }

    Render();

    Input::ProgressFrame();

#ifdef _DEBUG
    if (m_hotReloaderPendingGraphics)
        HotReloader::ReloadPipelines(m_d3d.get(), false, ReloadMode::eGraphics);
    else if (m_hotReloaderPendingCompute)
        HotReloader::ReloadPipelines(m_d3d.get(), false, ReloadMode::eCompute);
    else
        HotReloader::ReloadPipelines(m_d3d.get(), !m_hotReloaderPendingAll);

    m_hotReloaderPendingAll = m_hotReloaderPendingGraphics = m_hotReloaderPendingCompute = false;
#endif
}

void Engine::Render()
{
    const ComPtr<ID3D12GraphicsCommandList> cmdList = m_d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    D12Resource* rtv = m_d3d->GetRtv();

    {
        GPU_SCOPE(cmdList.Get(), L"Setup");

        rtv->Transition(cmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    {
        GPU_SCOPE(cmdList.Get(), L"App Render");

        m_app->Render(m_d3d.get(), cmdList.Get());
    }

    {
        GPU_SCOPE(cmdList.Get(), L"GUI");

        Gui::BeginFrame();

        RenderGUI();

        m_app->RenderGUI();

        const D3D12_CPU_DESCRIPTOR_HANDLE handle = m_d3d->GetRtvHandle();
        cmdList->OMSetRenderTargets(1, &handle, FALSE, nullptr);
        rtv->Transition(cmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        Gui::RenderAllWindows(cmdList.Get());
    }

    // Present
    {
        rtv->Transition(cmdList.Get(), D3D12_RESOURCE_STATE_PRESENT);

        V(cmdList->Close());
        m_d3d->ExecuteCommandList(cmdList.Get());
        m_d3d->Present();
    }

    // PostUpdate
    {
        m_app->PostUpdate(m_d3d.get());
    }
}

void Engine::CalculateFPS(const double deltaTime_s)
{
    m_frameTime = deltaTime_s;

    m_fpsTimeSinceUpdate10ms += deltaTime_s;
    m_fpsTimeSinceUpdate50ms += deltaTime_s;
    m_fpsTimeSinceUpdate100ms += deltaTime_s;
    m_fpsFramesSinceUpdate10ms++;
    m_fpsFramesSinceUpdate50ms++;
    m_fpsFramesSinceUpdate100ms++;

    if (m_fpsTimeSinceUpdate10ms > 0.1)
    {
        m_fps10ms = m_fpsFramesSinceUpdate10ms / m_fpsTimeSinceUpdate10ms;

        m_fpsFramesSinceUpdate10ms = 0;
        m_fpsTimeSinceUpdate10ms = 0.0;
    }

    if (m_fpsTimeSinceUpdate50ms > 0.5)
    {
        m_fps50ms = m_fpsFramesSinceUpdate50ms / m_fpsTimeSinceUpdate50ms;

        m_fpsFramesSinceUpdate50ms = 0;
        m_fpsTimeSinceUpdate50ms = 0.0;
    }

    if (m_fpsTimeSinceUpdate100ms > 1.0)
    {
        m_fps100ms = m_fpsFramesSinceUpdate100ms / m_fpsTimeSinceUpdate100ms;

        m_fpsFramesSinceUpdate100ms = 0;
        m_fpsTimeSinceUpdate100ms = 0.0;
    }
}