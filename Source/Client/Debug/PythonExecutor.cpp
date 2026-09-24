//
// Created by fionaw on 06/11/2025.
//

#include "System/pch.h"
#include "Debug/PythonExecutor.h"

void PythonExecutor::ExecutePython(const char* pythonFile, const std::vector<const char*>& args)
{
    std::string command = "python ";

    command += "\"" + std::string(SOURCE_DIR) + "/Python Scripts/" + std::string(pythonFile) + "\"";

    for (int i = 0; i < args.size(); i++)
        command += " " + std::string(args[i]);

    system(command.c_str());
}

void PythonExecutor::ExecutePythonWithData(const char* pythonFile, const char* data, size_t size, const std::vector<const char*>& args)
{
    const std::string dataFile = std::string(BUILD_DIR) + "/Data/Temp/" + std::string(pythonFile) + ".bin";

    std::ofstream fs(dataFile, std::ios::out | std::ios::binary | std::ios::app);
    fs.write(data, size);
    fs.close();

    std::vector<const char*> newArgs = {};
    newArgs.emplace_back(dataFile.c_str());

    for (int i = 0; i < args.size(); i++)
        newArgs.emplace_back(args[i]);

    ExecutePython(pythonFile, args);
}