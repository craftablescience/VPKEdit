set(${PROJECT_NAME}_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/res/gui/attribution.qrc"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/gui/res.qrc"

        "${CMAKE_CURRENT_SOURCE_DIR}/src/common/Config.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VCreditsDialog.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dialogs/VCreditsDialog.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/plugins/previews/VPreviewPlugin.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/plugins/previews/VPreviewPlugin.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/plugins/thumbnails/VThumbnailPlugin.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/plugins/thumbnails/VThumbnailPlugin.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/plugins/VPluginLoader.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/plugins/VPluginLoader.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/plugins/VWindowAccess.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/plugins/VWindowAccess.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/utility/VDiscordPresence.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/VDiscordPresence.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/VSettings.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utility/VSettings.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/directories/VPreviewDirectoryCompact.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/directories/VPreviewDirectoryCompact.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/directories/VPreviewDirectoryDetails.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/directories/VPreviewDirectoryDetails.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/VPreviewDirectoryParent.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/VPreviewDirectoryParent.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/VPreviewInfo.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/VPreviewInfo.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/VPreviewNone.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/previews/VPreviewNone.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/widgets/VEntryTreeView.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/VEntryTreeView.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/VNavigationBar.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/VNavigationBar.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/VPreviewParent.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/widgets/VPreviewParent.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/windows/VMainWindow.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/windows/VMainWindow.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/VPKEdit.cpp")
qt_add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${${PROJECT_NAME}_SOURCES})
cs_configure_target(${PROJECT_NAME} LOGO MANIFEST)

if(VPKEDIT_BUILD_FOR_STRATA_SOURCE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VPKEDIT_BUILD_FOR_STRATA_SOURCE)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/cli" "${CMAKE_CURRENT_SOURCE_DIR}/src/common" "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")

target_link_libraries(${PROJECT_NAME} PRIVATE discord-rpc sourcepp::bsppp sourcepp::vpkpp sourcepp::vtfpp)

file(GLOB ${PROJECT_NAME}_I18N_TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/i18n/${PROJECT_NAME}_*.ts")
list(APPEND ${PROJECT_NAME}_I18N_TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/i18n/base/${PROJECT_NAME}_en.ts")
qt_add_translations(${PROJECT_NAME}
        TS_FILES ${${PROJECT_NAME}_I18N_TS_FILES}
        RESOURCE_PREFIX "/i18n"
        SOURCES ${${PROJECT_NAME}_SOURCES})
target_use_qt(${PROJECT_NAME})
