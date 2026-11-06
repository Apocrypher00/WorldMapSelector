cmake_minimum_required(VERSION 3.21)

project(
    WorldMapSelector
    VERSION 1.5.0
    LANGUAGES CXX
)

# Keep the package manifest aligned with the version used for plugin metadata
# and release archive names.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json" vcpkgManifest)
string(JSON vcpkgVersion GET "${vcpkgManifest}" "version-string")
if(NOT "${vcpkgVersion}" STREQUAL "${PROJECT_VERSION}")
    message(FATAL_ERROR
        "Project version ${PROJECT_VERSION} does not match vcpkg manifest version ${vcpkgVersion}.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/CommonLibVR)

include("${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibVR/cmake/CommonLibSSE.cmake")

find_package(minhook CONFIG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIR SimpleIni.h REQUIRED)

add_commonlibsse_plugin(
    ${PROJECT_NAME}
    AUTHOR "Apocrypher00"
    COMPATIBLE_RUNTIMES
        1.5.97.0
        1.6.318.0
        1.6.323.0
        1.6.342.0
        1.6.353.0
        1.6.629.0
        1.6.640.0
        1.6.1130.0
        1.6.1170.0
        1.6.1179.1 # SKSE encodes GOG as runtime type 1; executable version remains 1.6.1179.0.
        1.7.99.0
        1.7.104.0
    SOURCES
        src/ClassicMessageBox.cpp
        src/ClassicMessageBox.h
        src/Config.cpp
        src/Diagnostics.cpp
        src/Hooks.cpp
        src/Hooks.h
        src/MapChooser.cpp
        src/MapChooserInput.cpp
        src/MapChooserInput.h
        src/MapMenuEvents.cpp
        src/MapMenuKeyHint.cpp
        src/MapMenuKeyHint.h
        src/MapMarkerOverride.cpp
        src/MapSelection.cpp
        src/MapSwitchFlow.cpp
        src/MapSwitchFlow.h
        src/RoutedMarkerOverride.cpp
        src/RoutedMarkerOverride.h
        src/Utilities.cpp
        src/Utilities.h
        src/WorldspaceCatalog.cpp
        src/WorldspaceOverride.cpp
        src/plugin.cpp
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        "${SIMPLEINI_INCLUDE_DIR}"
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        minhook::minhook
)

# Keep Windows min/max macros from colliding with standard-library functions.
target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
        NOMINMAX
)

target_precompile_headers(
    ${PROJECT_NAME}
    PRIVATE
        src/PCH.h
)

set(wmsConfigurationFile "${CMAKE_CURRENT_SOURCE_DIR}/WorldMapSelector.ini")

# The single-config Ninja presets place the DLL in this same directory.
# configure_file also tracks INI edits, so rebuilding refreshes it without relinking the DLL.
configure_file("${wmsConfigurationFile}" "${CMAKE_CURRENT_BINARY_DIR}/WorldMapSelector.ini" COPYONLY)

include(cmake/Packaging.cmake)
