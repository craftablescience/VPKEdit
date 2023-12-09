cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(vpkedit
        DESCRIPTION "A tool to read, preview, and write to VPK files."
        VERSION 3.6.0
        HOMEPAGE_URL "https://github.com/craftablescience/VPKEdit")
set(PROJECT_NAME_PRETTY "VPKEdit" CACHE STRING "" FORCE)
set(PROJECT_ORGANIZATION_NAME "craftablescience" CACHE STRING "" FORCE)
set(PROJECT_HOMEPAGE_URL_API "https://api.github.com/repos/craftablescience/VPKEdit" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

option(VPKEDIT_BUILD_GUI "Build VPKEdit GUI application" ON)
option(VPKEDIT_BUILD_INSTALLER "Build installer for VPKEdit GUI application" ON)
option(VPKEDIT_BUILD_TESTS "Run library tests" OFF)

# For hashing parts of the VPK and stored files
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/lib/thirdparty/md5)

add_library(
        lib${PROJECT_NAME}

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/CRC.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/FileStream.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/VPK.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/detail/CRC.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/detail/FileStream.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/VPK.cpp")

target_link_libraries(lib${PROJECT_NAME} PUBLIC MD5)

target_include_directories(
        lib${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

# libvpkedit doesn't use C++20 yet
set_target_properties(lib${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

if(VPKEDIT_BUILD_GUI)
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

    # studiomodelpp
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/gui/thirdparty/studiomodelpp")

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
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/config/Config.h.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/config/Config.h"
            @ONLY)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/installer/deb/desktop.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/installer/deb/${PROJECT_NAME}.desktop")

    # Add sources and create executable
    list(APPEND ${PROJECT_NAME}_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/config/Config.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/config/Options.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/config/Options.cpp"

            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/NewEntryDialog.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/NewEntryDialog.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/NewUpdateDialog.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/NewUpdateDialog.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/VPKVersionDialog.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/dialogs/VPKVersionDialog.cpp"

            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/formats/VTFDecoder.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/formats/VTFDecoder.cpp"

            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/info/AbstractInfoPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/info/AbstractInfoPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/info/EmptyPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/info/EmptyPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/info/FileLoadErrorPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/info/FileLoadErrorPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/info/InvalidMDLErrorPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/info/InvalidMDLErrorPreview.cpp"

            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/DirPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/DirPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/ImagePreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/ImagePreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/MDLPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/MDLPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/TextPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/TextPreview.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/VTFPreview.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/previews/VTFPreview.cpp"

            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/res/res.qrc"

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
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network OpenGL OpenGLWidgets)
    target_link_libraries(${PROJECT_NAME} PRIVATE lib${PROJECT_NAME} vtflib studiomodelpp keyvalues SAPP Qt::Core Qt::Gui Qt::Widgets Qt::Network Qt::OpenGL Qt::OpenGLWidgets)
    target_include_directories(
            ${PROJECT_NAME} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/thirdparty/sapp/include"
            "${QT_INCLUDE}"
            "${QT_INCLUDE}/QtCore"
            "${QT_INCLUDE}/QtGui"
            "${QT_INCLUDE}/QtWidgets"
            "${QT_INCLUDE}/QtNetwork"
            "${QT_INCLUDE}/QtOpenGL"
            "${QT_INCLUDE}/QtOpenGLWidgets")
    if(WIN32 AND MSVC)
        target_link_options(
                ${PROJECT_NAME} PRIVATE
                "/ENTRY:mainCRTStartup")
    endif()

    # Copy these next to the executable
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md" "${CMAKE_BINARY_DIR}/CREDITS.md" COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"    "${CMAKE_BINARY_DIR}/LICENSE"    COPYONLY)
    # Don't copy the .nonportable file, we're debugging in standalone mode

    # Copy these so the user doesn't have to
    if(WIN32)
        configure_file("${QT_BASEDIR}/bin/Qt6Core${QT_LIB_SUFFIX}.dll"          "${CMAKE_BINARY_DIR}/Qt6Core${QT_LIB_SUFFIX}.dll"          COPYONLY)
        configure_file("${QT_BASEDIR}/bin/Qt6Gui${QT_LIB_SUFFIX}.dll"           "${CMAKE_BINARY_DIR}/Qt6Gui${QT_LIB_SUFFIX}.dll"           COPYONLY)
        configure_file("${QT_BASEDIR}/bin/Qt6Widgets${QT_LIB_SUFFIX}.dll"       "${CMAKE_BINARY_DIR}/Qt6Widgets${QT_LIB_SUFFIX}.dll"       COPYONLY)
        configure_file("${QT_BASEDIR}/bin/Qt6Network${QT_LIB_SUFFIX}.dll"       "${CMAKE_BINARY_DIR}/Qt6Network${QT_LIB_SUFFIX}.dll"       COPYONLY)
        configure_file("${QT_BASEDIR}/bin/Qt6OpenGL${QT_LIB_SUFFIX}.dll"        "${CMAKE_BINARY_DIR}/Qt6OpenGL${QT_LIB_SUFFIX}.dll"        COPYONLY)
        configure_file("${QT_BASEDIR}/bin/Qt6OpenGLWidgets${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/Qt6OpenGLWidgets${QT_LIB_SUFFIX}.dll" COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/imageformats/qjpeg${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/imageformats/qjpeg${QT_LIB_SUFFIX}.dll" COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/imageformats/qtga${QT_LIB_SUFFIX}.dll"  "${CMAKE_BINARY_DIR}/imageformats/qtga${QT_LIB_SUFFIX}.dll"  COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/imageformats/qwebp${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/imageformats/qwebp${QT_LIB_SUFFIX}.dll" COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/platforms/qwindows${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/platforms/qwindows${QT_LIB_SUFFIX}.dll" COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/styles/qwindowsvistastyle${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/styles/qwindowsvistastyle${QT_LIB_SUFFIX}.dll" COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/tls/qcertonlybackend${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/tls/qcertonlybackend${QT_LIB_SUFFIX}.dll" COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/tls/qschannelbackend${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/tls/qschannelbackend${QT_LIB_SUFFIX}.dll" COPYONLY)
    elseif(UNIX AND DEFINED QT_BASEDIR)
        configure_file("${QT_BASEDIR}/lib/libQt6Core.so.6"          "${CMAKE_BINARY_DIR}/libQt6Core.so.6"          COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6Gui.so.6"           "${CMAKE_BINARY_DIR}/libQt6Gui.so.6"           COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6Widgets.so.6"       "${CMAKE_BINARY_DIR}/libQt6Widgets.so.6"       COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6Network.so.6"       "${CMAKE_BINARY_DIR}/libQt6Network.so.6"       COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6OpenGL.so.6"        "${CMAKE_BINARY_DIR}/libQt6OpenGL.so.6"        COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6OpenGLWidgets.so.6" "${CMAKE_BINARY_DIR}/libQt6OpenGLWidgets.so.6" COPYONLY)

        # Required by plugins
        configure_file("${QT_BASEDIR}/lib/libicudata.so.56"                         "${CMAKE_BINARY_DIR}/libicudata.so.56"                         COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libicui18n.so.56"                         "${CMAKE_BINARY_DIR}/libicui18n.so.56"                         COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libicuuc.so.56"                           "${CMAKE_BINARY_DIR}/libicuuc.so.56"                           COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6DBus.so.6"                          "${CMAKE_BINARY_DIR}/libQt6DBus.so.6"                          COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6EglFSDeviceIntegration.so.6"        "${CMAKE_BINARY_DIR}/libQt6EglFSDeviceIntegration.so.6"        COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6EglFsKmsSupport.so.6"               "${CMAKE_BINARY_DIR}/libQt6EglFsKmsSupport.so.6"               COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6WaylandClient.so.6"                 "${CMAKE_BINARY_DIR}/libQt6WaylandClient.so.6"                 COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6WaylandEglClientHwIntegration.so.6" "${CMAKE_BINARY_DIR}/libQt6WaylandEglClientHwIntegration.so.6" COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6WlShellIntegration.so.6"            "${CMAKE_BINARY_DIR}/libQt6WlShellIntegration.so.6"            COPYONLY)
        configure_file("${QT_BASEDIR}/lib/libQt6XcbQpa.so.6"                        "${CMAKE_BINARY_DIR}/libQt6XcbQpa.so.6"                        COPYONLY)

        configure_file("${QT_BASEDIR}/plugins/imageformats/libqjpeg.so" "${CMAKE_BINARY_DIR}/imageformats/libqjpeg.so" COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/imageformats/libqtga.so"  "${CMAKE_BINARY_DIR}/imageformats/libqtga.so"  COPYONLY)
        configure_file("${QT_BASEDIR}/plugins/imageformats/libqwebp.so" "${CMAKE_BINARY_DIR}/imageformats/libqwebp.so" COPYONLY)

        # Copy all this stuff wholesale, who knows if we need it now or later
        file(GLOB ${PROJECT_NAME}_QT_PLUGINS_EGLDEVICEINTEGRATIONS               "${QT_BASEDIR}/plugins/egldeviceintegrations/*.so*")
        file(GLOB ${PROJECT_NAME}_QT_PLUGINS_PLATFORMINPUTCONTEXTS               "${QT_BASEDIR}/plugins/platforminputcontexts/*.so*")
        file(GLOB ${PROJECT_NAME}_QT_PLUGINS_PLATFORMS                           "${QT_BASEDIR}/plugins/platforms/*.so*")
        file(GLOB ${PROJECT_NAME}_QT_PLUGINS_PLATFORMTHEMES                      "${QT_BASEDIR}/plugins/platformthemes/*.so*")
        file(GLOB ${PROJECT_NAME}_QT_PLUGINS_TLS                                 "${QT_BASEDIR}/plugins/tls/*.so*")
        file(GLOB ${PROJECT_NAME}_QT_PLUGINS_WAYLANDDECORATIONCLIENT             "${QT_BASEDIR}/plugins/wayland-decoration-client/*.so*")
        file(GLOB ${PROJECT_NAME}_QT_PLUGINS_WAYLANDGRAPHICSINTEGRATIONCLIENT    "${QT_BASEDIR}/plugins/wayland-graphics-integration-client/*.so*")
        file(GLOB ${PROJECT_NAME}_QT_PLUGINS_WAYLANDSHELLINTEGRATION             "${QT_BASEDIR}/plugins/wayland-shell-integration/*.so*")
        file(GLOB ${PROJECT_NAME}_QT_PLUGINS_XCBGLINTEGRATIONS                   "${QT_BASEDIR}/plugins/xcbglintegrations/*.so*")
        file(COPY ${${PROJECT_NAME}_QT_PLUGINS_EGLDEVICEINTEGRATIONS}            DESTINATION "${CMAKE_BINARY_DIR}/egldeviceintegrations")
        file(COPY ${${PROJECT_NAME}_QT_PLUGINS_PLATFORMINPUTCONTEXTS}            DESTINATION "${CMAKE_BINARY_DIR}/platforminputcontexts")
        file(COPY ${${PROJECT_NAME}_QT_PLUGINS_PLATFORMS}                        DESTINATION "${CMAKE_BINARY_DIR}/platforms")
        file(COPY ${${PROJECT_NAME}_QT_PLUGINS_PLATFORMTHEMES}                   DESTINATION "${CMAKE_BINARY_DIR}/platformthemes")
        file(COPY ${${PROJECT_NAME}_QT_PLUGINS_TLS}                              DESTINATION "${CMAKE_BINARY_DIR}/tls")
        file(COPY ${${PROJECT_NAME}_QT_PLUGINS_WAYLANDDECORATIONCLIENT}          DESTINATION "${CMAKE_BINARY_DIR}/wayland-decoration-client")
        file(COPY ${${PROJECT_NAME}_QT_PLUGINS_WAYLANDGRAPHICSINTEGRATIONCLIENT} DESTINATION "${CMAKE_BINARY_DIR}/wayland-graphics-integration-client")
        file(COPY ${${PROJECT_NAME}_QT_PLUGINS_WAYLANDSHELLINTEGRATION}          DESTINATION "${CMAKE_BINARY_DIR}/wayland-shell-integration")
        file(COPY ${${PROJECT_NAME}_QT_PLUGINS_XCBGLINTEGRATIONS}                DESTINATION "${CMAKE_BINARY_DIR}/xcbglintegrations")
    endif()

    # Set up install rules
    install(TARGETS ${PROJECT_NAME} vtflib
            CONFIGURATIONS ${CMAKE_BUILD_TYPE}
            RUNTIME DESTINATION .
            LIBRARY DESTINATION .)
    install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/installer/.nonportable"
            DESTINATION .)
    if(WIN32)
        install(IMPORTED_RUNTIME_ARTIFACTS
                Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets
                RUNTIME DESTINATION .
                LIBRARY DESTINATION .)

        install(FILES
                "${QT_BASEDIR}/plugins/imageformats/qjpeg${QT_LIB_SUFFIX}.dll"
                "${QT_BASEDIR}/plugins/imageformats/qtga${QT_LIB_SUFFIX}.dll"
                "${QT_BASEDIR}/plugins/imageformats/qwebp${QT_LIB_SUFFIX}.dll"
                DESTINATION imageformats)

        install(FILES "${QT_BASEDIR}/plugins/platforms/qwindows${QT_LIB_SUFFIX}.dll"
                DESTINATION platforms)

        install(FILES "${QT_BASEDIR}/plugins/styles/qwindowsvistastyle${QT_LIB_SUFFIX}.dll"
                DESTINATION styles)

        install(FILES
                "${QT_BASEDIR}/plugins/tls/qcertonlybackend${QT_LIB_SUFFIX}.dll"
                "${QT_BASEDIR}/plugins/tls/qschannelbackend${QT_LIB_SUFFIX}.dll"
                DESTINATION tls)
    elseif(UNIX)
        if (DEFINED QT_BASEDIR)
            # If this is a custom install, we've copied the Qt libraries to the build directory and done special fixups
            install(FILES
                    "${CMAKE_BINARY_DIR}/libQt6Core.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6Gui.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6Widgets.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6Network.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6OpenGL.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6OpenGLWidgets.so.6"

                    # Required by plugins
                    "${CMAKE_BINARY_DIR}/libicudata.so.56"
                    "${CMAKE_BINARY_DIR}/libicui18n.so.56"
                    "${CMAKE_BINARY_DIR}/libicuuc.so.56"
                    "${CMAKE_BINARY_DIR}/libQt6DBus.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6EglFSDeviceIntegration.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6EglFsKmsSupport.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6WaylandClient.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6WaylandEglClientHwIntegration.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6WlShellIntegration.so.6"
                    "${CMAKE_BINARY_DIR}/libQt6XcbQpa.so.6"
                    DESTINATION .)

            install(DIRECTORY
                    "${CMAKE_BINARY_DIR}/egldeviceintegrations"
                    "${CMAKE_BINARY_DIR}/imageformats"
                    "${CMAKE_BINARY_DIR}/platforminputcontexts"
                    "${CMAKE_BINARY_DIR}/platforms"
                    "${CMAKE_BINARY_DIR}/platformthemes"
                    "${CMAKE_BINARY_DIR}/tls"
                    "${CMAKE_BINARY_DIR}/wayland-decoration-client"
                    "${CMAKE_BINARY_DIR}/wayland-graphics-integration-client"
                    "${CMAKE_BINARY_DIR}/wayland-shell-integration"
                    "${CMAKE_BINARY_DIR}/xcbglintegrations"
                    DESTINATION .)
        else()
            install(IMPORTED_RUNTIME_ARTIFACTS
                    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets
                    RUNTIME DESTINATION .
                    LIBRARY DESTINATION .)
        endif()

        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/installer/deb/${PROJECT_NAME}.desktop"
                DESTINATION "/usr/share/applications/")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/res/icon-128.png"
                DESTINATION "/usr/share/pixmaps/"
                RENAME "${PROJECT_NAME}.png")
    endif()

    if(VPKEDIT_BUILD_INSTALLER)
        set(CPACK_PACKAGE_NAME ${PROJECT_NAME_PRETTY})
        set(CPACK_PACKAGE_VENDOR ${PROJECT_ORGANIZATION_NAME})
        set(CPACK_PACKAGE_CONTACT "lauralewisdev@gmail.com")
        set(CPACK_PACKAGE_DESCRIPTION ${CMAKE_PROJECT_DESCRIPTION})
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_DESCRIPTION})
        set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
        set(CPACK_MONOLITHIC_INSTALL ON)
        set(CPACK_PACKAGE_EXECUTABLES ${PROJECT_NAME} ${PROJECT_NAME_PRETTY})
        if(WIN32)
            set(CPACK_GENERATOR "NSIS")
            set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
            set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
            set(CPACK_NSIS_DISPLAY_NAME ${PROJECT_NAME_PRETTY})
            set(CPACK_NSIS_PACKAGE_NAME ${PROJECT_NAME_PRETTY})
            set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/res/icon.ico")
            set(CPACK_NSIS_INSTALLED_ICON_NAME "${PROJECT_NAME}.exe")
            set(CPACK_NSIS_URL_INFO_ABOUT "${CMAKE_PROJECT_HOMEPAGE_URL}")
            set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
            set(HELP_QUOTE "\"") # CMake is shit
            set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
                WriteRegStr HKCR '.vpk' '' '${PROJECT_NAME_PRETTY}'
                WriteRegStr HKCR '${PROJECT_NAME_PRETTY}' '' 'Valve Pack File'
                WriteRegStr HKCR '${PROJECT_NAME_PRETTY}\\\\shell' '' 'open'
                WriteRegStr HKCR '${PROJECT_NAME_PRETTY}\\\\DefaultIcon' '' '$INSTDIR\\\\${PROJECT_NAME}.exe,0'
                WriteRegStr HKCR '${PROJECT_NAME_PRETTY}\\\\shell\\\\open\\\\command' '' '$INSTDIR\\\\${PROJECT_NAME}.exe \\${HELP_QUOTE}%1\\${HELP_QUOTE}'
                WriteRegStr HKCR '${PROJECT_NAME_PRETTY}\\\\shell\\\\edit' '' 'Browse VPK'
                WriteRegStr HKCR '${PROJECT_NAME_PRETTY}\\\\shell\\\\edit\\\\command' '' '$INSTDIR\\\\${PROJECT_NAME}.exe \\${HELP_QUOTE}%1\\${HELP_QUOTE}'
                System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
            ")
            set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
                DeleteRegKey HKCR '.vpk'
                DeleteRegKey HKCR '${PROJECT_NAME_PRETTY}'
            ")
        else()
            set(CPACK_GENERATOR "DEB")
            set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <${CPACK_PACKAGE_CONTACT}>")
            set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${PROJECT_NAME}")
        endif()
        include(CPack)
    endif()
endif()

if(VPKEDIT_BUILD_TESTS)
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
