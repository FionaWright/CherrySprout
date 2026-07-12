set(IS_DEBUG FALSE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES MATCHES "Debug")
    set(IS_DEBUG TRUE)
endif()

if (IS_DEBUG)
    set(DEFAULT_DEBUG_ON ON)
else()
    set(DEFAULT_DEBUG_ON OFF)
endif()

option(CHERRY_PRINT_ENABLED "Cherry Printing Enabled" ${DEFAULT_DEBUG_ON})
option(CHERRY_ASSERT_ENABLED "Cherry Asserts Enabled" ${DEFAULT_DEBUG_ON})
option(CHERRY_DEBUG_FEATURES_ENABLED "Cherry Debug Features Enabled" ${DEFAULT_DEBUG_ON})

add_library(ProjectOptions INTERFACE)

if(CHERRY_PRINT_ENABLED)
    target_compile_definitions(ProjectOptions INTERFACE CHERRY_PRINT_ENABLED)
endif()

if(CHERRY_ASSERT_ENABLED)
    target_compile_definitions(ProjectOptions INTERFACE CHERRY_ASSERT_ENABLED)
endif()

if(CHERRY_DEBUG_FEATURES_ENABLED)
    target_compile_definitions(ProjectOptions INTERFACE CHERRY_DEBUG_FEATURES_ENABLED)
endif()