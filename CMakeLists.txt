cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Create project
project(vpkedit
        DESCRIPTION "A CLI/GUI tool to create, read, and write several pack file formats."
        VERSION "4.4.1"
        HOMEPAGE_URL "https://github.com/craftablescience/VPKEdit")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include CMake libraries
include(CheckIPOSupported)
include(GNUInstallDirs)

# Initial VS integration
set(CMAKE_FOLDER "Thirdparty" CACHE INTERNAL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set up variables
set(PROJECT_NAME_PRETTY "VPKEdit" CACHE STRING "" FORCE)
# If the tweak number exists, version is a beta or release candidate
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

# Options
option(VPKEDIT_BUILD_INSTALLER "Build installer for VPKEdit GUI application" ON)
option(VPKEDIT_BUILD_FOR_STRATA_SOURCE "Build VPKEdit with the intent of the CLI/GUI going into the bin folder of a Strata Source game" OFF)
option(VPKEDIT_USE_LTO "Build VPKEdit with link-time optimization enabled" OFF)

# Global CMake options
if(PROJECT_IS_TOP_LEVEL)
    # Set proper runpath
    set(CMAKE_SKIP_BUILD_RPATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH $ORIGIN)

    # Compile with PIC
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # Compile with LTO if supported
    set(VPKEDIT_USE_LTO_INTERNAL OFF)
    if(VPKEDIT_USE_LTO)
        check_ipo_supported(RESULT VPKEDIT_USE_LTO_INTERNAL)
    endif()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${VPKEDIT_USE_LTO_INTERNAL})

    # Set default install directory permissions
    set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

# Macro to set up many things at once for a given target
function(vpkedit_configure_target TARGET)
    # Define DEBUG macro
    target_compile_definitions(${TARGET} PRIVATE "$<$<CONFIG:Debug>:DEBUG>")

    # Set optimization flags
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        # Build with debug friendly optimizations and debug symbols (MSVC defaults are fine)
        if(UNIX OR MINGW)
            target_compile_options(${TARGET} PRIVATE -Og -g)
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
            target_compile_options(${TARGET} PRIVATE -fno-limit-debug-info)
        endif()
    else()
        # Build with optimizations and don't omit stack pointer for debugging (MSVC defaults are fine)
        if(UNIX OR MINGW)
            target_compile_options(${TARGET} PRIVATE -O2 -fno-omit-frame-pointer)
        endif()
    endif()

    if(WIN32 AND MSVC)
        get_target_property(TARGET_TYPE ${TARGET} TYPE)
        get_target_property(TARGET_IS_GUI ${TARGET} WIN32_EXECUTABLE)

        # Organize VS solution into folders
        if(TARGET_TYPE STREQUAL "EXECUTABLE")
            set_target_properties(${TARGET} PROPERTIES FOLDER "Executables")
        else()
            set_target_properties(${TARGET} PROPERTIES FOLDER "Libraries")
        endif()

        # Create PDBs in release
        if((TARGET_TYPE STREQUAL "SHARED_LIBRARY") OR (TARGET_TYPE STREQUAL "EXECUTABLE"))
            target_compile_options(
                    ${TARGET} PRIVATE
                    "$<$<CONFIG:Release>:/Zi>")
            target_link_options(
                    ${TARGET} PRIVATE
                    "$<$<CONFIG:Release>:/DEBUG>"
                    "$<$<CONFIG:Release>:/OPT:REF>"
                    "$<$<CONFIG:Release>:/OPT:ICF>")
        endif()

        # Add an icon and manifest to the executable
        if(TARGET_TYPE STREQUAL "EXECUTABLE")
            target_sources(
                    ${TARGET} PRIVATE
                    "$<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/src/shared/res/logo_alt.rc>"
                    "$<$<NOT:$<CONFIG:Debug>>:${CMAKE_CURRENT_SOURCE_DIR}/src/shared/res/logo.rc>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/res/program.manifest")
        endif()

        # Don't show the console when running the executable
        if(TARGET_IS_GUI)
            target_link_options(
                    ${TARGET} PRIVATE
                    "/ENTRY:mainCRTStartup")
        endif()
    endif()
endfunction()

# Shared
include("${CMAKE_CURRENT_SOURCE_DIR}/src/shared/_shared.cmake")

# vpkeditcli
include("${CMAKE_CURRENT_SOURCE_DIR}/src/cli/_cli.cmake")

# vpkedit
include("${CMAKE_CURRENT_SOURCE_DIR}/src/gui/_gui.cmake")

# Installer
if(VPKEDIT_BUILD_INSTALLER)
    include("${CMAKE_CURRENT_SOURCE_DIR}/src/installer/_installer.cmake")
endif()

# VS setup
if(WIN32)
    set_property(
            DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
    set_target_properties(
            ${PROJECT_NAME} PROPERTIES
            VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>"
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
            VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${QT_BASEDIR}/bin;")

    # Qt hacks
    set_target_properties(${PROJECT_NAME}_lrelease PROPERTIES FOLDER "Qt")
    set_target_properties(${PROJECT_NAME}_lupdate PROPERTIES FOLDER "Qt")
    set_target_properties(${PROJECT_NAME}_other_files PROPERTIES FOLDER "Qt")
endif()
