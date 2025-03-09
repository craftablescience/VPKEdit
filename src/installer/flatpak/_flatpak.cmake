install(TARGETS ${PROJECT_NAME}cli ${PROJECT_NAME}
        DESTINATION "bin/")

# Desktop file
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/desktop.in"
        "${CMAKE_CURRENT_LIST_DIR}/generated/${VPKEDIT_FLATPAK_ID}.desktop")
install(FILES "${CMAKE_CURRENT_LIST_DIR}/generated/${VPKEDIT_FLATPAK_ID}.desktop"
        DESTINATION "share/applications/")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/branding/logo.png"
        DESTINATION "share/pixmaps/"
        RENAME "${PROJECT_NAME}.png")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/branding/logo.png"
        DESTINATION "share/icons/hicolor/128x128/apps/"
        RENAME "${VPKEDIT_FLATPAK_ID}.png")

# MIME type info
set(VPKEDIT_MIME_TYPE_ICON_ID "${VPKEDIT_FLATPAK_ID}" CACHE INTERNAL "" FORCE)
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/../linux/mime-type.xml.in"
        "${CMAKE_CURRENT_LIST_DIR}/generated/mime-type.xml")
install(FILES "${CMAKE_CURRENT_LIST_DIR}/generated/mime-type.xml"
        DESTINATION "share/mime/packages/"
        RENAME "${VPKEDIT_FLATPAK_ID}.xml")
