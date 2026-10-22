//
// Created by fionaw on 28/09/2025.
//

#ifndef PT_CONFIG_H
#define PT_CONFIG_H

#define NUM_FRAMES_IN_FLIGHT 3

struct SettingsSystem
{
    uint32_t RtvWidth = 1020;
    uint32_t RtvHeight = 575;

    uint32_t WindowAppGuiWidth = 425;
    uint32_t WindowEngineGuiWidth = 250;

    bool VSyncEnabled = false;
    bool ForceSyncCpuGpu = false;
    bool AppGuiEnabled = true;

    bool ProfilingEnabled = true;
    bool DebugHeapEnabled = false;
};

enum class DepthBufferType : uint32_t
{
    eForward,
    eReversed,
    eReversedInfiniteFar
};

struct SettingsRender
{
    DXGI_FORMAT RtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT DsvFormat = DXGI_FORMAT_D32_FLOAT; // No stencil
    float RtvClearColor[4] = {0.3f, 0.3f, 0.5f, 1.0f};

    uint32_t MsaaSampleCount = 1;

    float FoV = 60.0f;
    float NearPlane = 0.01f;
    float FarPlane = 500.0f;
    DepthBufferType DepthBufferType = DepthBufferType::eReversed;
};

class Config
{
public:
    static void SetBoolFromArg(bool* variable, const char* key);
    static void SetUIntFromArg(uint32_t* variable, const char* key);

    static void ParseCommandLineArgs(const char* args);
    static SettingsSystem& GetSystem() { return ms_settingsSystem; }
    static SettingsRender& GetRender() { return ms_settingsRender; }

    static XMMATRIX ComputeProjectionMatrix();

private:
    static SettingsSystem ms_settingsSystem;
    static SettingsRender ms_settingsRender;
    static std::unordered_map<std::string, std::string> ms_argsMap;
};


#endif //PT_CONFIG_H