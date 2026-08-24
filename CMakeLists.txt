cmake_minimum_required(VERSION 3.21)

if(POLICY CMP0219)
    cmake_policy(SET CMP0219 OLD)
endif()

project(holyscreen VERSION 0.14.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build bundled dependencies statically" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Do not build bundled dependency tests" FORCE)
set(BUILD_CLI OFF CACHE BOOL "Do not build the libgit2 CLI" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Do not build bundled dependency examples" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "Do not build bundled dependency fuzzers" FORCE)
set(INSTALL_PROJECT OFF CACHE BOOL "Do not install bundled dependencies" FORCE)
set(USE_SSH OFF CACHE STRING "HolyScreen imports public HTTPS repositories only" FORCE)
set(USE_HTTP_PARSER builtin CACHE STRING "Use bundled libgit2 HTTP parser" FORCE)
set(REGEX_BACKEND builtin CACHE STRING "Use bundled libgit2 regex backend" FORCE)
set(USE_BUNDLED_ZLIB ON CACHE STRING "Use bundled libgit2 zlib" FORCE)
set(USE_NTLMCLIENT OFF CACHE BOOL "Public Bible imports do not use NTLM" FORCE)
set(STATIC_CRT OFF CACHE BOOL "Match the Qt dynamic MSVC runtime" FORCE)

FetchContent_Declare(libgit2
    URL https://github.com/libgit2/libgit2/archive/refs/tags/v1.9.7.tar.gz
    URL_HASH SHA256=1a4fbe7589e814777ae76b64734ad80f4ecad22cd33a22682a2aaea4ae5375e7
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(miniz
    URL https://github.com/richgel999/miniz/archive/refs/tags/3.1.2.tar.gz
    URL_HASH SHA256=98468f8924934b723276680f85238b6c78bf1f8b49b4459cc9b7214a20e2e9fb
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(RTMIDI_BUILD_STATIC_LIBS ON CACHE BOOL "Link RtMidi statically" FORCE)
set(RTMIDI_BUILD_TESTING OFF CACHE BOOL "Do not build the RtMidi test programs" FORCE)
set(RTMIDI_TARGETNAME_UNINSTALL "rtmidi_uninstall" CACHE STRING "Avoid clashing uninstall targets" FORCE)

FetchContent_Declare(rtmidi
    URL https://github.com/thestk/rtmidi/archive/refs/tags/6.0.0.tar.gz
    URL_HASH SHA256=ef7bcda27fee6936b651c29ebe9544c74959d0b1583b716ce80a1c6fea7617f0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(qrcodegen
    URL https://github.com/nayuki/QR-Code-generator/archive/2c9044de6b049ca25cb3cd1649ed7e27aa055138.tar.gz
    URL_HASH SHA256=fd5b5079196cbe4d733838e784eecc1d854f3772562f166483a9b617844922f5
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(libgit2 miniz qrcodegen rtmidi)

# miniz 3.1.2 forces /Zi and /Fd in its own CMake project. sccache then expects
# a program database even when our later /Z7 option makes MSVC embed the debug
# information in each object. Compile only this small C target directly while
# retaining the cache for libgit2 and all HolyScreen C++ targets.
if(MSVC AND TARGET miniz)
    set_property(TARGET miniz PROPERTY C_COMPILER_LAUNCHER "")
endif()

find_package(Qt6 COMPONENTS Core Gui Quick QuickControls2 Sql Multimedia Network HttpServer WebSockets Concurrent Test QuickTest LinguistTools REQUIRED)
qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)

qt_add_translations(
    TARGETS church-presenter
    SOURCE_TARGETS presenter-ui presenter-app
    TS_FILES
        translations/holyscreen_pt_BR.ts
        translations/holyscreen_en_US.ts
    RESOURCE_PREFIX "/i18n"
    LUPDATE_OPTIONS -no-obsolete
)

install(TARGETS church-presenter
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
install(FILES THIRD_PARTY_NOTICES.md DESTINATION share/doc/HolyScreen)
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
        EXCLUDE_PLUGIN_TYPES qmltooling
        EXCLUDE_PLUGINS qsqlibase qsqlmimer qsqloci qsqlodbc qsqlpsql
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
