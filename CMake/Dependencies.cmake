include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(CPM_DONT_UPDATE_MODULE_PATH ON)
set(GET_CPM_FILE "${CMAKE_CURRENT_LIST_DIR}/Dependencies/get_cpm.cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Set CPM source cache
if (NOT CPM_SOURCE_CACHE)
    set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_BINARY_DIR}/_deps_cache")
endif ()

# Get CPM
if (NOT EXISTS ${GET_CPM_FILE})
    file(DOWNLOAD
            https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake
            "${GET_CPM_FILE}"
    )
endif ()
include(${GET_CPM_FILE})

if (WIN32)
    include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/DepsWin.cmake)
endif()

# Download DXC if needed
include(${CMAKE_CURRENT_LIST_DIR}/Dependencies/DXC.cmake)

# CPM dependencies

# ImGui
CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.92.6
    DOWNLOAD_ONLY TRUE
)

# Manually make ImGui available as a target
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp

    # Backends for Win32 and DirectX 12
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)

# DirectXTex
CPMAddPackage(
        NAME DirectXTex
        GITHUB_REPOSITORY microsoft/DirectXTex
        GIT_TAG may2026
)

# Alias for consistency
if(TARGET DirectXTex)
    add_library(DirectXTex::DirectXTex ALIAS DirectXTex)
endif()

include(FetchContent)

set(FFMPEG_VERSION "8.1.2")
set(FFMPEG_URL "https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-${FFMPEG_VERSION}-essentials_build.zip")

set(FFMPEG_ROOT "${CMAKE_BINARY_DIR}/_deps/ffmpeg")

if (WIN32)
    if (NOT EXISTS "${FFMPEG_ROOT}/ffmpeg.exe")

        file(DOWNLOAD
                "${FFMPEG_URL}"
                "${CMAKE_BINARY_DIR}/ffmpeg.zip"
                SHOW_PROGRESS)

        file(ARCHIVE_EXTRACT
                INPUT "${CMAKE_BINARY_DIR}/ffmpeg.zip"
                DESTINATION "${FFMPEG_ROOT}")

        file(GLOB FFMPEG_BIN
                "${FFMPEG_ROOT}/*/bin/ffmpeg.exe")

        list(GET FFMPEG_BIN 0 FFMPEG_EXE)

        file(COPY
                "${FFMPEG_EXE}"
                DESTINATION "${FFMPEG_ROOT}")

    endif()

    set(FFMPEG_EXECUTABLE
            "${FFMPEG_ROOT}/ffmpeg.exe"
            CACHE FILEPATH "ffmpeg executable")
endif()