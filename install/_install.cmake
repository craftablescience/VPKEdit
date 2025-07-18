# Set up install rules
if(WIN32)
    install(TARGETS ${PROJECT_NAME}cli
            DESTINATION .)

    install(TARGETS ${PROJECT_NAME}
            DESTINATION .)

    install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            DESTINATION .)

    install(IMPORTED_RUNTIME_ARTIFACTS
            Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg
            RUNTIME DESTINATION .
            LIBRARY DESTINATION .)

    install(FILES "${QT_BASEDIR}/bin/opengl32sw.dll"
            DESTINATION .)

    install(FILES "${QT_BASEDIR}/plugins/platforms/qwindows${QT_LIB_SUFFIX}.dll"
            DESTINATION platforms)

    if(EXISTS "${QT_BASEDIR}/plugins/styles/qmodernwindowsstyle${QT_LIB_SUFFIX}.dll")
        install(FILES "${QT_BASEDIR}/plugins/styles/qmodernwindowsstyle${QT_LIB_SUFFIX}.dll"
                DESTINATION styles)
    else()
        install(FILES "${QT_BASEDIR}/plugins/styles/qwindowsvistastyle${QT_LIB_SUFFIX}.dll"
                DESTINATION styles)
    endif()

    install(FILES
            "${QT_BASEDIR}/plugins/tls/qcertonlybackend${QT_LIB_SUFFIX}.dll"
            "${QT_BASEDIR}/plugins/tls/qschannelbackend${QT_LIB_SUFFIX}.dll"
            DESTINATION tls)

    # NSIS install commands
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/win/InstallCommands.nsh.in"
            "${CMAKE_CURRENT_LIST_DIR}/win/generated/InstallCommands.nsh"
            @ONLY)

    # NSIS uninstall commands
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/win/UninstallCommands.nsh.in"
            "${CMAKE_CURRENT_LIST_DIR}/win/generated/UninstallCommands.nsh"
            @ONLY)
elseif(UNIX)
    install(TARGETS ${PROJECT_NAME}cli
            DESTINATION bin)

    install(TARGETS ${PROJECT_NAME}
            DESTINATION bin)

    install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            DESTINATION "share/licenses/${PROJECT_NAME}")

    # Use system Qt - no install rules

    # Desktop file
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/linux/desktop.in"
            "${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME}.desktop")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME}.desktop"
            DESTINATION "share/applications")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/brand/logo$<$<CONFIG:Debug>:_alt>_512.png"
            DESTINATION "share/pixmaps"
            RENAME "${PROJECT_NAME}.png")

    # MIME type info
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/linux/mime-type.xml.in"
            "${CMAKE_CURRENT_LIST_DIR}/linux/generated/mime-type.xml")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/generated/mime-type.xml"
            DESTINATION "share/mime/packages"
            RENAME "${PROJECT_NAME}.xml")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/brand/logo$<$<CONFIG:Debug>:_alt>_128.png"
            DESTINATION "share/icons/hicolor/128x128/mimetypes"
            RENAME "application-x-vpkedit.png")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/brand/logo$<$<CONFIG:Debug>:_alt>_512.png"
            DESTINATION "share/icons/hicolor/512x512/mimetypes"
            RENAME "application-x-vpkedit.png")
else()
    message(FATAL_ERROR "No install rules for selected platform.")
endif()

# CPack stuff
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
set(CPACK_STRIP_FILES ON)
set(CPACK_THREADS 0)
set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
if(WIN32)
    if(NOT (CPACK_GENERATOR STREQUAL "NSIS"))
        message(WARNING "CPack generator must be NSIS! Setting generator to NSIS...")
        set(CPACK_GENERATOR "NSIS" CACHE INTERNAL "" FORCE)
    endif()
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_DISPLAY_NAME ${PROJECT_NAME_PRETTY})
    set(CPACK_NSIS_PACKAGE_NAME ${PROJECT_NAME_PRETTY})
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/logo.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${PROJECT_NAME}.exe")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CMAKE_PROJECT_HOMEPAGE_URL}")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/win/generated/InstallCommands.nsh"   CPACK_NSIS_EXTRA_INSTALL_COMMANDS)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/win/generated/UninstallCommands.nsh" CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/win") # NSIS.template.in, NSIS.InstallOptions.ini.in
else()
    if(CPACK_GENERATOR STREQUAL "DEB")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <${CPACK_PACKAGE_CONTACT}>")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxcb-cursor0, libqt6core6 >= 6.4.2, libqt6dbus6 >= 6.4.2, libqt6gui6 >= 6.4.2, libqt6widgets6 >= 6.4.2, libqt6network6 >= 6.4.2, libqt6opengl6 >= 6.4.2, libqt6openglwidgets6 >= 6.4.2, libqt6svg6 >= 6.4.2")
        set(CPACK_DEBIAN_COMPRESSION_TYPE "zstd")
    elseif(CPACK_GENERATOR STREQUAL "RPM")
        set(CPACK_RPM_PACKAGE_LICENSE "MIT")
        set(CPACK_RPM_PACKAGE_REQUIRES "libxcb >= 1.17, qt6-qtbase >= 6.4.2, qt6-qtbase-gui >= 6.4.2, qt6-qtsvg >= 6.4.2")
        if(CMAKE_VERSION VERSION_LESS "3.31")
            set(CPACK_RPM_COMPRESSION_TYPE "xz")
        else()
            set(CPACK_RPM_COMPRESSION_TYPE "zstd")
        endif()
    else()
        message(FATAL_ERROR "CPACK_GENERATOR is unset! Only DEB and RPM generators are supported.")
    endif()
endif()
include(CPack)
