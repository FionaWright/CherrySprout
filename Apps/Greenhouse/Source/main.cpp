#include "System/pch.h"
#include "System/CherrySprout.h"

#include "Greenhouse.h"

_Use_decl_annotations_

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR args, int nCmdShow)
{
    Greenhouse greenhouseApp;
    return CherrySprout::Run(greenhouseApp, hInstance, args, nCmdShow);
}
