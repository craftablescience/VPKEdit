# discord-rpc
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/discord")

# miniaudio
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/miniaudio")

# Qt
if(WIN32 AND NOT DEFINED QT_BASEDIR)
    message(FATAL_ERROR "Please define your Qt install dir with -DQT_BASEDIR=\"C:/your/qt6/here\"")
endif()

if(DEFINED QT_BASEDIR)
    string(REPLACE "\\" "/" QT_BASEDIR "${QT_BASEDIR}")

    # Add it to the prefix path so find_package can find it
    list(APPEND CMAKE_PREFIX_PATH "${QT_BASEDIR}")
    set(QT_INCLUDE "${QT_BASEDIR}/include")
    message(STATUS "Using ${QT_INCLUDE} as the Qt include directory")
endif()

# CMake has an odd policy that links a special link lib for Qt on newer versions of CMake
cmake_policy(SET CMP0020 NEW)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network OpenGL OpenGLWidgets LinguistTools Svg)

get_target_property(QT_QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS OUTPUT_VARIABLE QT_TRANSLATIONS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
file(GLOB QT_I18N_QM_FILES "${QT_TRANSLATIONS_DIR}/qt_*.qm" "${QT_TRANSLATIONS_DIR}/qtbase_*.qm")

function(target_use_qt TARGET)
    set_target_properties(${TARGET} PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)
    target_link_libraries(${TARGET} PRIVATE
            Qt::Core
            Qt::Gui
            Qt::Widgets
            Qt::Network
            Qt::OpenGL
            Qt::OpenGLWidgets
            Qt::Svg)
    target_include_directories(${TARGET} PRIVATE
            "${QT_INCLUDE}"
            "${QT_INCLUDE}/QtCore"
            "${QT_INCLUDE}/QtGui"
            "${QT_INCLUDE}/QtWidgets"
            "${QT_INCLUDE}/QtNetwork"
            "${QT_INCLUDE}/QtOpenGL"
            "${QT_INCLUDE}/QtOpenGLWidgets"
            "${QT_INCLUDE}/QtSvg")
    qt_add_resources(${TARGET} "${TARGET}_qt_translations"
            BASE "${QT_TRANSLATIONS_DIR}"
            PREFIX "/i18n"
            BIG_RESOURCES
            FILES ${QT_I18N_QM_FILES})
endfunction()

# Copy these in
if(WIN32)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(QT_LIB_SUFFIX "d" CACHE STRING "" FORCE)
    else()
        set(QT_LIB_SUFFIX "" CACHE STRING "" FORCE)
    endif()

    configure_file("${QT_BASEDIR}/bin/opengl32sw.dll"                       "${CMAKE_BINARY_DIR}/opengl32sw.dll"                       COPYONLY)
    configure_file("${QT_BASEDIR}/bin/Qt6Core${QT_LIB_SUFFIX}.dll"          "${CMAKE_BINARY_DIR}/Qt6Core${QT_LIB_SUFFIX}.dll"          COPYONLY)
    configure_file("${QT_BASEDIR}/bin/Qt6Gui${QT_LIB_SUFFIX}.dll"           "${CMAKE_BINARY_DIR}/Qt6Gui${QT_LIB_SUFFIX}.dll"           COPYONLY)
    configure_file("${QT_BASEDIR}/bin/Qt6Widgets${QT_LIB_SUFFIX}.dll"       "${CMAKE_BINARY_DIR}/Qt6Widgets${QT_LIB_SUFFIX}.dll"       COPYONLY)
    configure_file("${QT_BASEDIR}/bin/Qt6Network${QT_LIB_SUFFIX}.dll"       "${CMAKE_BINARY_DIR}/Qt6Network${QT_LIB_SUFFIX}.dll"       COPYONLY)
    configure_file("${QT_BASEDIR}/bin/Qt6OpenGL${QT_LIB_SUFFIX}.dll"        "${CMAKE_BINARY_DIR}/Qt6OpenGL${QT_LIB_SUFFIX}.dll"        COPYONLY)
    configure_file("${QT_BASEDIR}/bin/Qt6OpenGLWidgets${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/Qt6OpenGLWidgets${QT_LIB_SUFFIX}.dll" COPYONLY)
    configure_file("${QT_BASEDIR}/bin/Qt6Svg${QT_LIB_SUFFIX}.dll"           "${CMAKE_BINARY_DIR}/Qt6Svg${QT_LIB_SUFFIX}.dll" COPYONLY)

    configure_file("${QT_BASEDIR}/plugins/platforms/qwindows${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/platforms/qwindows${QT_LIB_SUFFIX}.dll" COPYONLY)

    if(EXISTS "${QT_BASEDIR}/plugins/styles/qmodernwindowsstyle${QT_LIB_SUFFIX}.dll")
        configure_file("${QT_BASEDIR}/plugins/styles/qmodernwindowsstyle${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/styles/qmodernwindowsstyle${QT_LIB_SUFFIX}.dll" COPYONLY)
    else()
        configure_file("${QT_BASEDIR}/plugins/styles/qwindowsvistastyle${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/styles/qwindowsvistastyle${QT_LIB_SUFFIX}.dll" COPYONLY)
    endif()

    configure_file("${QT_BASEDIR}/plugins/tls/qcertonlybackend${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/tls/qcertonlybackend${QT_LIB_SUFFIX}.dll" COPYONLY)
    configure_file("${QT_BASEDIR}/plugins/tls/qschannelbackend${QT_LIB_SUFFIX}.dll" "${CMAKE_BINARY_DIR}/tls/qschannelbackend${QT_LIB_SUFFIX}.dll" COPYONLY)
elseif(UNIX AND DEFINED QT_BASEDIR)
    configure_file("${QT_BASEDIR}/lib/libQt6Core.so.6"          "${CMAKE_BINARY_DIR}/libQt6Core.so.6"          COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6Gui.so.6"           "${CMAKE_BINARY_DIR}/libQt6Gui.so.6"           COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6Widgets.so.6"       "${CMAKE_BINARY_DIR}/libQt6Widgets.so.6"       COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6Network.so.6"       "${CMAKE_BINARY_DIR}/libQt6Network.so.6"       COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6OpenGL.so.6"        "${CMAKE_BINARY_DIR}/libQt6OpenGL.so.6"        COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6OpenGLWidgets.so.6" "${CMAKE_BINARY_DIR}/libQt6OpenGLWidgets.so.6" COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6Svg.so.6"           "${CMAKE_BINARY_DIR}/libQt6Svg.so.6"           COPYONLY)

    # These end in a lot of different numbers depending on the Qt version
    function(copy_latest_icu_lib NAME)
        file(GLOB LIBS "${QT_BASEDIR}/lib/libicu${NAME}.so.[0-9]+$")
        if(LIBS)
            list(SORT LIBS)
            list(REVERSE LIBS)
            list(GET LIBS 0 LIB)
            cmake_path(GET "${LIB}" FILENAME LIB_FILENAME)
            configure_file("${LIB}" "${CMAKE_BINARY_DIR}/${LIB_FILENAME}" COPYONLY)
        endif()
    endfunction()
    copy_latest_icu_lib("data")
    copy_latest_icu_lib("i18n")
    copy_latest_icu_lib("uc")

    # Required by plugins
    configure_file("${QT_BASEDIR}/lib/libQt6DBus.so.6"                          "${CMAKE_BINARY_DIR}/libQt6DBus.so.6"                          COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6EglFSDeviceIntegration.so.6"        "${CMAKE_BINARY_DIR}/libQt6EglFSDeviceIntegration.so.6"        COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6EglFsKmsSupport.so.6"               "${CMAKE_BINARY_DIR}/libQt6EglFsKmsSupport.so.6"               COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6WaylandClient.so.6"                 "${CMAKE_BINARY_DIR}/libQt6WaylandClient.so.6"                 COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6WaylandEglClientHwIntegration.so.6" "${CMAKE_BINARY_DIR}/libQt6WaylandEglClientHwIntegration.so.6" COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6WlShellIntegration.so.6"            "${CMAKE_BINARY_DIR}/libQt6WlShellIntegration.so.6"            COPYONLY)
    configure_file("${QT_BASEDIR}/lib/libQt6XcbQpa.so.6"                        "${CMAKE_BINARY_DIR}/libQt6XcbQpa.so.6"                        COPYONLY)

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
