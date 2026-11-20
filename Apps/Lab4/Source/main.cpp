#include "System/pch.h"
#include "System/CherrySprout.h"

#include "Apps/Lab4/Headers/Lab4.h"

_Use_decl_annotations_

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nShowCmd)
{
    Lab4 app;
    return CherrySprout::Run(app, hInstance, lpCmdLine, nShowCmd);
}