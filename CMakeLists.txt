cmake_minimum_required(VERSION 3.15)
project(vpktool)
set(CMAKE_CXX_STANDARD 17)

option(VPKTOOL_BUILD_GUI "Build VPKTool GUI application" ON)
option(VPKTOOL_BUILD_TESTS "Run library tests" OFF)

add_library(
        lib${PROJECT_NAME}

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpktool/InputStream.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpktool/VPK.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/InputStream.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/VPK.cpp")

target_include_directories(
        lib${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(VPKTOOL_BUILD_GUI)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
    set(CMAKE_INSTALL_RPATH $ORIGIN)

    set(VTFLIB_STATIC OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/gui/thirdparty/vtflib")
    set_target_properties(
            vtflib PROPERTIES
            # I don't know which one of these puts it next to the executable so let's do all of them!
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

    if(WIN32)
        if(DEFINED QT_BASEDIR)
            # Add it to the prefix path so find_package can find it
            list(APPEND CMAKE_PREFIX_PATH "${QT_BASEDIR}")
            set(QT_INCLUDE "${QT_BASEDIR}/include")
            # CMake has an odd policy that links a special link lib for Qt on newer versions of CMake
            cmake_policy(SET CMP0020 NEW)
        else()
            message(FATAL_ERROR "Please define your QT install dir with -DQT_BASEDIR=\"C:/your/qt6/here\"")
        endif()
        message(STATUS "Using ${QT_INCLUDE} as the Qt include directory")
    endif()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(
            ${PROJECT_NAME} WIN32

            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/res/res.qrc"

            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/VTFPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/VTFPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/EntryTree.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/EntryTree.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/FileViewer.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/FileViewer.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Main.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Window.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Window.cpp")

    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
    target_link_libraries(${PROJECT_NAME} PRIVATE lib${PROJECT_NAME} vtflib Qt6::Core Qt6::Gui Qt6::Widgets)
    target_include_directories(
            ${PROJECT_NAME} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${QT_INCLUDE}"
            "${QT_INCLUDE}/QtCore"
            "${QT_INCLUDE}/QtGui"
            "${QT_INCLUDE}/QtWidgets")
    if(WIN32 AND MSVC)
        target_link_options(
                ${PROJECT_NAME} PRIVATE
                "/ENTRY:mainCRTStartup")
    endif()

    # Copy these so the user doesn't have to
    if(WIN32)
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            configure_file("${QT_BASEDIR}/bin/Qt6Cored.dll"    "${CMAKE_BINARY_DIR}/Qt6Cored.dll" COPYONLY)
            configure_file("${QT_BASEDIR}/bin/Qt6Guid.dll"     "${CMAKE_BINARY_DIR}/Qt6Guid.dll" COPYONLY)
            configure_file("${QT_BASEDIR}/bin/Qt6Widgetsd.dll" "${CMAKE_BINARY_DIR}/Qt6Widgetsd.dll" COPYONLY)
            configure_file("${QT_BASEDIR}/plugins/platforms/qwindowsd.dll"        "${CMAKE_BINARY_DIR}/platforms/qwindowsd.dll" COPYONLY)
            configure_file("${QT_BASEDIR}/plugins/styles/qwindowsvistastyled.dll" "${CMAKE_BINARY_DIR}/styles/qwindowsvistastyled.dll" COPYONLY)
        else()
            configure_file("${QT_BASEDIR}/bin/Qt6Core.dll"    "${CMAKE_BINARY_DIR}/Qt6Core.dll" COPYONLY)
            configure_file("${QT_BASEDIR}/bin/Qt6Gui.dll"     "${CMAKE_BINARY_DIR}/Qt6Gui.dll" COPYONLY)
            configure_file("${QT_BASEDIR}/bin/Qt6Widgets.dll" "${CMAKE_BINARY_DIR}/Qt6Widgets.dll" COPYONLY)
            configure_file("${QT_BASEDIR}/plugins/platforms/qwindows.dll"        "${CMAKE_BINARY_DIR}/platforms/qwindows.dll" COPYONLY)
            configure_file("${QT_BASEDIR}/plugins/styles/qwindowsvistastyle.dll" "${CMAKE_BINARY_DIR}/styles/qwindowsvistastyle.dll" COPYONLY)
        endif()
    elseif(UNIX)
        configure_file("${QT_BASEDIR}/lib/libQt6Core.so"    "${CMAKE_BINARY_DIR}/libQt6Core.so.6" COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6Gui.so"     "${CMAKE_BINARY_DIR}/libQt6Gui.so.6" COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6Widgets.so" "${CMAKE_BINARY_DIR}/libQt6Widgets.so.6" COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/platforms/libqminimal.so" "${CMAKE_BINARY_DIR}/platforms/libqminimal.so" COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/platforms/libqxcb.so"     "${CMAKE_BINARY_DIR}/platforms/libqxcb.so" COPYONLY)
    endif()
endif()

if(VPKTOOL_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1)
    FetchContent_MakeAvailable(googletest)
    enable_testing()

    add_executable(${PROJECT_NAME}test ${CMAKE_CURRENT_SOURCE_DIR}/tests/VPKTest.cpp)
    target_link_libraries(${PROJECT_NAME}test PUBLIC lib${PROJECT_NAME} gtest_main)
    target_include_directories(
            ${PROJECT_NAME}test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}test)
endif()
