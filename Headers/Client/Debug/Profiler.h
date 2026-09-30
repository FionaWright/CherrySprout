//
// Created by fionaw on 13/11/2025.
//

#ifndef H_PROFILER_H
#define H_PROFILER_H

#if NDEBUG
#error
#endif

#include <chrono>
#include <stack>
#include <string>

struct TrackedTask
{
    std::string Name;
    std::chrono::high_resolution_clock::time_point StartTime;
};

class Profiler
{
public:
    static void AddToStack(const std::wstring& name);
    static void AddToStack(const char* name);
    static void PopAndPrint();

private:
    static std::stack<TrackedTask> m_stack;
};


#endif //CHERRYPIP_PROFILER_H