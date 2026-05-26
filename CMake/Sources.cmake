# Source files
set(PROJECT_SOURCES

)

# Header files
set(PROJECT_HEADERS
        Headers/client/Helper.h
        Headers/client/MathUtils.h
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES MATCHES "Debug")
    set(PROJECT_SOURCES
            ${PROJECT_SOURCES}
    )
    set(PROJECT_HEADERS
            ${PROJECT_HEADERS}
    )
endif()