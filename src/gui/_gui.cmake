set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH $ORIGIN)

# VTFLib
set(VTFLIB_STATIC OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/vtflib")
set_target_properties(
        vtflib PROPERTIES
        # I don't know which one of these puts it next to the executable so let's do all of them!
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# sourcepp (various parsing libraries)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/sourcepp")

# SpeedyKeyV (for SAPP)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/speedykeyv")

# SAPP
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/sapp")

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

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network OpenGL OpenGLWidgets LinguistTools)

# Add sources and create executable
list(APPEND ${PROJECT_NAME}_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/config/Options.h"
        "${CMAKE_CURRENT_LIST_DIR}/config/Options.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/dialogs/ControlsDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/ControlsDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/EntryOptionsDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/EntryOptionsDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/NewUpdateDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/NewUpdateDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/NewVPKOptionsDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/NewVPKOptionsDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/PackFileOptionsDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/PackFileOptionsDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VerifyChecksumsDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VerifyChecksumsDialog.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/previews/AudioPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/AudioPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/DirPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/DirPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/EmptyPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/EmptyPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/ImagePreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/ImagePreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/InfoPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/InfoPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/MDLPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/MDLPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/TextPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/TextPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/VTFPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/VTFPreview.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/res/res.qrc"

        "${CMAKE_CURRENT_LIST_DIR}/utility/AudioPlayer.h"
        "${CMAKE_CURRENT_LIST_DIR}/utility/AudioPlayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/ThemedIcon.h"
        "${CMAKE_CURRENT_LIST_DIR}/utility/ThemedIcon.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/VTFDecoder.h"
        "${CMAKE_CURRENT_LIST_DIR}/utility/VTFDecoder.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/EntryTree.h"
        "${CMAKE_CURRENT_LIST_DIR}/EntryTree.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/FileViewer.h"
        "${CMAKE_CURRENT_LIST_DIR}/FileViewer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Window.h"
        "${CMAKE_CURRENT_LIST_DIR}/Window.cpp")
if(WIN32)
    list(APPEND ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_LIST_DIR}/res/icon.rc")
endif()

add_executable(${PROJECT_NAME} WIN32 ${${PROJECT_NAME}_SOURCES})

qt_add_translations(${PROJECT_NAME}
        TS_FILES "${CMAKE_CURRENT_LIST_DIR}/res/i18n/${PROJECT_NAME}.en.ts"
        TS_FILES "${CMAKE_CURRENT_LIST_DIR}/res/i18n/${PROJECT_NAME}.ru.ts"
        SOURCES ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE lib${PROJECT_NAME} ${CMAKE_DL_LIBS} vtflib dmxpp studiomodelpp keyvalues SAPP Qt::Core Qt::Gui Qt::Widgets Qt::Network Qt::OpenGL Qt::OpenGLWidgets)
target_include_directories(
        ${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/thirdparty/miniaudio"
        "${CMAKE_CURRENT_LIST_DIR}/thirdparty/sapp/include"
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

# Create PDBs in release
if(WIN32)
    target_compile_options(
            ${PROJECT_NAME} PRIVATE
            "$<$<CONFIG:Release>:/Zi>")
    target_link_options(
            ${PROJECT_NAME} PRIVATE
            "$<$<CONFIG:Release>:/DEBUG>"
            "$<$<CONFIG:Release>:/OPT:REF>"
            "$<$<CONFIG:Release>:/OPT:ICF>")
endif()

# Copy these next to the executable
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md" "${CMAKE_BINARY_DIR}/CREDITS.md" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"    "${CMAKE_BINARY_DIR}/LICENSE"    COPYONLY)
# Don't copy the .nonportable file, we're debugging in standalone mode

# Copy these so the user doesn't have to
if(WIN32)
    configure_file("${QT_BASEDIR}/bin/opengl32sw.dll"                       "${CMAKE_BINARY_DIR}/opengl32sw.dll"                       COPYONLY)
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
