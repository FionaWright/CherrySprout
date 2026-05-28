#include "System/pch.h"
#include "System/CherrySprout.h"

#include "Greenhouse.h"

_Use_decl_annotations_

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nShowCmd)
{
    Greenhouse greenhouseApp;
    return CherrySprout::Run(greenhouseApp, hInstance, lpCmdLine, nShowCmd);
}
