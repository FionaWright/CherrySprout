cmake_minimum_required(VERSION 4.0)
project(CherrySprout)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    include(CMake/MsvcFlags.cmake)
endif()

set(IS_DEBUG FALSE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES MATCHES "Debug")
    set(IS_DEBUG TRUE)
endif()

set(USD_DEBUG 0)
if(IS_DEBUG)
    set(USD_DEBUG 1)
endif()

# -------------- DOWNLOAD + BUILD USD ----------------

include(ExternalProject)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(OPEN_USD_DIR ${CMAKE_BINARY_DIR}/SceneLoaderUSD/OpenUSD)
set(OPEN_USD_DIR_BUILD ${OPEN_USD_DIR}/Build)

if (USD_DEBUG)
    set(BUILD_VARIANT debug)
    set(TBB_DEBUG_BYPRODUCT "${OPEN_USD_DIR_BUILD}/lib/tbb_debug.lib")
else()
    set(BUILD_VARIANT release)
    set(TBB_DEBUG_BYPRODUCT )
endif()

if (NOT EXISTS "${OPEN_USD_DIR_BUILD}/lib/usd_ms.lib")
    ExternalProject_Add(USD_EP
            GIT_REPOSITORY https://github.com/PixarAnimationStudios/OpenUSD.git
            GIT_TAG v25.05
            GIT_SHALLOW TRUE

            SOURCE_DIR "${OPEN_USD_DIR}/Source"
            BINARY_DIR "${OPEN_USD_DIR_BUILD}"
            PREFIX     "${OPEN_USD_DIR}/USD_EP-Prefix"

            CONFIGURE_COMMAND ""
            BUILD_COMMAND
                ${Python3_EXECUTABLE}
                -u
                <SOURCE_DIR>/build_scripts/build_usd.py
                -vvv
                --build-variant ${BUILD_VARIANT}
                --build-monolithic
                --no-examples
                --no-tutorials
                --no-tools
                --no-tests
                --no-python
                --no-imaging
                --no-usdview
                --
                "${OPEN_USD_DIR_BUILD}"

            INSTALL_COMMAND ""

            BUILD_BYPRODUCTS
                "${OPEN_USD_DIR_BUILD}/lib/usd_ms.lib"
                "${OPEN_USD_DIR_BUILD}/lib/tbb.lib"
                "${OPEN_USD_DIR_BUILD}/lib/tbbmalloc.lib"
                ${TBB_DEBUG_BYPRODUCT}
    )
endif()

# Required
file(MAKE_DIRECTORY "${OPEN_USD_DIR_BUILD}/include")

# -------------- IMPORTED LIBRARIES -------------------

add_library(usd_m SHARED IMPORTED GLOBAL)
set_target_properties(usd_m PROPERTIES
        IMPORTED_IMPLIB "${OPEN_USD_DIR_BUILD}/lib/usd_ms.lib"
        IMPORTED_LOCATION "${OPEN_USD_DIR_BUILD}/bin/usd_ms.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${OPEN_USD_DIR_BUILD}/include"
)

add_dependencies(usd_m USD_EP)

add_library(tbb SHARED IMPORTED GLOBAL)
set_target_properties(tbb PROPERTIES
        IMPORTED_IMPLIB "${OPEN_USD_DIR_BUILD}/lib/tbb.lib"
        IMPORTED_LOCATION "${OPEN_USD_DIR_BUILD}/bin/tbb.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${OPEN_USD_DIR_BUILD}/include"
)

add_library(tbb_malloc SHARED IMPORTED GLOBAL)
set_target_properties(tbb_malloc PROPERTIES
        IMPORTED_IMPLIB "${OPEN_USD_DIR_BUILD}/lib/tbbmalloc.lib"
        IMPORTED_LOCATION "${OPEN_USD_DIR_BUILD}/bin/tbbmalloc.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${OPEN_USD_DIR_BUILD}/include"
)

# -------------- SCENE LOADER TARGET ------------------

add_library(SceneLoaderUSD SHARED
        "${CMAKE_SOURCE_DIR}/Source/SceneLoaderUSD/SceneLoaderUSD.cpp"
        "${CMAKE_SOURCE_DIR}/Source/SceneLoaderUSD/Importer.cpp"
        "${CMAKE_SOURCE_DIR}/Source/SceneLoaderUSD/Processor.cpp"
)

target_include_directories(SceneLoaderUSD PUBLIC
        $<BUILD_INTERFACE:${OPEN_USD_DIR_BUILD}/include>
        "${CMAKE_SOURCE_DIR}/Headers/SceneLoaderUSD"
        "${CMAKE_SOURCE_DIR}/Headers/Client"
        "${CMAKE_SOURCE_DIR}/Assets/Shaders"
)
target_compile_definitions(SceneLoaderUSD PUBLIC NOMINMAX)

target_link_libraries(SceneLoaderUSD PUBLIC
        usd_m
        tbb
        tbb_malloc
)

# -------------- SCENE LOADER COPY FILES  -------------

# Copy usd_ms.dll to bin dir
add_custom_command(TARGET SceneLoaderUSD POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPEN_USD_DIR_BUILD}/lib/usd_ms.dll"
        "${USD_BIN_DIR}"
)

# Copy tbb.lib to bin dir
add_custom_command(TARGET SceneLoaderUSD POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPEN_USD_DIR_BUILD}/lib/tbb.lib"
        "${USD_BIN_DIR}"
)

# Copy tbb_debug.lib to bin dir
add_custom_command(TARGET SceneLoaderUSD POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPEN_USD_DIR_BUILD}/lib/tbb_debug.lib"
        "${USD_BIN_DIR}"
)

# Copy bin/* to bin dir
add_custom_command(TARGET SceneLoaderUSD POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${OPEN_USD_DIR_BUILD}/bin"
        "${USD_BIN_DIR}"
)

# Copy lib/usd dir to bin dir
add_custom_command(TARGET SceneLoaderUSD POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${OPEN_USD_DIR_BUILD}/lib/usd"
        "${USD_BIN_DIR}/usd"
)

if(USD_DEBUG)
    # Copy usd_ms.pdb to bin dir
    add_custom_command(TARGET SceneLoaderUSD POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPEN_USD_DIR_BUILD}/lib/usd_ms.pdb"
            "${USD_BIN_DIR}"
    )
endif()

# Copy SceneLoaderUSD lib/dll to bin dir
add_custom_command(TARGET SceneLoaderUSD POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/SceneLoaderUSD.lib"
        "${USD_BIN_DIR}"
)
add_custom_command(TARGET SceneLoaderUSD POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/SceneLoaderUSD.dll"
        "${USD_BIN_DIR}"
)

# -------------- SCENE LOADER TEST TARGET -------------

add_executable(SceneLoaderUSD_Test
        "${CMAKE_SOURCE_DIR}/Source/SceneLoaderUSD/Test.cpp"
)

target_link_libraries(SceneLoaderUSD_Test PRIVATE SceneLoaderUSD)
target_link_directories(SceneLoaderUSD_Test PRIVATE "${USD_BIN_DIR}")

add_custom_command(TARGET SceneLoaderUSD POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${USD_BIN_DIR}"
        $<TARGET_FILE_DIR:SceneLoaderUSD_Test>
)

