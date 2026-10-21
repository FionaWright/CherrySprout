#include "System/pch.h"
#include "PathTracer.h"

#include "GreenhouseConfig.h"

void PathTracer::GUI(GreenhouseConfig* config, bool& frameDirty)
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    m_debugManager.GUI(config->PathTracerConfig, frameDirty);
#endif
}