file(GLOB_RECURSE PROJECT_SOURCES "Source/*.cpp")

file(GLOB_RECURSE PROJECT_HEADERS "Headers/*.h")

set(IS_DEBUG CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES MATCHES "Debug")

if(NOT IS_DEBUG)
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/Source/Client/Debug/.*")
    list(FILTER PROJECT_HEADERS EXCLUDE REGEX ".*/Headers/Client/Debug/.*")
endif()

add_executable(Greenhouse WIN32
        Source/main.cpp
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
        ${CHERRYSPROUT_ASSETS_DIR}/Resources/CherrySprout.rc
        ${CHERRYSPROUT_ASSETS_DIR}/Resources/Resources.h
)

target_compile_options(Greenhouse PRIVATE
        $<$<CONFIG:Debug>:/Zi /Od /RTC1>
)

target_link_libraries(Greenhouse PRIVATE client)
set_target_properties(Greenhouse PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_include_directories(Greenhouse PRIVATE
        ${CMAKE_SOURCE_DIR}
        "${CMAKE_SOURCE_DIR}/Apps/Greenhouse/Headers"
)

# copy .dlls to output directory
add_custom_command(TARGET Greenhouse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:WinPixEventRuntime>
        $<TARGET_FILE_DIR:Greenhouse>
)
add_custom_command(TARGET Greenhouse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DXC_DLLS}
        $<TARGET_FILE_DIR:Greenhouse>
)

wis_install_dx_win32(Greenhouse)