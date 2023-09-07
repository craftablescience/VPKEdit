cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(vpktool VERSION 2.3.0 HOMEPAGE_URL "https://github.com/craftablescience/VPKTool")
set(CMAKE_CXX_STANDARD 17)

option(VPKTOOL_BUILD_GUI "Build VPKTool GUI application" ON)
option(VPKTOOL_BUILD_INSTALLER "Build installer for VPKTool GUI application" ON)
option(VPKTOOL_BUILD_TESTS "Run library tests" OFF)

add_library(
        lib${PROJECT_NAME}

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpktool/InputStream.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpktool/VPK.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/InputStream.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/VPK.cpp")

target_include_directories(
        lib${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(VPKTOOL_BUILD_GUI)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
    set(CMAKE_INSTALL_RPATH $ORIGIN)

    # VTFLib
    set(VTFLIB_STATIC OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/gui/thirdparty/vtflib")
    set_target_properties(
            vtflib PROPERTIES
            # I don't know which one of these puts it next to the executable so let's do all of them!
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

    # SpeedyKeyV (for SAPP)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/gui/thirdparty/speedykeyv")

    # SAPP
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/gui/thirdparty/sapp")

    # Qt
    if(WIN32 AND NOT DEFINED QT_BASEDIR)
        message(FATAL_ERROR "Please define your QT install dir with -DQT_BASEDIR=\"C:/your/qt6/here\"")
    endif()

    if(DEFINED QT_BASEDIR)
        string(REPLACE "\\" "/" QT_BASEDIR "${QT_BASEDIR}")

        # Add it to the prefix path so find_package can find it
        list(APPEND CMAKE_PREFIX_PATH "${QT_BASEDIR}")
        set(QT_INCLUDE "${QT_BASEDIR}/include")
        message(STATUS "Using ${QT_INCLUDE} as the Qt include directory")
    endif()

    if(WIN32)
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            set(QT_LIB_SUFFIX "d" CACHE STRING "" FORCE)
        else()
            set(QT_LIB_SUFFIX "" CACHE STRING "" FORCE)
        endif()
    endif()

    # CMake has an odd policy that links a special link lib for Qt on newer versions of CMake
    cmake_policy(SET CMP0020 NEW)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    # Config file
    set(VPKTOOL_PROJECT_NAME ${CMAKE_PROJECT_NAME} CACHE STRING "" FORCE)
    set(VPKTOOL_PROJECT_VERSION ${CMAKE_PROJECT_VERSION} CACHE STRING "" FORCE)
    set(VPKTOOL_PROJECT_HOMEPAGE ${CMAKE_PROJECT_HOMEPAGE_URL} CACHE STRING "" FORCE)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Config.h.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Config.h"
            @ONLY)

    # Add sources and create executable
    list(APPEND ${PROJECT_NAME}_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Config.h"

            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/res/res.qrc"

            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/DirPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/DirPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/ImagePreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/ImagePreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/TextPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/TextPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/VTFPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/VTFPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/EntryTree.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/EntryTree.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/FileViewer.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/FileViewer.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Main.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Window.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Window.cpp")
    if(WIN32)
        list(APPEND ${PROJECT_NAME}_SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/res/icon.rc")
    endif()

    add_executable(
            ${PROJECT_NAME} WIN32
            ${${PROJECT_NAME}_SOURCES})

    # Final Qt setup
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
    target_link_libraries(${PROJECT_NAME} PRIVATE lib${PROJECT_NAME} vtflib keyvalues SAPP Qt::Core Qt::Gui Qt::Widgets)
    target_include_directories(
            ${PROJECT_NAME} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${QT_INCLUDE}"
            "${QT_INCLUDE}/QtCore"
            "${QT_INCLUDE}/QtGui"
            "${QT_INCLUDE}/QtWidgets"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/thirdparty/sapp/include") # For some reason this one isn't populating automatically
    if(WIN32 AND MSVC)
        target_link_options(
                ${PROJECT_NAME} PRIVATE
                "/ENTRY:mainCRTStartup")
    endif()

    # Set up install rules
    # Note: if a Linux installer is ever made, DBus will need to be bundled
    install(TARGETS ${PROJECT_NAME} vtflib
            CONFIGURATIONS ${CMAKE_BUILD_TYPE}
            RUNTIME DESTINATION .
            LIBRARY DESTINATION .)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::Core Qt6::Gui Qt6::Widgets
            RUNTIME DESTINATION .
            LIBRARY DESTINATION .)
    install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            DESTINATION .)
    if(WIN32)
        # No convenient libraries for these... ugh
        install(FILES
                "${QT_BASEDIR}/plugins/imageformats/qjpeg${QT_LIB_SUFFIX}.dll"
                "${QT_BASEDIR}/plugins/imageformats/qtga${QT_LIB_SUFFIX}.dll"
                "${QT_BASEDIR}/plugins/imageformats/qwebp${QT_LIB_SUFFIX}.dll"
                DESTINATION imageformats)
        install(FILES "${QT_BASEDIR}/plugins/platforms/qwindows${QT_LIB_SUFFIX}.dll"
                DESTINATION platforms)
        install(FILES "${QT_BASEDIR}/plugins/styles/qwindowsvistastyle${QT_LIB_SUFFIX}.dll"
                DESTINATION styles)
    elseif(UNIX)
        install(FILES
                    "${QT_BASEDIR}/plugins/platforms/libqminimal.so"
                    "${QT_BASEDIR}/plugins/platforms/libqxcb.so"
                DESTINATION platforms)
        install(FILES
                    "${QT_BASEDIR}/lib/libicui18n.so.56"
                    "${QT_BASEDIR}/lib/libicuuc.so.56"
                    "${QT_BASEDIR}/lib/libicudata.so.56"
                DESTINATION .)
    endif()

    # Copy these so the user doesn't have to
    if(WIN32)
        configure_file("${QT_BASEDIR}/bin/Qt6Core${QT_LIB_SUFFIX}.dll"    "${CMAKE_BINARY_DIR}/Qt6Core${QT_LIB_SUFFIX}.dll"    COPYONLY)
        configure_file("${QT_BASEDIR}/bin/Qt6Gui${QT_LIB_SUFFIX}.dll"     "${CMAKE_BINARY_DIR}/Qt6Gui${QT_LIB_SUFFIX}.dll"     COPYONLY)
        configure_file("${QT_BASEDIR}/bin/Qt6Widgets${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/Qt6Widgets${QT_LIB_SUFFIX}.dll" COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/imageformats/qjpeg${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/imageformats/qjpeg${QT_LIB_SUFFIX}.dll" COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/imageformats/qtga${QT_LIB_SUFFIX}.dll"  "${CMAKE_BINARY_DIR}/imageformats/qtga${QT_LIB_SUFFIX}.dll"  COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/imageformats/qwebp${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/imageformats/qwebp${QT_LIB_SUFFIX}.dll" COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/platforms/qwindows${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/platforms/qwindows${QT_LIB_SUFFIX}.dll" COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/styles/qwindowsvistastyle${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/styles/qwindowsvistastyle${QT_LIB_SUFFIX}.dll" COPYONLY)
    elseif(UNIX)
        configure_file("${QT_BASEDIR}/lib/libQt6Core.so.6"    "${CMAKE_BINARY_DIR}/libQt6Core.so.6"    COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6Gui.so.6"     "${CMAKE_BINARY_DIR}/libQt6Gui.so.6"     COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6Widgets.so.6" "${CMAKE_BINARY_DIR}/libQt6Widgets.so.6" COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6DBus.so.6"    "${CMAKE_BINARY_DIR}/libQt6DBus.so.6"    COPYONLY)

        configure_file("${QT_BASEDIR}/lib/libQt6WaylandClient.so.6" "${CMAKE_BINARY_DIR}/libQt6WaylandClient.so.6" COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6XcbQpa.so.6"        "${CMAKE_BINARY_DIR}/libQt6XcbQpa.so.6"        COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libicui18n.so.56"         "${CMAKE_BINARY_DIR}/libicui18n.so.56"         COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libicuuc.so.56"           "${CMAKE_BINARY_DIR}/libicuuc.so.56"           COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libicudata.so.56"         "${CMAKE_BINARY_DIR}/libicudata.so.56"         COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/imageformats/libqjpeg.so" "${CMAKE_BINARY_DIR}/imageformats/libqjpeg.so" COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/imageformats/libqtga.so"  "${CMAKE_BINARY_DIR}/imageformats/libqtga.so"  COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/imageformats/libqwebp.so" "${CMAKE_BINARY_DIR}/imageformats/libqwebp.so" COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/platforms/libqwayland-generic.so" "${CMAKE_BINARY_DIR}/platforms/libqwayland-generic.so" COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/platforms/libqminimal.so"         "${CMAKE_BINARY_DIR}/platforms/libqminimal.so"         COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/platforms/libqxcb.so"             "${CMAKE_BINARY_DIR}/platforms/libqxcb.so"             COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/platformthemes/libqgtk3.so" "${CMAKE_BINARY_DIR}/platformthemes/libqgtk3.so" COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/wayland-shell-integration/libxdg-shell.so" "${CMAKE_BINARY_DIR}/wayland-shell-integration/libxdg-shell.so" COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/wayland-shell-integration/libqt-shell.so"  "${CMAKE_BINARY_DIR}/wayland-shell-integration/libqt-shell.so"  COPYONLY)
    endif()

    if(VPKTOOL_BUILD_INSTALLER)
        set(CPACK_PACKAGE_VENDOR "craftablescience")
        set(CPACK_PACKAGE_DESCRIPTION "A tool to read and extract data from VPK files")
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
        set(CPACK_MONOLITHIC_INSTALL ON)
        set(CPACK_PACKAGE_EXECUTABLES "vpktool;VPKTool")
        if(WIN32)
            set(CPACK_GENERATOR "NSIS")
            set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
            set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
            set(CPACK_NSIS_DISPLAY_NAME "VPKTool")
            set(CPACK_NSIS_PACKAGE_NAME "VPKTool")
            set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/res/icon.ico")
            set(CPACK_NSIS_INSTALLED_ICON_NAME "${PROJECT_NAME}.exe")
            set(CPACK_NSIS_URL_INFO_ABOUT "${CMAKE_PROJECT_HOMEPAGE_URL}")
            set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
            set(HELP_QUOTE "\"") # CMake is shit
            set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
                WriteRegStr HKCR '.vpk' '' 'VPKTool'
                WriteRegStr HKCR 'VPKTool' '' 'Valve PacK'
                WriteRegStr HKCR 'VPKTool\\\\shell' '' 'open'
                WriteRegStr HKCR 'VPKTool\\\\DefaultIcon' '' '$INSTDIR\\\\vpktool.exe,0'
                WriteRegStr HKCR 'VPKTool\\\\shell\\\\open\\\\command' '' '$INSTDIR\\\\vpktool.exe \\${HELP_QUOTE}%1\\${HELP_QUOTE}'
                WriteRegStr HKCR 'VPKTool\\\\shell\\\\edit' '' 'Browse VPK'
                WriteRegStr HKCR 'VPKTool\\\\shell\\\\edit\\\\command' '' '$INSTDIR\\\\vpktool.exe \\${HELP_QUOTE}%1\\${HELP_QUOTE}'
                System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
            ")
            set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
                DeleteRegKey HKCR '.vpk'
                DeleteRegKey HKCR 'VPKTool'
            ")
        else()
            message(STATUS "Installers on non-Windows platforms are not implemented")
        endif()
        include(CPack)
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
