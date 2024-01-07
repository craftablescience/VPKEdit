cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(vpkedit
        DESCRIPTION "A tool to read, preview, and write to VPK files."
        VERSION "4.0.0.1"
        HOMEPAGE_URL "https://github.com/craftablescience/VPKEdit")
set(PROJECT_NAME_PRETTY "VPKEdit" CACHE STRING "" FORCE)
set(PROJECT_ORGANIZATION_NAME "craftablescience" CACHE STRING "" FORCE)
set(PROJECT_HOMEPAGE_URL_API "https://api.github.com/repos/craftablescience/VPKEdit" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

option(VPKEDIT_BUILD_CLI "Build VPKEdit CLI application" ON)
option(VPKEDIT_BUILD_GUI "Build VPKEdit GUI application" ON)
option(VPKEDIT_BUILD_INSTALLER "Build installer for VPKEdit GUI application" ON)
option(VPKEDIT_BUILD_TESTS "Run library tests" OFF)

# libvpkedit
include("${CMAKE_CURRENT_SOURCE_DIR}/src/lib/_lib.cmake")

# vpkeditcli
if(VPKEDIT_BUILD_CLI)
    include("${CMAKE_CURRENT_SOURCE_DIR}/src/cli/_cli.cmake")
endif()

# vpkedit
if(VPKEDIT_BUILD_GUI)
    include("${CMAKE_CURRENT_SOURCE_DIR}/src/gui/_gui.cmake")
endif()

# Installer
if(VPKEDIT_BUILD_GUI AND VPKEDIT_BUILD_INSTALLER)
    include("${CMAKE_CURRENT_SOURCE_DIR}/src/installer/_installer.cmake")
endif()

# vpkedittest
if(VPKEDIT_BUILD_TESTS)
    include("${CMAKE_CURRENT_SOURCE_DIR}/test/_test.cmake")
endif()
