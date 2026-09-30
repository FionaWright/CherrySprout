cmake_minimum_required(VERSION 3.14)
project(CherrySprout)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if (ENABLE_ASAN AND MSVC)
    add_compile_options(/fsanitize=address)
    add_link_options(/fsanitize=address)
endif()

if(MSVC)
    include(cmake/MsvcFlags.cmake)
endif()

include(cmake/Dependencies.cmake)
include(cmake/Sources.cmake)
include(cmake/Options.cmake)

add_library(client STATIC
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
)
target_precompile_headers(client PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/Headers/client/System/pch.h>"
)
target_link_libraries(client
        PUBLIC
        ProjectOptions
        DX12Agility
        d3d12
        dxgi
        dxguid
        imgui
        DirectXTex::DirectXTex
        WinPixEventRuntime
)
target_include_directories(client PUBLIC
        "${CMAKE_SOURCE_DIR}/Headers"
        "${CMAKE_SOURCE_DIR}/Headers/Client"
        "${CMAKE_SOURCE_DIR}/Assets/Shaders"
        "${CMAKE_SOURCE_DIR}/Assets/Resources"

        "${dxc_SOURCE_DIR}/inc"
)
target_compile_definitions(client PUBLIC NOMINMAX)
set_target_properties(client PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

include(cmake/Assets.cmake)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES MATCHES "Debug")
    target_compile_definitions(client PUBLIC SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()
target_compile_definitions(client PUBLIC BUILD_DIR="${CMAKE_BINARY_DIR}")

set(USD_BIN_DIR ${CMAKE_BINARY_DIR}/SceneLoaderUSD/bin)
file(MAKE_DIRECTORY "${USD_BIN_DIR}")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Snapshots")

include(CMake/SceneLoaderUSD.cmake)

add_subdirectory(Apps)