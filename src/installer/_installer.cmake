# Set up install rules
install(TARGETS ${PROJECT_NAME}cli
        DESTINATION .)

install(TARGETS ${PROJECT_NAME}
        DESTINATION .)

install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_LIST_DIR}/.nonportable"
        DESTINATION .)

foreach(${PROJECT_NAME}_QTBASE_TRANSLATION IN LISTS ${PROJECT_NAME}_QTBASE_TRANSLATIONS)
    install(FILES "${${PROJECT_NAME}_QTBASE_TRANSLATION}"
            DESTINATION i18n)
endforeach()

if(WIN32)
    install(IMPORTED_RUNTIME_ARTIFACTS
            Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg
            RUNTIME DESTINATION .
            LIBRARY DESTINATION .)

    install(FILES "${QT_BASEDIR}/bin/opengl32sw.dll"
            DESTINATION .)

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
    if (DEFINED QT_BASEDIR)
        # If this is a custom install, we've copied the Qt libraries to the build directory and done special fixups
        install(FILES
                "${CMAKE_BINARY_DIR}/libQt6Core.so.6"
                "${CMAKE_BINARY_DIR}/libQt6Gui.so.6"
                "${CMAKE_BINARY_DIR}/libQt6Widgets.so.6"
                "${CMAKE_BINARY_DIR}/libQt6Network.so.6"
                "${CMAKE_BINARY_DIR}/libQt6OpenGL.so.6"
                "${CMAKE_BINARY_DIR}/libQt6OpenGLWidgets.so.6"
                "${CMAKE_BINARY_DIR}/libQt6Svg.so.6"

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
                Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Svg
                RUNTIME DESTINATION .
                LIBRARY DESTINATION .)
    endif()

    # Desktop file
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/linux/desktop.in"
            "${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME}.desktop")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME}.desktop"
            DESTINATION "/usr/share/applications/")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/branding/logo.png"
            DESTINATION "/usr/share/pixmaps/"
            RENAME "${PROJECT_NAME}.png")

    # MIME type info
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/linux/mime-type.xml.in"
            "${CMAKE_CURRENT_LIST_DIR}/linux/generated/mime-type.xml")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/generated/mime-type.xml"
            DESTINATION "/usr/share/mime/packages/"
            RENAME "${PROJECT_NAME}.xml")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/branding/logo.png"
            DESTINATION "/usr/share/icons/hicolor/128x128/mimetypes/"
            RENAME "application-x-vpkedit.png")
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
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_LIST_DIR}/../shared/res/logo.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${PROJECT_NAME}.exe")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CMAKE_PROJECT_HOMEPAGE_URL}")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/win/generated/InstallCommands.nsh"   CPACK_NSIS_EXTRA_INSTALL_COMMANDS)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/win/generated/UninstallCommands.nsh" CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/win") # NSIS.template.in, NSIS.InstallOptions.ini.in
else()
    if(NOT (CPACK_GENERATOR STREQUAL "DEB"))
        message(WARNING "CPack generator must be DEB! Setting generator to DEB...")
        set(CPACK_GENERATOR "DEB" CACHE INTERNAL "" FORCE)
    endif()
    set(CPACK_STRIP_FILES ON)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <${CPACK_PACKAGE_CONTACT}>")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${PROJECT_NAME}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxcb-cursor0")
    set(CPACK_DEBIAN_COMPRESSION_TYPE "zstd")

    # Add symlinks so it can be ran from anywhere
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink /opt/${PROJECT_NAME}/${PROJECT_NAME}cli ${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME}cli)")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME}cli"
            DESTINATION "/usr/bin")

    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink /opt/${PROJECT_NAME}/${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME})")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME}"
            DESTINATION "/usr/bin")
endif()
include(CPack)
