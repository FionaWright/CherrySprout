#include "System/pch.h"
#include "PathTracer.h"

#include "imgui.h"
#include "GreenhouseConfig.h"

void PathTracer::GUI(GreenhouseConfig* config)
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    bool ptFrameDirty = false;
    m_debugManager.GUI(config->PathTracerConfig, ptFrameDirty);

    if (ptFrameDirty)
        Reset();
#endif
}