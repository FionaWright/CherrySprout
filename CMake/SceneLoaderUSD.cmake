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

if (NOT EXISTS "${USD_BIN_DIR}/usd_ms.lib")
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

    # Required
    file(MAKE_DIRECTORY "${USD_BIN_DIR}/include")

    # -------------- SCENE LOADER COPY FILES  -------------

    set(EXTRA_LIB )
    set(EXTRA_LIB_OUTPUT )
    if (USD_DEBUG)
        set(EXTRA_LIB "${USD_BIN_DIR}/tbb_debug.lib")
        set(EXTRA_LIB_OUTPUT OUTPUT "${USD_BIN_DIR}/tbb_debug.lib")
    endif()

    add_custom_command(
            OUTPUT "${USD_BIN_DIR}/usd_ms.lib"
            OUTPUT "${USD_BIN_DIR}/usd_ms.dll"
            OUTPUT "${USD_BIN_DIR}/tbb.lib"
            OUTPUT "${USD_BIN_DIR}/tbbmalloc.lib"
            ${EXTRA_LIB_OUTPUT}

            COMMAND ${CMAKE_COMMAND} -E make_directory "${USD_BIN_DIR}"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPEN_USD_DIR_BUILD}/lib/usd_ms.lib" "${USD_BIN_DIR}/usd_ms.lib"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPEN_USD_DIR_BUILD}/lib/usd_ms.dll" "${USD_BIN_DIR}/usd_ms.dll"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPEN_USD_DIR_BUILD}/lib/tbb.lib" "${USD_BIN_DIR}/tbb.lib"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPEN_USD_DIR_BUILD}/lib/tbbmalloc.lib" "${USD_BIN_DIR}/tbbmalloc.lib"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPEN_USD_DIR_BUILD}/lib/tbb_debug.lib" "${USD_BIN_DIR}/tbb_debug.lib"

            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${OPEN_USD_DIR_BUILD}/bin" "${USD_BIN_DIR}"

            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${OPEN_USD_DIR_BUILD}/lib/usd" "${USD_BIN_DIR}/usd"

            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${OPEN_USD_DIR_BUILD}/include" "${USD_BIN_DIR}/include"

            DEPENDS USD_EP
            VERBATIM
    )
    add_custom_target(SceneLoaderUSD_CopyBuild DEPENDS
            "${USD_BIN_DIR}/usd_ms.lib"
            "${USD_BIN_DIR}/usd_ms.dll"
            "${USD_BIN_DIR}/tbb.lib"
            ${EXTRA_LIB}
            "${USD_BIN_DIR}/tbbmalloc.lib"
    )

    if(USD_DEBUG)
        # Copy usd_ms.pdb to bin dir
        add_custom_command(TARGET SceneLoaderUSD_CopyBuild PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OPEN_USD_DIR_BUILD}/lib/usd_ms.pdb"
                "${USD_BIN_DIR}"
        )
    endif()

    set(DELETE_BUILD_DIR 1)

    # Delete OpenUSD source/build (They are MASSIVE and no longer needed)
    if(DELETE_BUILD_DIR AND EXISTS ${OPEN_USD_DIR})
        add_custom_command(TARGET SceneLoaderUSD_CopyBuild POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "${OPEN_USD_DIR}"
        )
    endif()
else()
    add_custom_target(USD_EP)
    add_custom_target(SceneLoaderUSD_CopyBuild)
endif()

# -------------- IMPORTED LIBRARIES -------------------

add_library(usd_m SHARED IMPORTED GLOBAL)
set_target_properties(usd_m PROPERTIES
        IMPORTED_IMPLIB "${USD_BIN_DIR}/usd_ms.lib"
        IMPORTED_LOCATION "${USD_BIN_DIR}/usd_ms.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${USD_BIN_DIR}/include"
)

add_dependencies(usd_m USD_EP SceneLoaderUSD_CopyBuild)

add_library(tbb SHARED IMPORTED GLOBAL)
set_target_properties(tbb PROPERTIES
        IMPORTED_LOCATION_DEBUG "${USD_BIN_DIR}/tbb_debug.dll"
        IMPORTED_IMPLIB_DEBUG "${USD_BIN_DIR}/tbb_debug.lib"

        IMPORTED_LOCATION_RELEASE "${USD_BIN_DIR}/tbb.dll"
        IMPORTED_IMPLIB_RELEASE "${USD_BIN_DIR}/tbb.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${USD_BIN_DIR}/include"
)

add_dependencies(tbb USD_EP SceneLoaderUSD_CopyBuild)

add_library(tbb_malloc SHARED IMPORTED GLOBAL)
set_target_properties(tbb_malloc PROPERTIES
        IMPORTED_IMPLIB "${USD_BIN_DIR}/tbbmalloc.lib"
        IMPORTED_LOCATION "${USD_BIN_DIR}/tbbmalloc.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${USD_BIN_DIR}/include"
)

add_dependencies(tbb_malloc USD_EP SceneLoaderUSD_CopyBuild)

# -------------- SCENE LOADER TARGET ------------------

add_library(SceneLoaderUSD SHARED
        "${CMAKE_SOURCE_DIR}/Source/SceneLoaderUSD/SceneLoaderUSD.cpp"
        "${CMAKE_SOURCE_DIR}/Source/SceneLoaderUSD/Importer.cpp"
        "${CMAKE_SOURCE_DIR}/Source/SceneLoaderUSD/Processor.cpp"
)

add_dependencies(SceneLoaderUSD SceneLoaderUSD_CopyBuild)

target_include_directories(SceneLoaderUSD PUBLIC
        $<BUILD_INTERFACE:${USD_BIN_DIR}/include>
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

if (USD_DEBUG)
    target_link_directories(SceneLoaderUSD PUBLIC "${USD_BIN_DIR}")
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

