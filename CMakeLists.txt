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

find_package(minhook CONFIG REQUIRED)

add_commonlibsse_plugin(
    ${PROJECT_NAME}
    SOURCES
        src/Config.cpp
        src/Diagnostics.cpp
        src/MapMarkerOverride.cpp
        src/WorldspaceCatalog.cpp
        src/WorldspaceOverride.cpp
        src/plugin.cpp
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        minhook::minhook
)

target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/PCH.h"
)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/WorldMapSelector.ini"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/WorldMapSelector.ini"
)
