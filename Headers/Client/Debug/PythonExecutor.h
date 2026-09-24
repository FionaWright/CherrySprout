//
// Created by fionaw on 06/11/2025.
//

#ifndef H_PYTHON_EXECUTOR_H
#define H_PYTHON_EXECUTOR_H

#include <vector>

class PythonExecutor
{
public:
    static void ExecutePython(const char* pythonFile, const std::vector<const char*>& args);
    static void ExecutePythonWithData(const char* pythonFile, const char* data, size_t size,
                                      const std::vector<const char*>& args);
};


#endif // H_PYTHON_EXECUTOR_H