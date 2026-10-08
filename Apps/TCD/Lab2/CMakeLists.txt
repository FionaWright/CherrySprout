file(GLOB_RECURSE PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/Apps/TCD/Lab2/Source/*.cpp")
file(GLOB_RECURSE PROJECT_HEADERS "${CMAKE_SOURCE_DIR}/Apps/TCD/Lab2/Headers/*.h")

add_executable(Lab2 WIN32
        Source/main.cpp
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
        ${CMAKE_SOURCE_DIR}/Assets/Resources/CherrySprout.rc
        ${CMAKE_SOURCE_DIR}/Assets/Resources/Resources.h
)

target_compile_options(Lab2 PRIVATE
        $<$<CONFIG:Debug>:/Zi /Od /RTC1>
)

target_link_libraries(Lab2 PRIVATE client ProjectOptions)
set_target_properties(Lab2 PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_include_directories(Lab2 PRIVATE
        ${CMAKE_SOURCE_DIR}
        "${CMAKE_SOURCE_DIR}/Apps/TCD/Lab2/Headers"
)

# copy .dlls to output directory
add_custom_command(TARGET Lab2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:WinPixEventRuntime>
        $<TARGET_FILE_DIR:Lab2>
)
add_custom_command(TARGET Lab2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DXC_DLLS}
        $<TARGET_FILE_DIR:Lab2>
)

add_custom_command(TARGET Lab2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${USD_BIN_DIR}"
        $<TARGET_FILE_DIR:Lab2>
)

add_custom_command(TARGET Lab2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FFMPEG_EXECUTABLE}"
        "$<TARGET_FILE_DIR:Lab2>/ffmpeg.exe")

wis_install_dx_win32(Lab2)