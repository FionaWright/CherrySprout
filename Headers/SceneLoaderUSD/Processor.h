//
// Created by fionaw on 30/05/2026.
//

#ifndef CHERRYSPROUT_POSTPROCESSOR_H
#define CHERRYSPROUT_POSTPROCESSOR_H

#include "Importer.h"
#include "Scene/SceneCPU.h"

namespace SceneLoaderUSD
{
    void Process(const ImporterContext& importerContext, SceneCPU* scene);
}

#endif //CHERRYSPROUT_POSTPROCESSOR_H