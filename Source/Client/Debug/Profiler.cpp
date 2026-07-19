//
// Created by fionaw on 13/11/2025.
//

#include "System/pch.h"
#include "Debug/Profiler.h"

#include "System/Config.h"
#include "Utils/Helper.h"

std::stack<TrackedTask> Profiler::m_stack;

void Profiler::AddToStack(const std::wstring& name)
{
    if (!Config::GetSystem().ProfilingEnabled)
        return;

    const std::string sName = wstringToString(name);

    const TrackedTask task = { sName, std::chrono::high_resolution_clock::now() };
    m_stack.push(task);
}

void Profiler::AddToStack(const char* name)
{
    if (!Config::GetSystem().ProfilingEnabled)
        return;

    const TrackedTask task = { name, std::chrono::high_resolution_clock::now() };
    m_stack.push(task);
}

void Profiler::PopAndPrint()
{
    if (!Config::GetSystem().ProfilingEnabled)
        return;

    const TrackedTask task = m_stack.top();
    m_stack.pop();

    const auto finish = std::chrono::high_resolution_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(finish - task.StartTime).count();
    CherryPrint(task.Name << " took " << millis << " ms");
}