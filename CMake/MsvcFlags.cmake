# Use the Release CRT for all configurations
if(MSVC)
    add_compile_options(
            /permissive-
            /Zc:preprocessor
            /Zc:__cplusplus
    )

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()