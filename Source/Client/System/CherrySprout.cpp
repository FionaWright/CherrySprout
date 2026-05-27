
#include "System/pch.h"
#include "System/CherrySprout.h"

#include "System/Config.h"
#include "System/Win32App.h"
#include "Utils/Helper.h"

int CherrySprout::Run(App& app, HINSTANCE hInstance, const LPSTR args, const int nCmdShow)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return -1;

#ifdef _DEBUG
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    CherryPrint("Console Window Initialised");
#endif

    Config::ParseCommandLineArgs(args);

    const int result = Win32App::Run({&app}, hInstance, nCmdShow);

    CoUninitialize();

    return result;
}
