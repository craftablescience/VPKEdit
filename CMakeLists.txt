cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# set up project
project(vpkedit
        DESCRIPTION "A utility to create, edit, and display several pack file formats."
        VERSION 6.0.0.99
        HOMEPAGE_URL "https://vpkedit.app")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add helpers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers")
include(CS_All)

# extra metadata variables
set(PROJECT_NAME_PRETTY "VPKEdit" CACHE STRING "" FORCE)
cs_version_pretty() # PROJECT_VERSION_PRETTY
set(PROJECT_ORGANIZATION_NAME "craftablescience" CACHE STRING "" FORCE)
set(PROJECT_GITHUB_URL "https://github.com/craftablescience/VPKEdit" CACHE STRING "" FORCE)

# options
option(VPKEDIT_BUILD_FOR_STRATA_SOURCE "Build ${PROJECT_NAME_PRETTY} for Strata Source games" OFF)
option(VPKEDIT_BUILD_CLI               "Build ${PROJECT_NAME_PRETTY} CLI"                      ON)
option(VPKEDIT_BUILD_GUI               "Build ${PROJECT_NAME_PRETTY} GUI"                      ON)
option(VPKEDIT_BUILD_GUI_PLUGINS       "Build ${PROJECT_NAME_PRETTY} GUI plugins"              ON)
option(VPKEDIT_BUILD_INSTALLER         "Build ${PROJECT_NAME_PRETTY} installer"               OFF)

# initial VS integration
cs_setup_vs_defaults(STARTUP_PROJECT "${PROJECT_NAME}")

# set common compile options
option(VPKEDIT_USE_LTO "Compile ${PROJECT_NAME_PRETTY} with LTO/IPO" OFF)
cs_configure_defaults(VPKEDIT_USE_LTO)

# "system" cpack generators use system-provided Qt
if(DEFINED CPACK_GENERATOR AND ((CPACK_GENERATOR STREQUAL "DEB") OR (CPACK_GENERATOR STREQUAL "RPM")))
    set(VPKEDIT_SYSTEM_INSTALL ON CACHE BOOL "" FORCE)
else()
    set(VPKEDIT_SYSTEM_INSTALL OFF CACHE BOOL "" FORCE)
endif()

# external libraries
cs_include_directory(ext)

# common documents and config
if(VPKEDIT_BUILD_CLI OR VPKEDIT_BUILD_GUI)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CREDITS" "${CMAKE_BINARY_DIR}/CREDITS" COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_BINARY_DIR}/LICENSE" COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/common/Config.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/common/Config.hpp")
endif()

# vpkeditcli
if(VPKEDIT_BUILD_CLI)
    cs_include_directory(src/cli)
endif()

# vpkedit
if(VPKEDIT_BUILD_GUI)
    cs_include_directory(src/gui)
endif()

# vpkedit plugins
if(VPKEDIT_BUILD_GUI_PLUGINS)
    cs_include_directory(src/gui/plugins/previews)
    cs_include_directory(src/gui/plugins/thumbnails)
endif()

# installer
if(VPKEDIT_BUILD_INSTALLER)
    cs_include_directory(install)
endif()

# VS setup
if(WIN32 AND VPKEDIT_BUILD_GUI)
    set_target_properties(
            ${PROJECT_NAME} PROPERTIES
            VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${QT_BASEDIR}/bin;")
endif()
