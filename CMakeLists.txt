cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

# set up project
project(vpkedit
        DESCRIPTION "A CLI/GUI tool to create, read, and write several pack file formats."
        VERSION "5.0.0.3"
        HOMEPAGE_URL "https://github.com/craftablescience/VPKEdit")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# options
option(VPKEDIT_BUILD_FOR_STRATA_SOURCE "Build VPKEdit with the intent of the CLI/GUI going into the bin folder of a Strata Source game" OFF)
option(VPKEDIT_BUILD_INSTALLER "Build installer for VPKEdit GUI application" ON)

# add helpers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers")
include(CS_All)
include(GNUInstallDirs)

# extra metadata variables
set(PROJECT_NAME_PRETTY "VPKEdit")
cs_version_pretty() # PROJECT_VERSION_PRETTY
set(PROJECT_ORGANIZATION_NAME "craftablescience" CACHE INTERNAL "" FORCE)
set(PROJECT_HOMEPAGE_URL_API "https://api.github.com/repos/craftablescience/VPKEdit" CACHE INTERNAL "" FORCE)

# initial VS integration
cs_setup_vs_defaults(STARTUP_PROJECT "${PROJECT_NAME}")

# set common compile options
option(VPKEDIT_USE_LTO "Compile ${PROJECT_NAME_PRETTY} with LTO/IPO" OFF)
cs_configure_defaults(VPKEDIT_USE_LTO)

# shared code
cs_include_directory(ext)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/shared/Config.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/shared/Config.h")

# cs_configure_target wrapper to have a custom icon and manifest path
function(vpkedit_configure_target TARGET)
    cs_configure_target(${TARGET} MANIFEST)
    if(WIN32 AND MSVC)
        get_target_property(TARGET_TYPE ${TARGET} TYPE)
        if(TARGET_TYPE STREQUAL "EXECUTABLE")
            target_sources(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/res/logo$<$<CONFIG:Debug>:_alt>.rc")
        endif()
    elseif(APPLE)
        option(VPKEDIT_MAC_BUNDLE_QT "Bundle Qt into the application bundle for macOS" ON)
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macOS")
        include(MakeICNS)
        set(MACOS_ICON "${CMAKE_CURRENT_BINARY_DIR}/logo.icns")
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            make_icns_from_png(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/res/brand/logo_alt.png" OUTPUT "${MACOS_ICON}")
        else()
            make_icns_from_png(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/res/brand/logo.png" OUTPUT "${MACOS_ICON}")
        endif()
        target_sources(${TARGET} PRIVATE ${MACOS_ICON})
        set_source_files_properties(${MACOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        # app bundle properties
        set_target_properties(${TARGET} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/install/macos/MacOSXBundleInfo.plist.in"
            MACOSX_BUNDLE_ICON_FILE logo.icns
            MACOSX_BUNDLE_GUI_IDENTIFIER "info.${PROJECT_ORGANIZATION_NAME}.${PROJECT_NAME}"
            MACOSX_BUNDLE_INFO_STRING ${CMAKE_PROJECT_DESCRIPTION}
            MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME_PRETTY}
            MACOSX_BUNDLE_BUNDLE_VERSION ${CMAKE_PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${CMAKE_PROJECT_VERSION}
            MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2022 Laura Lewis")
        add_custom_command(TARGET ${TARGET}
            POST_BUILD COMMAND
            ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../Frameworks/"
            $<TARGET_FILE:${TARGET}>)
    endif()
endfunction()

# vpkeditcli
cs_include_directory(src/cli)

# vpkedit
cs_include_directory(src/gui)

# installer
if(VPKEDIT_BUILD_INSTALLER)
    cs_include_directory(install)
endif()

# VS setup
if(WIN32)
    set_target_properties(
            ${PROJECT_NAME} PROPERTIES
            VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${QT_BASEDIR}/bin;")
endif()
