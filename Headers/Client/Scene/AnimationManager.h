//
// Created by fionaw on 18/11/2026.
//

#ifndef CHERRYSPROUT_ANIMATIONMANAGER_H
#define CHERRYSPROUT_ANIMATIONMANAGER_H

#include "HWI/D12Resource.h"
#include "HWI/D3D.h"

class AnimationManager
{
public:
    void Init(D3D* d3d, );

private:
    D12Resource m_vertexSkinnedMegaBuffer;
};


#endif //CHERRYSPROUT_ANIMATIONMANAGER_H