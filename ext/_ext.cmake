# argparse
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/argparse")

# discord
if(VPKEDIT_BUILD_GUI)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/discord")
endif()

# libsndfile
set(BUILD_PROGRAMS OFF CACHE INTERNAL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libsndfile")

# sdl
set(SDL_UNIX_CONSOLE_BUILD    ON CACHE INTERNAL "" FORCE)
set(SDL_AUDIO                 ON CACHE INTERNAL "" FORCE)
set(SDL_VIDEO_DEFAULT        OFF CACHE INTERNAL "" FORCE)
set(SDL_GPU_DEFAULT          OFF CACHE INTERNAL "" FORCE)
set(SDL_RENDER_DEFAULT       OFF CACHE INTERNAL "" FORCE)
set(SDL_CAMERA_DEFAULT       OFF CACHE INTERNAL "" FORCE)
set(SDL_JOYSTICK_DEFAULT     OFF CACHE INTERNAL "" FORCE)
set(SDL_HAPTIC_DEFAULT       OFF CACHE INTERNAL "" FORCE)
set(SDL_HIDAPI_DEFAULT       OFF CACHE INTERNAL "" FORCE)
set(SDL_POWER_DEFAULT        OFF CACHE INTERNAL "" FORCE)
set(SDL_SENSOR_DEFAULT       OFF CACHE INTERNAL "" FORCE)
set(SDL_DIALOG_DEFAULT       OFF CACHE INTERNAL "" FORCE)
set(SDL_TRAY_DEFAULT         OFF CACHE INTERNAL "" FORCE)
set(SDL_NOTIFICATION_DEFAULT OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/sdl")

# sourcepp
set(SOURCEPP_LIBS_START_ENABLED OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_BSPPP           ON CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_SNDPP           ON CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_VPKPP           ON CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_VTFPP           ON CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/sourcepp")

# Qt
if(VPKEDIT_BUILD_GUI OR VPKEDIT_BUILD_GUI_PLUGINS)
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

    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network OpenGL OpenGLWidgets LinguistTools)

    get_target_property(QT_QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
    execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_VERSION              OUTPUT_VARIABLE QT_VERSION          OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS OUTPUT_VARIABLE QT_TRANSLATIONS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

    file(GLOB QT_I18N_QM_FILES "${QT_TRANSLATIONS_DIR}/qtbase_*.qm" "${QT_TRANSLATIONS_DIR}/qt_*.qm")
    if (QT_VERSION VERSION_GREATER_EQUAL "6.7.0")
        set_property(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/sourcepp" PROPERTY QT_EXCLUDE_FROM_TRANSLATION ON)
    endif()

    function(target_use_qt TARGET)
        set_target_properties(${TARGET} PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)
        target_link_libraries(${TARGET} PRIVATE Qt::Core Qt::Gui Qt::Widgets Qt::Network Qt::OpenGL Qt::OpenGLWidgets)
        target_include_directories(${TARGET} PRIVATE "${QT_INCLUDE}" "${QT_INCLUDE}/QtCore" "${QT_INCLUDE}/QtGui" "${QT_INCLUDE}/QtWidgets" "${QT_INCLUDE}/QtNetwork" "${QT_INCLUDE}/QtOpenGL" "${QT_INCLUDE}/QtOpenGLWidgets")
        qt_add_resources(${TARGET} "${TARGET}_qt_translations" BASE "${QT_TRANSLATIONS_DIR}" PREFIX "/i18n" FILES ${QT_I18N_QM_FILES})

        # Change RPATH to target lib folder when shipping Qt
        if(NOT VPKEDIT_SYSTEM_INSTALL AND DEFINED QT_BASEDIR)
            set_target_properties(${TARGET} PROPERTIES BUILD_RPATH "$ORIGIN/lib" INSTALL_RPATH "$ORIGIN/lib")
        endif()
    endfunction()

    # Copy these in
    list(APPEND QT_REQUIRED_ROOT_LIBS
            # Core
            "Core" "Gui" "Widgets" "Network" "OpenGL" "OpenGLWidgets"
            # Needed by plugins
            "DBus" "EglFSDeviceIntegration" "EglFsKmsSupport" "EglFsKmsGbmSupport" "WaylandClient" "WaylandEglClientHwIntegration" "WlShellIntegration" "XcbQpa")
    list(APPEND QT_REQUIRED_PLUGIN_DIRS
            "egldeviceintegrations" "networkinformation" "platforminputcontexts" "platforms" "platformthemes" "styles" "tls"
            "wayland-decoration-client" "wayland-graphics-integration-client" "wayland-shell-integration" "xcbglintegrations")

    if(WIN32)
        set(QT_LIB_SUFFIX_DEBUG "d")
        set(QT_LIB_SUFFIX_RELEASE "")
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            set(QT_LIB_SUFFIX "${QT_LIB_SUFFIX_DEBUG}" CACHE STRING "" FORCE)
        else()
            set(QT_LIB_SUFFIX "${QT_LIB_SUFFIX_RELEASE}" CACHE STRING "" FORCE)
        endif()

        if(EXISTS "${QT_BASEDIR}/bin/opengl32sw.dll")
            file(COPY "${QT_BASEDIR}/bin/opengl32sw.dll" DESTINATION "${CMAKE_BINARY_DIR}")
        endif()

        foreach(QT_REQUIRED_ROOT_LIB ${QT_REQUIRED_ROOT_LIBS})
            if(EXISTS "${QT_BASEDIR}/bin/Qt6${QT_REQUIRED_ROOT_LIB}${QT_LIB_SUFFIX}.dll")
                file(COPY "${QT_BASEDIR}/bin/Qt6${QT_REQUIRED_ROOT_LIB}${QT_LIB_SUFFIX}.dll" DESTINATION "${CMAKE_BINARY_DIR}")
            endif()
        endforeach()

        foreach(QT_REQUIRED_PLUGIN_DIR ${QT_REQUIRED_PLUGIN_DIRS})
            if(EXISTS "${QT_BASEDIR}/plugins/${QT_REQUIRED_PLUGIN_DIR}")
                file(GLOB QT_REQUIRED_PLUGIN_DLLS "${QT_BASEDIR}/plugins/${QT_REQUIRED_PLUGIN_DIR}/*.dll")
                foreach(QT_REQUIRED_PLUGIN_DLL ${QT_REQUIRED_PLUGIN_DLLS})
                    set(QT_REQUIRED_PLUGIN_DLL_MODIFIED "${QT_REQUIRED_PLUGIN_DLL}")
                    if(CMAKE_BUILD_TYPE MATCHES Debug)
                        cmake_path(GET QT_REQUIRED_PLUGIN_DLL STEM QT_REQUIRED_PLUGIN_DLL_STEM)
                        cmake_path(REPLACE_FILENAME QT_REQUIRED_PLUGIN_DLL_MODIFIED "${QT_REQUIRED_PLUGIN_DLL_STEM}${QT_LIB_SUFFIX_DEBUG}.dll")
                        if(EXISTS "${QT_REQUIRED_PLUGIN_DLL_MODIFIED}")
                            continue()
                        endif()
                    else()
                        cmake_path(GET QT_REQUIRED_PLUGIN_DLL STEM QT_REQUIRED_PLUGIN_DLL_STEM)
                        string(REGEX REPLACE "${QT_LIB_SUFFIX_DEBUG}$" "" QT_REQUIRED_PLUGIN_DLL_STEM_REL "${QT_REQUIRED_PLUGIN_DLL_STEM}")
                        cmake_path(REPLACE_FILENAME QT_REQUIRED_PLUGIN_DLL_MODIFIED "${QT_REQUIRED_PLUGIN_DLL_STEM_REL}${QT_LIB_SUFFIX}.dll")
                        if(NOT ("${QT_REQUIRED_PLUGIN_DLL}" STREQUAL "${QT_REQUIRED_PLUGIN_DLL_MODIFIED}") AND EXISTS "${QT_REQUIRED_PLUGIN_DLL_MODIFIED}")
                            continue()
                        endif()
                    endif()
                    file(COPY "${QT_REQUIRED_PLUGIN_DLL}" DESTINATION "${CMAKE_BINARY_DIR}/${QT_REQUIRED_PLUGIN_DIR}")
                endforeach()
            endif()
        endforeach()
    elseif(UNIX AND DEFINED QT_BASEDIR)
        foreach(QT_REQUIRED_ROOT_LIB ${QT_REQUIRED_ROOT_LIBS})
            if(EXISTS "${QT_BASEDIR}/lib/libQt6${QT_REQUIRED_ROOT_LIB}.so")
                file(COPY "${QT_BASEDIR}/lib/libQt6${QT_REQUIRED_ROOT_LIB}.so" DESTINATION "${CMAKE_BINARY_DIR}/lib" FOLLOW_SYMLINK_CHAIN)
            endif()
        endforeach()

        foreach(QT_REQUIRED_PLUGIN_DIR ${QT_REQUIRED_PLUGIN_DIRS})
            if(EXISTS "${QT_BASEDIR}/plugins/${QT_REQUIRED_PLUGIN_DIR}")
                file(COPY "${QT_BASEDIR}/plugins/${QT_REQUIRED_PLUGIN_DIR}" DESTINATION "${CMAKE_BINARY_DIR}/plugins")
            endif()
        endforeach()

        # These end in a lot of different numbers depending on the Qt version
        function(copy_latest_icu_lib NAME)
            file(GLOB QT_ICU_LIBS "${QT_BASEDIR}/lib/libicu${NAME}.so.[0-9][0-9]" "${QT_BASEDIR}/lib/libicu${NAME}.so.[0-9][0-9][0-9]")
            if(QT_ICU_LIBS)
                list(SORT QT_ICU_LIBS)
                list(REVERSE QT_ICU_LIBS)
                list(GET QT_ICU_LIBS 0 QT_ICU_LIB)
                cmake_path(GET QT_ICU_LIB FILENAME QT_ICU_LIB_FILENAME)
                file(COPY "${QT_ICU_LIB}" DESTINATION "${CMAKE_BINARY_DIR}/lib" FOLLOW_SYMLINK_CHAIN)
            endif()
        endfunction()
        copy_latest_icu_lib("data")
        copy_latest_icu_lib("i18n")
        copy_latest_icu_lib("io")
        copy_latest_icu_lib("tu")
        copy_latest_icu_lib("uc")
    endif()
endif()
