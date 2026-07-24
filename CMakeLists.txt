cmake_minimum_required(VERSION 3.21)

project(
    WorldMapSelector
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENABLE_SKYRIM_SE OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/CommonLibVR)

include("${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibVR/cmake/CommonLibSSE.cmake")

add_commonlibsse_plugin(
    ${PROJECT_NAME}
    SOURCES
        src/plugin.cpp
)

target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/PCH.h"
)
