cmake_minimum_required(VERSION 3.21)

project(holyscreen VERSION 0.10.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

find_package(Qt6 COMPONENTS Core Gui Quick QuickControls2 Sql Multimedia Network HttpServer WebSockets Test REQUIRED)
qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)

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
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "HolyScreen")
endif()
include(CPack)
