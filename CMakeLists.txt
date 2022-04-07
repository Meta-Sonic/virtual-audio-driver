cmake_minimum_required(VERSION 3.15)

# Prevent from running cmake in root folder.
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "CMake needs to be called in a sub directory.")
endif()

project(virtual_audio_driver VERSION 1)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(CreateDriver)

# Options.
# option(BUILD_TESTS "Build tests" ON)

# No reason to set CMAKE_CONFIGURATION_TYPES if it's not a multiconfig generator
# Also no reason mess with CMAKE_BUILD_TYPE if it's a multiconfig generator.
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (IS_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
elseif (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release"  CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_XCODE_GENERATE_SCHEME ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(VIRTUAL_DRIVER_ROOT_DIRECTORY "${PROJECT_SOURCE_DIR}" CACHE INTERNAL "Virtual audio driver root directory")

CreateDriver(${PROJECT_NAME}
    "${PROJECT_SOURCE_DIR}/config/default_config.ini"
    "${PROJECT_SOURCE_DIR}/resources/MetaSonic.icns")
