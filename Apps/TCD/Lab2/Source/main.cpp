#include "System/pch.h"
#include "System/CherrySprout.h"

#include "Apps/TCD/Lab2/Headers/Lab2.h"

_Use_decl_annotations_

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nShowCmd)
{
    Lab2 app;
    return CherrySprout::Run(app, hInstance, lpCmdLine, nShowCmd);
}
