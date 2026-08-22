cmake_minimum_required(VERSION 3.21)

if(POLICY CMP0219)
    cmake_policy(SET CMP0219 OLD)
endif()

project(holyscreen VERSION 0.10.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

find_package(Qt6 COMPONENTS Core Gui Quick QuickControls2 Sql Multimedia Network HttpServer WebSockets Test REQUIRED)
qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)

install(TARGETS church-presenter
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
if(APPLE)
    qt_generate_deploy_script(
        TARGET church-presenter
        NAME deploy_holyscreen
        OUTPUT_SCRIPT deploy_script
        CONTENT [[
qt6_deploy_qml_imports(TARGET church-presenter PLUGINS_FOUND plugins_found)

# Homebrew distributes some QML plugins as relative symlinks. CPack preserves
# those links in its staging directory, where they no longer point at the Qt
# installation. Materialize only those broken links before macdeployqt scans
# the bundle. Official Qt packages contain regular files, so this is a no-op in
# the release runners.
foreach(plugin IN LISTS plugins_found)
    set(installed_plugin "\${QT_DEPLOY_PREFIX}/\${plugin}")
    if(IS_SYMLINK "\${installed_plugin}" AND NOT EXISTS "\${installed_plugin}")
        get_filename_component(plugin_name "\${installed_plugin}" NAME)
        file(GLOB_RECURSE source_candidates
            FOLLOW_SYMLINKS
            LIST_DIRECTORIES false
            "\${__QT_DEPLOY_QT_INSTALL_PREFIX}/share/qt/qml/\${plugin_name}"
        )
        set(source_plugin "")
        foreach(candidate IN LISTS source_candidates)
            if(EXISTS "\${candidate}")
                file(REAL_PATH "\${candidate}" source_plugin)
                break()
            endif()
        endforeach()
        if(source_plugin STREQUAL "")
            message(FATAL_ERROR "Could not materialize QML plugin: \${plugin_name}")
        endif()
        file(REMOVE "\${installed_plugin}")
        file(COPY_FILE "\${source_plugin}" "\${installed_plugin}")
    endif()
endforeach()

qt6_deploy_runtime_dependencies(
    EXECUTABLE "$<TARGET_FILE_NAME:church-presenter>.app"
    ADDITIONAL_MODULES \${plugins_found}
)
        ]]
    )
else()
    qt_generate_deploy_qml_app_script(
        TARGET church-presenter
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    )
endif()
install(SCRIPT ${deploy_script})

if(UNIX AND NOT APPLE)
    install(FILES packaging/app.holyscreen.presenter.desktop
        DESTINATION share/applications
    )
    install(FILES packaging/holyscreen.svg
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME app.holyscreen.presenter.svg
    )
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "HolyScreen")
set(CPACK_PACKAGE_VENDOR "HolyScreen")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Motor de apresentação para igrejas")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "HolyScreen")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
endif()
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "HolyScreen")
endif()
include(CPack)
