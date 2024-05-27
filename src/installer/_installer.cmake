# Set up install rules
if(VPKEDIT_BUILD_CLI)
    install(TARGETS ${PROJECT_NAME}cli
            RUNTIME DESTINATION .
            LIBRARY DESTINATION .)
endif()

install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_LIST_DIR}/.nonportable"
        DESTINATION .)

install(TARGETS ${PROJECT_NAME} vtflib
        RUNTIME DESTINATION .
        LIBRARY DESTINATION .)

if(WIN32)
    install(IMPORTED_RUNTIME_ARTIFACTS
            Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets
            RUNTIME DESTINATION .
            LIBRARY DESTINATION .)

    install(FILES
            "${QT_BASEDIR}/bin/opengl32sw.dll"
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

    # Desktop file
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/deb/desktop.in"
            "${CMAKE_CURRENT_LIST_DIR}/deb/generated/${PROJECT_NAME}.desktop")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/deb/generated/${PROJECT_NAME}.desktop"
            DESTINATION "/usr/share/applications/")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/../gui/res/icon-128.png"
            DESTINATION "/usr/share/pixmaps/"
            RENAME "${PROJECT_NAME}.png")

    # MIME type info
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/deb/mime-type.xml.in"
            "${CMAKE_CURRENT_LIST_DIR}/deb/generated/mime-type.xml")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/deb/generated/mime-type.xml"
            DESTINATION "/usr/share/mime/packages/"
            RENAME "${PROJECT_NAME}.xml")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/../gui/res/icon-128.png"
            DESTINATION "/usr/share/icons/hicolor/128x128/mimetypes/"
            RENAME "application-x-vpkedit.png")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/../gui/res/icon-512.png"
            DESTINATION "/usr/share/icons/hicolor/512x512/mimetypes/"
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
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DISPLAY_NAME ${PROJECT_NAME_PRETTY})
    set(CPACK_NSIS_PACKAGE_NAME ${PROJECT_NAME_PRETTY})
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_LIST_DIR}/../gui/res/icon.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${PROJECT_NAME}.exe")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CMAKE_PROJECT_HOMEPAGE_URL}")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    set(HELP_QUOTE "\"") # CMake is shit
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
            WriteRegStr HKCR '.bmz' '' '${PROJECT_NAME_PRETTY}'
            WriteRegStr HKCR '.bsp' '' '${PROJECT_NAME_PRETTY}'
            WriteRegStr HKCR '.fpx' '' '${PROJECT_NAME_PRETTY}'
            WriteRegStr HKCR '.gcf' '' '${PROJECT_NAME_PRETTY}'
            WriteRegStr HKCR '.gma' '' '${PROJECT_NAME_PRETTY}'
            WriteRegStr HKCR '.pck' '' '${PROJECT_NAME_PRETTY}'
            WriteRegStr HKCR '.vpk' '' '${PROJECT_NAME_PRETTY}'
            WriteRegStr HKCR '${PROJECT_NAME_PRETTY}' '' 'VPKEdit Pack File'
            WriteRegStr HKCR '${PROJECT_NAME_PRETTY}\\\\shell' '' 'open'
            WriteRegStr HKCR '${PROJECT_NAME_PRETTY}\\\\DefaultIcon' '' '$INSTDIR\\\\${PROJECT_NAME}.exe,0'
            WriteRegStr HKCR '${PROJECT_NAME_PRETTY}\\\\shell\\\\open\\\\command' '' '$INSTDIR\\\\${PROJECT_NAME}.exe \\${HELP_QUOTE}%1\\${HELP_QUOTE}'
            System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
        ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
            DeleteRegKey HKCR '.bmz'
            DeleteRegKey HKCR '.bsp'
            DeleteRegKey HKCR '.fpx'
            DeleteRegKey HKCR '.gcf'
            DeleteRegKey HKCR '.gma'
            DeleteRegKey HKCR '.pck'
            DeleteRegKey HKCR '.vpk'
            DeleteRegKey HKCR '${PROJECT_NAME_PRETTY}'
        ")
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_STRIP_FILES ON)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <${CPACK_PACKAGE_CONTACT}>")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${PROJECT_NAME}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxcb-cursor0")

    # Add symlinks so it can be ran from anywhere
    if(VPKEDIT_BUILD_CLI)
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink /opt/${PROJECT_NAME}/${PROJECT_NAME}cli ${CMAKE_CURRENT_LIST_DIR}/deb/generated/${PROJECT_NAME}cli)")
        install(FILES
                "${CMAKE_CURRENT_LIST_DIR}/deb/generated/${PROJECT_NAME}cli"
                DESTINATION "/usr/bin")
    endif()
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink /opt/${PROJECT_NAME}/${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/deb/generated/${PROJECT_NAME})")
    install(FILES
            "${CMAKE_CURRENT_LIST_DIR}/deb/generated/${PROJECT_NAME}"
            DESTINATION "/usr/bin")
endif()
include(CPack)
