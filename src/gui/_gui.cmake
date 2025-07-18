# Create executable
list(APPEND ${PROJECT_NAME}_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/ControlsDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/ControlsDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/EntryOptionsDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/EntryOptionsDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/NewUpdateDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/NewUpdateDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/PackFileOptionsDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/PackFileOptionsDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VerifyChecksumsDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VerifyChecksumsDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VerifySignatureDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VerifySignatureDialog.h"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VICEDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VICEDialog.h"

        "${CMAKE_CURRENT_LIST_DIR}/extensions/Folder.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/extensions/Folder.h"
        "${CMAKE_CURRENT_LIST_DIR}/extensions/SingleFile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/extensions/SingleFile.h"

        "${CMAKE_CURRENT_LIST_DIR}/previews/AudioPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/AudioPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/DirPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/DirPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/DMXPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/DMXPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/EmptyPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/EmptyPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/InfoPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/InfoPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/MDLPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/MDLPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/TextPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/TextPreview.h"
        "${CMAKE_CURRENT_LIST_DIR}/previews/TexturePreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/previews/TexturePreview.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/res/logo$<$<CONFIG:Debug>:_alt>.qrc"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/res.qrc"

        "${CMAKE_CURRENT_LIST_DIR}/utility/AudioPlayer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/AudioPlayer.h"
        "${CMAKE_CURRENT_LIST_DIR}/utility/DiscordPresence.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/DiscordPresence.h"
        "${CMAKE_CURRENT_LIST_DIR}/utility/ImageLoader.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/ImageLoader.h"
        "${CMAKE_CURRENT_LIST_DIR}/utility/Options.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/Options.h"
        "${CMAKE_CURRENT_LIST_DIR}/utility/TempDir.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/TempDir.h"
        "${CMAKE_CURRENT_LIST_DIR}/utility/ThemedIcon.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/ThemedIcon.h"

        "${CMAKE_CURRENT_LIST_DIR}/EntryContextMenuData.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/EntryContextMenuData.h"
        "${CMAKE_CURRENT_LIST_DIR}/EntryTree.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/EntryTree.h"
        "${CMAKE_CURRENT_LIST_DIR}/FileViewer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/FileViewer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Window.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Window.h")

add_executable(${PROJECT_NAME} WIN32 ${${PROJECT_NAME}_SOURCES})

vpkedit_configure_target(${PROJECT_NAME})

file(GLOB ${PROJECT_NAME}_I18N_TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/i18n/${PROJECT_NAME}_*.ts")
qt_add_translations(${PROJECT_NAME}
		TS_FILES ${${PROJECT_NAME}_I18N_TS_FILES}
		RESOURCE_PREFIX "/i18n"
		SOURCES ${${PROJECT_NAME}_SOURCES})

target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        ${CMAKE_DL_LIBS}
        discord-rpc
		miniaudio
        sourcepp::bsppp
        sourcepp::dmxpp
        sourcepp::kvpp
        sourcepp::mdlpp
        sourcepp::steampp
        sourcepp::vcryptpp
        sourcepp::vpkpp
        sourcepp::vtfpp)

target_include_directories(
        ${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/shared")

target_use_qt(${PROJECT_NAME})

# Copy these next to the executable
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CREDITS.md" "${CMAKE_BINARY_DIR}/CREDITS.md" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"    "${CMAKE_BINARY_DIR}/LICENSE"    COPYONLY)
