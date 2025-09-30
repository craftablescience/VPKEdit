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

    install(TARGETS ${PROJECT_NAME}_dmx_preview ${PROJECT_NAME}_mdl_preview
            DESTINATION previews)

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
elseif(APPLE)
    # macOS app bundle installation
    install(TARGETS ${PROJECT_NAME}
            BUNDLE DESTINATION .
            RENAME ${PROJECT_NAME_PRETTY})
    
    install(TARGETS ${PROJECT_NAME}cli
            DESTINATION "Command Line Utilities")

    install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            DESTINATION .)

    install(TARGETS ${PROJECT_NAME}_dmx_preview ${PROJECT_NAME}_mdl_preview
            DESTINATION "${PROJECT_NAME_PRETTY}.app/Contents/PlugIns/previews")

    # Deploy Qt into the bundle
    if(VPKEDIT_MAC_BUNDLE_QT)
        qt_generate_deploy_app_script(
                TARGET ${PROJECT_NAME}
                OUTPUT_SCRIPT qt_deploy_script)
        install(SCRIPT ${qt_deploy_script})
    endif()
elseif(UNIX)
    install(TARGETS ${PROJECT_NAME}cli
            DESTINATION "${CMAKE_INSTALL_BINDIR}")

    install(TARGETS ${PROJECT_NAME}
            DESTINATION "${CMAKE_INSTALL_BINDIR}")

    install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME}")

    install(TARGETS ${PROJECT_NAME}_dmx_preview ${PROJECT_NAME}_mdl_preview
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/previews")

    # Use system Qt - no install rules

    # Desktop file
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/linux/desktop.in"
            "${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME}.desktop")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/generated/${PROJECT_NAME}.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/brand/logo_16.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/16x16/apps"
            RENAME "${PROJECT_NAME}.png")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/brand/logo$<$<CONFIG:Debug>:_alt>_128.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps"
            RENAME "${PROJECT_NAME}.png")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/brand/logo$<$<CONFIG:Debug>:_alt>_512.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps"
            RENAME "${PROJECT_NAME}.png")

    # MIME type info
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/linux/mime-type.xml.in"
            "${CMAKE_CURRENT_LIST_DIR}/linux/generated/mime-type.xml")
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/generated/mime-type.xml"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages"
            RENAME "${PROJECT_NAME}.xml")
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
elseif(APPLE)
    if(NOT (CPACK_GENERATOR STREQUAL "DragNDrop"))
        message(WARNING "CPack generator should be DragNDrop on macOS! Setting generator to DragNDrop...")
        set(CPACK_GENERATOR "DragNDrop" CACHE INTERNAL "" FORCE)
    endif()
    set(CPACK_BUNDLE_NAME ${PROJECT_NAME_PRETTY})
    set(CPACK_DMG_VOLUME_NAME ${PROJECT_NAME_PRETTY})
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/install/macos/DMGSetup.scpt")
else()
    if(CPACK_GENERATOR STREQUAL "DEB")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <${CPACK_PACKAGE_CONTACT}>")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxcb-cursor0, libqt6core6 (>= 6.8.2), libqt6dbus6 (>= 6.8.2), libqt6gui6 (>= 6.8.2), libqt6widgets6 (>= 6.8.2), libqt6network6 (>= 6.8.2), libqt6opengl6 (>= 6.8.2), libqt6openglwidgets6 (>= 6.8.2), libqt6svg6 (>= 6.8.2)")
        set(CPACK_DEBIAN_COMPRESSION_TYPE "zstd")
    elseif(CPACK_GENERATOR STREQUAL "RPM")
        set(CPACK_RPM_PACKAGE_LICENSE "MIT")
        set(CPACK_RPM_PACKAGE_REQUIRES "libxcb, qt6-qtbase >= 6.8.2, qt6-qtbase-gui >= 6.8.2, qt6-qtsvg >= 6.8.2")
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
