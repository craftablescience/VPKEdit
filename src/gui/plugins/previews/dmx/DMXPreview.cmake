# Create library
qt_add_plugin(${PROJECT_NAME}_dmx_preview CLASS_NAME "DMXPreview"
        "${CMAKE_CURRENT_LIST_DIR}/../IVPKEditPreviewPlugin_V1_0.h"
        "${CMAKE_CURRENT_LIST_DIR}/DMXPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/DMXPreview.h")
vpkedit_configure_target(${PROJECT_NAME}_dmx_preview)

target_link_libraries(${PROJECT_NAME}_dmx_preview PRIVATE sourcepp::dmxpp Qt::Core Qt::Widgets)
set_target_properties(${PROJECT_NAME}_dmx_preview PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)
target_include_directories(${PROJECT_NAME}_dmx_preview PRIVATE "${QT_INCLUDE}" "${QT_INCLUDE}/QtCore" "${QT_INCLUDE}/QtWidgets")

set_target_properties(${PROJECT_NAME}_dmx_preview PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/previews"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/previews")
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_dmx_preview)
