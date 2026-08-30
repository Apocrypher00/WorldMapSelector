cmake_minimum_required(VERSION 3.21)

project(
    WorldMapSelector
    VERSION 1.4.0
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

set(packagePluginDirectory "SKSE/Plugins")
set(packageLicenseDirectory "${packagePluginDirectory}/WorldMapSelector/Licenses")

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${packagePluginDirectory}" COMPONENT ReleasePackage
    LIBRARY DESTINATION "${packagePluginDirectory}" COMPONENT ReleasePackage
)

install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/WorldMapSelector.ini"
    DESTINATION "${packagePluginDirectory}"
    COMPONENT ReleasePackage
)

function(install_license sourcePath outputName)
    if(NOT EXISTS "${sourcePath}")
        message(FATAL_ERROR "Required license file was not found: ${sourcePath}")
    endif()

    install(
        FILES "${sourcePath}"
        DESTINATION "${packageLicenseDirectory}"
        RENAME "${outputName}.txt"
        COMPONENT ReleasePackage
    )
endfunction()

install_license("${CMAKE_CURRENT_SOURCE_DIR}/UNLICENSE" "WorldMapSelector")
install_license("${CMAKE_CURRENT_SOURCE_DIR}/LICENSING.md" "WorldMapSelector-Licensing")
install_license("${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibVR/COPYING" "CommonLibSSE-NG-GPL-3.0")
install_license("${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibVR/EXCEPTIONS.md" "CommonLibSSE-NG-Exceptions")

set(vcpkgShareDirectory "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share")
install_license("${vcpkgShareDirectory}/directxmath/copyright" "directxmath")
install_license("${vcpkgShareDirectory}/directxtk/copyright" "directxtk")
install_license("${vcpkgShareDirectory}/fmt/copyright" "fmt")
install_license("${vcpkgShareDirectory}/minhook/copyright" "minhook")
install_license("${vcpkgShareDirectory}/nlohmann-json/copyright" "nlohmann-json")
install_license("${vcpkgShareDirectory}/rapidcsv/copyright" "rapidcsv")
install_license("${vcpkgShareDirectory}/simpleini/copyright" "simpleini")
install_license("${vcpkgShareDirectory}/spdlog/copyright" "spdlog")
install_license("${vcpkgShareDirectory}/toml11/copyright" "toml11")
install_license("${vcpkgShareDirectory}/xbyak/copyright" "xbyak")

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "WorldMapSelector-${PROJECT_VERSION}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};ReleasePackage;/")
include(CPack)
