cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Create project
project(vpkedit
        DESCRIPTION "A library and CLI/GUI tool to create, read, and write several pack file formats."
        VERSION "4.2.1"
        HOMEPAGE_URL "https://github.com/craftablescience/VPKEdit")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include CMake libraries
include(CheckIPOSupported)
include(GNUInstallDirs)

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
option(VPKEDIT_BUILD_LIBC "Build VPKEdit C wrapper library" OFF)
option(VPKEDIT_BUILD_CLI "Build VPKEdit CLI application" ON)
option(VPKEDIT_BUILD_GUI "Build VPKEdit GUI application" ON)
option(VPKEDIT_BUILD_INSTALLER "Build installer for VPKEdit GUI application" ON)
option(VPKEDIT_BUILD_EXAMPLE "Build library examples" OFF)
option(VPKEDIT_BUILD_TESTS "Run library tests" OFF)
option(VPKEDIT_BUILD_FOR_STRATA_SOURCE "Build VPKEdit with the intent of the CLI/GUI going into the bin folder of a Strata Source game" OFF)
option(VPKEDIT_USE_LTO "Build VPKEdit with link-time optimization enabled" OFF)

# Global CMake options
if(PROJECT_IS_TOP_LEVEL)
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

    # Create PDBs in release
    if(WIN32 AND MSVC)
        get_target_property(TARGET_TYPE ${TARGET} TYPE)
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
    endif()
endfunction()

# libvpkedit
include("${CMAKE_CURRENT_SOURCE_DIR}/src/lib/_lib.cmake")

# libvpkeditc
if(VPKEDIT_BUILD_LIBC)
    include("${CMAKE_CURRENT_SOURCE_DIR}/src/lib/lang/c/_c.cmake")

    # libvpkeditcs
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/lang/csharp/libvpkedit/libvpkedit.csproj.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/lang/csharp/libvpkedit/libvpkedit.csproj")
endif()

# vpkeditcli
if(VPKEDIT_BUILD_CLI)
    include("${CMAKE_CURRENT_SOURCE_DIR}/src/cli/_cli.cmake")
endif()

# vpkedit
if(VPKEDIT_BUILD_GUI)
    include("${CMAKE_CURRENT_SOURCE_DIR}/src/gui/_gui.cmake")
endif()

# Installer (GUI only, or CLI+GUI)
if(VPKEDIT_BUILD_GUI AND VPKEDIT_BUILD_INSTALLER)
    include("${CMAKE_CURRENT_SOURCE_DIR}/src/installer/_installer.cmake")
endif()

# vpkedit_example
if (VPKEDIT_BUILD_EXAMPLE)
    include("${CMAKE_CURRENT_SOURCE_DIR}/example/_example.cmake")
endif()

# vpkedit_test
if(VPKEDIT_BUILD_TESTS)
    include("${CMAKE_CURRENT_SOURCE_DIR}/test/_test.cmake")
endif()
