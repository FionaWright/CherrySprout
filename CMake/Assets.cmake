get_target_property(TEXCONV_DIR texconv RUNTIME_OUTPUT_DIRECTORY)
set(TEXCONV_EXE ${TEXCONV_DIR}/texconv.exe)

# ============================================================================
# Textures
# ============================================================================

file(GLOB_RECURSE TEXTURE_FILES
        "${CMAKE_SOURCE_DIR}/Assets/Textures/*"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.jpg"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.tga"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.png"
)

set(TEXTURE_OUTPUTS "")

foreach(SRC ${TEXTURE_FILES})
    if(IS_DIRECTORY "${SRC}")
        continue()
    endif()

    file(RELATIVE_PATH REL_PATH
            "${CMAKE_SOURCE_DIR}/Assets"
            "${SRC}"
    )

    get_filename_component(EXT "${SRC}" EXT)
    string(TOLOWER "${EXT}" EXT)

    if(EXT STREQUAL ".hdr" OR EXT STREQUAL ".exr") # HDR / EXR -> copy

        set(DST "${CMAKE_BINARY_DIR}/Assets/${REL_PATH}")

        get_filename_component(DST_DIR "${DST}" DIRECTORY)
        file(MAKE_DIRECTORY "${DST_DIR}")

        add_custom_command(
                OUTPUT "${DST}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SRC}"
                "${DST}"
                DEPENDS "${SRC}"
                COMMENT "Copying texture ${REL_PATH}"
        )

        list(APPEND TEXTURE_OUTPUTS "${DST}")

    else() # LDR textures -> DDS

        get_filename_component(NAME_WE "${REL_PATH}" NAME_WE)
        get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)

        set(DDS_OUT
                "${CMAKE_BINARY_DIR}/Assets/${REL_DIR}/${NAME_WE}.dds"
        )

        get_filename_component(DDS_DIR "${DDS_OUT}" DIRECTORY)
        file(MAKE_DIRECTORY "${DDS_DIR}")

        add_custom_command(
                OUTPUT "${DDS_OUT}"
                COMMAND "${TEXCONV_EXE}"
                -y
                -ft dds
                -f BC7_UNORM -srgb
                -o "${DDS_DIR}"
                "${SRC}"
                DEPENDS "${SRC}"
                COMMENT "Converting ${REL_PATH} -> DDS"
                VERBATIM
        )

        list(APPEND TEXTURE_OUTPUTS "${DDS_OUT}")

    endif()

endforeach()

add_custom_target(AssetsTextures ALL
        DEPENDS ${TEXTURE_OUTPUTS}
)

add_dependencies(AssetsTextures texconv)
add_dependencies(client AssetsTextures)

# ============================================================================
# Scenes
# ============================================================================

file(GLOB_RECURSE SCENE_FILES
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.gltf"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.glb"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.usd"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.usda"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.usdc"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.usdz"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.mtlx"
        "${CMAKE_SOURCE_DIR}/Assets/Scenes/*.mdl"
)

set(SCENE_OUTPUTS "")

foreach(SRC ${SCENE_FILES})

    file(RELATIVE_PATH REL_PATH
            "${CMAKE_SOURCE_DIR}/Assets/Scenes"
            "${SRC}"
    )

    set(DST
            "${CMAKE_BINARY_DIR}/Assets/Scenes/${REL_PATH}"
    )

    get_filename_component(DST_DIR "${DST}" DIRECTORY)
    file(MAKE_DIRECTORY "${DST_DIR}")

    add_custom_command(
            OUTPUT "${DST}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SRC}"
            "${DST}"
            DEPENDS "${SRC}"
            COMMENT "Copying scene ${REL_PATH}"
    )

    list(APPEND SCENE_OUTPUTS "${DST}")

endforeach()

add_custom_target(AssetsScenes ALL
        DEPENDS ${SCENE_OUTPUTS}
)

add_dependencies(client AssetsScenes)

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES MATCHES "Release")
    add_custom_target(CopyShaders ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets/Shaders"
        "${CMAKE_BINARY_DIR}/Assets/Shaders"
        COMMENT "Copying shaders for Release"
    )
    add_dependencies(client CopyShaders)

    set(CHERRYSPROUT_SHADERS_DIR "${CMAKE_BINARY_DIR}/Assets/Shaders" CACHE INTERNAL "")
else()
    set(CHERRYSPROUT_SHADERS_DIR "${CMAKE_SOURCE_DIR}/Assets/Shaders" CACHE INTERNAL "")
endif()

target_compile_definitions(client PUBLIC
    CHERRYSPROUT_SHADERS_DIR="${CHERRYSPROUT_SHADERS_DIR}"
)

set(CHERRYSPROUT_ASSETS_DIR "${CMAKE_BINARY_DIR}/Assets" CACHE INTERNAL "")
target_compile_definitions(client PUBLIC
    CHERRYSPROUT_ASSETS_DIR="${CHERRYSPROUT_ASSETS_DIR}"
)