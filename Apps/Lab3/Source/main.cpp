#include "System/pch.h"
#include "System/CherrySprout.h"

#include "Apps/Lab3/Headers/Lab3.h"

_Use_decl_annotations_

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nShowCmd)
{
    Lab3 app;
    return CherrySprout::Run(app, hInstance, lpCmdLine, nShowCmd);
}