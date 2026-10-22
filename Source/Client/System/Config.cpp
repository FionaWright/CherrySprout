//
// Created by fionaw on 28/09/2025.
//

#include "System/pch.h"
#include "System/Config.h"

SettingsSystem Config::ms_settingsSystem;
SettingsRender Config::ms_settingsRender;
std::unordered_map<std::string, std::string> Config::ms_argsMap;

template <typename Out>
void split(const std::string &s, const char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, const char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

inline void Config::SetBoolFromArg(bool* variable, const char* key)
{
    if (ms_argsMap.contains(key))
        *variable = ms_argsMap.at(key) != "false";
}

inline void Config::SetUIntFromArg(uint32_t* variable, const char* key)
{
    if (ms_argsMap.contains(key))
        *variable = std::stoi(ms_argsMap.at(key));
}

void Config::ParseCommandLineArgs(const char* args)
{
    const std::vector<std::string> splitArgs = split(args, ' ');

    for (const auto& arg : splitArgs)
    {
        const auto eqIdx = arg.find('=');
        if (eqIdx == std::string::npos)
        {
            ms_argsMap.insert({arg, ""});
            continue;
        }

        const std::string key = arg.substr(0, eqIdx);
        const std::string value = arg.substr(eqIdx + 1);
        ms_argsMap.insert({key, value});
    }

    SetUIntFromArg(&ms_settingsSystem.RtvWidth, "--window_width");
    SetUIntFromArg(&ms_settingsSystem.RtvHeight, "--window_height");
    SetUIntFromArg(&ms_settingsRender.MsaaSampleCount, "--msaa");
    SetUIntFromArg(reinterpret_cast<uint32_t*>(&ms_settingsRender.DepthBufferType), "--depthType");
    SetBoolFromArg(&ms_settingsSystem.VSyncEnabled, "--vsync");
    SetBoolFromArg(&ms_settingsSystem.DebugHeapEnabled, "--debugHeap");
}

XMMATRIX Config::ComputeProjectionMatrix()
{
    const float aspectRatio = static_cast<float>(ms_settingsSystem.RtvWidth) / static_cast<float>(ms_settingsSystem.RtvHeight);
    const float nearZ = ms_settingsRender.DepthBufferType == DepthBufferType::eForward ? ms_settingsRender.NearPlane : ms_settingsRender.FarPlane;
    const float farZ = ms_settingsRender.DepthBufferType == DepthBufferType::eForward ? ms_settingsRender.FarPlane : ms_settingsRender.NearPlane;
    return XMMatrixPerspectiveFovLH(XMConvertToRadians(ms_settingsRender.FoV), aspectRatio, nearZ, farZ);
}
