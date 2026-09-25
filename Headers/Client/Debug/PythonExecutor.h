//
// Created by fionaw on 06/11/2025.
//

#ifndef H_PYTHON_EXECUTOR_H
#define H_PYTHON_EXECUTOR_H

#include <vector>

class PythonExecutor
{
public:
    static void ExecutePython(const char* pythonFile, const std::vector<std::string>& args);
    static void ExecutePythonWithData(const char* pythonFile, const char* dataFileName, const char* data, size_t size,
                                      const std::vector<std::string>& args);
};


#endif // H_PYTHON_EXECUTOR_H