
#include "System/pch.h"
#include "System/CherrySprout.h"

#include "System/Config.h"
#include "System/Win32App.h"
#include "Utils/Helper.h"

void InitializeDebugOutput()
{
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    CherryPrint("Console Window Initialised");
}

int CherrySprout::Run(App& app, const HINSTANCE hInstance, const LPSTR args, const int nCmdShow)
{
    V(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

#if !NDEBUG
    InitializeDebugOutput();
#endif

    Config::ParseCommandLineArgs(args);

    const int result = Win32App::Run({&app}, hInstance, nCmdShow);

    CoUninitialize();

    return result;
}
