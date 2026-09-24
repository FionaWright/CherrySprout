#include "System/pch.h"
#include "PathTracer.h"

#include "imgui.h"
#include "GreenhouseConfig.h"

void PathTracer::GUI(GreenhouseConfig* config)
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    m_debugManager.GUI(config->PathTracerConfig);
#endif
}