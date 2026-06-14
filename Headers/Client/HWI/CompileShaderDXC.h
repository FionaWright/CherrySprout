#ifndef H_COMPILE_SHADER_DXC_H
#define H_COMPILE_SHADER_DXC_H

inline std::vector<uint8_t> readFileToByteVector(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Failed to open file");
    const auto size = static_cast<size_t>(file.tellg());
    std::vector<uint8_t> data(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

enum ShaderCompileFlags
{
    SCF_Debug            = 1 << 0,
    SCF_DisableOptimize  = 1 << 1,
    SCF_WarningsAsErrors = 1 << 2,
};

inline ComPtr<IDxcBlob> CompileShaderDXC(
    const std::string& filePath,
    const char* entryPoint,
    const char* targetProfile,
    ShaderCompileFlags compileFlags,
    const std::vector<std::string>& extraArgs)
{
    HMODULE dxCompilerDLL = LoadLibrary("dxcompiler.dll");
    if (!dxCompilerDLL)
    {
        CherryPrint("LoadLibrary failed: " << GetLastError());
    }

    // Get DxcCreateInstance function
    auto DxcCreateInstanceFn = reinterpret_cast<HRESULT(__stdcall*)(REFCLSID, REFIID, LPVOID*)>(
        GetProcAddress(dxCompilerDLL, "DxcCreateInstance"));
    if (!DxcCreateInstanceFn)
    {
        std::cerr << "Failed to get DxcCreateInstance\n";
    }

    // Create DXC objects
    ComPtr<IDxcCompiler3> compiler;
    ComPtr<IDxcLibrary> library;
    ComPtr<IDxcIncludeHandler> includeHandler;
    ComPtr<IDxcUtils> utils;
    try
    {
        V(DxcCreateInstanceFn(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
    }
    catch (const std::exception& e)
    {
        CherryPrint(e.what());
    }
    catch (...)
    {
        CherryPrint("Unknown exception in DxcCreateInstance");
    }

    V(DxcCreateInstanceFn(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
    V(DxcCreateInstanceFn(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
    V(library->CreateIncludeHandler(&includeHandler));

    const auto shaderBytes = readFileToByteVector(filePath);

    DxcBuffer buffer;
    buffer.Ptr = shaderBytes.data();
    buffer.Size = shaderBytes.size();
    buffer.Encoding = DXC_CP_UTF8; // or DXC_CP_ACP if ASCII

    const std::string dataPath = FileHelper::GetAssetsPath() + "Data/";
    const std::string shadersPath = FileHelper::GetShadersPath();

    std::wstring entryPointW = stringToWString(entryPoint);
    std::wstring targetProfileW = stringToWString(targetProfile);
    std::wstring dataPathW = stringToWString(dataPath);
    std::wstring shadersPathW = stringToWString(shadersPath);

    ComPtr<IDxcResult> result;
    std::vector<const wchar_t*> args = {
        L"-E", entryPointW.c_str(),
        L"-T", targetProfileW.c_str(),
        L"-I", dataPathW.c_str(),
        L"-I", shadersPathW.c_str()
    };

    std::vector<std::wstring> argStorage; // Prevent dangling pointers
    for (const auto& arg : extraArgs)
    {
        argStorage.push_back(stringToWString(arg));
        args.push_back(argStorage.back().c_str());
    }

    if (compileFlags & SCF_Debug)
        args.push_back(L"-Zi");

    if (compileFlags & SCF_DisableOptimize)
        args.push_back(L"-Od");
    else
        args.push_back(L"-O3");

    if (compileFlags & SCF_WarningsAsErrors)
        args.push_back(L"-WX");

#ifdef CHERRY_PRINT_ENABLED
    std::cout << "Compiling Shader: ";
    for (const auto& arg : args)
        std::cout << wstringToString(arg) << " ";
    std::cout << std::endl;
#endif

    if (FAILED(compiler->Compile(&buffer, args.data(), args.size(), includeHandler.Get(), IID_PPV_ARGS(&result))))
    {
        CherryPrint("Shader compile failed");
        return nullptr;
    }

    // Get compiled blob
    ComPtr<IDxcBlobUtf8> errors;
    if (FAILED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr)))
    {
        CherryPrint("Failed to get shader errors");
        return nullptr;
    }
    if (errors && errors->GetStringLength() > 0)
    {
        CherryPrint("Shader compile warnings/errors:\n" << errors->GetStringPointer());
    }

    ComPtr<IDxcBlob> vertexShaderBlob;
    if (FAILED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&vertexShaderBlob), nullptr)))
    {
        CherryPrint("Failed to get compiled shader");
        return nullptr;
    }

    return vertexShaderBlob;
}

#endif