cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(vpkedit
        DESCRIPTION "A tool to read, preview, and write to VPK files."
        VERSION "4.1.0"
        HOMEPAGE_URL "https://github.com/craftablescience/VPKEdit")
set(PROJECT_NAME_PRETTY "VPKEdit" CACHE STRING "" FORCE)
# If the tweak number exists, we're in beta
if(PROJECT_VERSION_TWEAK STREQUAL "")
    # Proper release version
    set(PROJECT_VERSION_PRETTY "${PROJECT_VERSION}" CACHE STRING "" FORCE)
elseif(PROJECT_VERSION_TWEAK MATCHES "^99+$")
    # Release candidate, number of 9s controls the RC number
    string(LENGTH ${PROJECT_VERSION_TWEAK} PROJECT_VERSION_TWEAK_LENGTH)
    math(EXPR PROJECT_VERSION_TWEAK_LENGTH "${PROJECT_VERSION_TWEAK_LENGTH} - 1" OUTPUT_FORMAT DECIMAL)
    set(PROJECT_VERSION_PRETTY "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-rc.${PROJECT_VERSION_TWEAK_LENGTH}" CACHE STRING "" FORCE)
else()
    # Beta version
    set(PROJECT_VERSION_PRETTY "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-beta.${PROJECT_VERSION_TWEAK}" CACHE STRING "" FORCE)
endif()
set(PROJECT_ORGANIZATION_NAME "craftablescience" CACHE STRING "" FORCE)
set(PROJECT_HOMEPAGE_URL_API "https://api.github.com/repos/craftablescience/VPKEdit" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 20)

option(VPKEDIT_BUILD_CLI "Build VPKEdit CLI application" ON)
option(VPKEDIT_BUILD_GUI "Build VPKEdit GUI application" ON)
option(VPKEDIT_BUILD_INSTALLER "Build installer for VPKEdit GUI application" ON)
option(VPKEDIT_BUILD_TESTS "Run library tests" OFF)
option(VPKEDIT_BUILD_FOR_STRATA_SOURCE "Build VPKEdit with the intent of the CLI/GUI going into the bin folder of a Strata Source game" OFF)

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
