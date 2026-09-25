//
// Created by fionaw on 06/11/2025.
//

#include "System/pch.h"
#include "Debug/PythonExecutor.h"

#include "Utils/Helper.h"

void PythonExecutor::ExecutePython(const char* pythonFile, const std::vector<std::string>& args)
{
    std::string command = "python ";

    command += "\"" + std::string(SOURCE_DIR) + "/Scripts/Python/" + std::string(pythonFile) + "\"";

    for (int i = 0; i < args.size(); i++)
        command += " " + std::string(args[i]);

    CherryPrint("\n" << command << std::endl);

    system(command.c_str());
}

void PythonExecutor::ExecutePythonWithData(const char* pythonFile, const char* dataFileName, const char* data, const size_t size, const std::vector<std::string>& args)
{
    const std::string dataFilePath = std::string(BUILD_DIR) + "/Data/Temp/" + std::string(dataFileName) + ".bin";

    std::filesystem::create_directories(std::filesystem::path(dataFilePath).parent_path());

    std::ofstream fs(dataFilePath, std::ios::out | std::ios::binary);
    fs.write(data, size);
    fs.close();

    std::vector<std::string> newArgs = {};
    newArgs.emplace_back("\"" + dataFilePath + "\"");

    for (int i = 0; i < args.size(); i++)
        newArgs.emplace_back(args[i]);

    ExecutePython(pythonFile, newArgs);
}