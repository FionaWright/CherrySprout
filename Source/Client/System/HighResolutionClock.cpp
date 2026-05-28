//
// Created by fionaw on 26/10/2025.
//

#include "System/pch.h"
#include "System/HighResolutionClock.h"

HighResolutionClock::HighResolutionClock()
    : m_deltaTime(0)
    , m_totalTime(0)
{
    m_t0 = high_resolution_clock::now();
}

void HighResolutionClock::Tick()
{
    const auto t1 = high_resolution_clock::now();
    m_deltaTime = t1 - m_t0;
    m_totalTime += m_deltaTime;
    m_t0 = t1;
}

void HighResolutionClock::Reset()
{
    m_t0 = high_resolution_clock::now();
    m_deltaTime = high_resolution_clock::duration();
    m_totalTime = high_resolution_clock::duration();
}

double HighResolutionClock::GetDeltaNanoseconds() const
{
    return static_cast<double>(m_deltaTime.count()) * 1.0;
}
double HighResolutionClock::GetDeltaMicroseconds() const
{
    return static_cast<double>(m_deltaTime.count()) * 1e-3;
}

double HighResolutionClock::GetDeltaMilliseconds() const
{
    return static_cast<double>(m_deltaTime.count()) * 1e-6;
}

double HighResolutionClock::GetDeltaSeconds() const
{
    return static_cast<double>(m_deltaTime.count()) * 1e-9;
}

double HighResolutionClock::GetTotalNanoseconds() const
{
    return static_cast<double>(m_deltaTime.count()) * 1.0;
}

double HighResolutionClock::GetTotalMicroseconds() const
{
    return static_cast<double>(m_deltaTime.count()) * 1e-3;
}

double HighResolutionClock::GetTotalMilliSeconds() const
{
    return static_cast<double>(m_deltaTime.count()) * 1e-6;
}

double HighResolutionClock::GetTotalSeconds() const
{
    return static_cast<double>(m_deltaTime.count()) * 1e-9;
}

TimeArgs HighResolutionClock::GetTimeArgs() const
{
    TimeArgs args{};
    args.ElapsedTime_ms = GetDeltaMilliseconds();
    args.TotalTime_s = GetTotalSeconds();
    return args;
}
