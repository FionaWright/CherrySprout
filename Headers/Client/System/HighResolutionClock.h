//
// Created by fionaw on 26/10/2025.
//

#ifndef PT_HIGHRESOLUTIONCLOCK_H
#define PT_HIGHRESOLUTIONCLOCK_H

using std::chrono::high_resolution_clock;

struct TimeArgs
{
    double ElapsedTime_ms;
    double TotalTime_s;
};

class HighResolutionClock
{
public:
    HighResolutionClock();

    void Tick();

    void Reset();

    [[nodiscard]] double GetDeltaNanoseconds() const;
    [[nodiscard]] double GetDeltaMicroseconds() const;
    [[nodiscard]] double GetDeltaMilliseconds() const;
    [[nodiscard]] double GetDeltaSeconds() const;

    [[nodiscard]] double GetTotalNanoseconds() const;
    [[nodiscard]] double GetTotalMicroseconds() const;
    [[nodiscard]] double GetTotalMilliSeconds() const;
    [[nodiscard]] double GetTotalSeconds() const;

    [[nodiscard]] TimeArgs GetTimeArgs() const;

private:
    high_resolution_clock::time_point m_t0;

    high_resolution_clock::duration m_deltaTime;
    high_resolution_clock::duration m_totalTime;
};

#endif //PT_HIGHRESOLUTIONCLOCK_H