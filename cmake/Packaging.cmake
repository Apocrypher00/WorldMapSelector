set(packagePluginDirectory "SKSE/Plugins")
set(packageLicenseDirectory "${packagePluginDirectory}/${PROJECT_NAME}/Licenses")

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${packagePluginDirectory}" COMPONENT ReleasePackage
    LIBRARY DESTINATION "${packagePluginDirectory}" COMPONENT ReleasePackage
)

install(
    FILES "${wmsConfigurationFile}"
    DESTINATION "${packagePluginDirectory}"
    RENAME "WorldMapSelector.ini"
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
install_license("${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md" "WorldMapSelector-ThirdPartyNotices")
install_license("${CMAKE_CURRENT_SOURCE_DIR}/SOURCES.md" "WorldMapSelector-Sources")
install_license("${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibVR/COPYING.txt" "CommonLibSSE-NG-GPL-3.0")
install_license("${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibVR/EXCEPTIONS.md" "CommonLibSSE-NG-Exceptions")

set(vcpkgShareDirectory "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share")
foreach(dependency directxmath directxtk fmt minhook nlohmann-json rapidcsv simpleini spdlog toml11 xbyak)
    install_license("${vcpkgShareDirectory}/${dependency}/copyright" "${dependency}")
endforeach()

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};ReleasePackage;/")
include(CPack)

# Both archives use the same compiled DLL; only their installed components differ.
install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${packagePluginDirectory}" COMPONENT TestingPackage
    LIBRARY DESTINATION "${packagePluginDirectory}" COMPONENT TestingPackage
)

# Track testing INI edits without replacing the public INI beside the loose DLL.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config/WorldMapSelector-testing.ini"
    "${CMAKE_CURRENT_BINARY_DIR}/WorldMapSelector-testing.ini"
    COPYONLY
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/WorldMapSelector-testing.ini"
    DESTINATION "${packagePluginDirectory}"
    RENAME "WorldMapSelector.ini"
    COMPONENT TestingPackage
)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/CPackTestingConfig.cmake" CONTENT
"include(\"${CMAKE_CURRENT_BINARY_DIR}/CPackConfig.cmake\")
set(CPACK_PACKAGE_FILE_NAME \"${PROJECT_NAME}-${PROJECT_VERSION}-testing\")
set(CPACK_INSTALL_CMAKE_PROJECTS \"${CMAKE_BINARY_DIR};${PROJECT_NAME};TestingPackage;/\")
")

# Private testing archives intentionally omit notices and must not be distributed.
add_custom_target(package-testing
    COMMAND "${CMAKE_CPACK_COMMAND}" --config "${CMAKE_CURRENT_BINARY_DIR}/CPackTestingConfig.cmake" -C "$<CONFIG>"
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    USES_TERMINAL
    VERBATIM
)
