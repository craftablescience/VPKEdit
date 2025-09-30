# Create library
qt_add_plugin(${PROJECT_NAME}_mdl_preview CLASS_NAME "MDLPreview"
        "${CMAKE_CURRENT_LIST_DIR}/../../../utility/ThemedIcon.h"
        "${CMAKE_CURRENT_LIST_DIR}/../IVPKEditPreviewPlugin_V1_0.h"
        "${CMAKE_CURRENT_LIST_DIR}/MDLPreview.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/MDLPreview.h")
vpkedit_configure_target(${PROJECT_NAME}_mdl_preview)

target_link_libraries(${PROJECT_NAME}_mdl_preview PRIVATE
        sourcepp::kvpp sourcepp::mdlpp sourcepp::vtfpp Qt::Core Qt::Gui  Qt::OpenGL Qt::OpenGLWidgets Qt::Widgets)
target_include_directories(${PROJECT_NAME}_mdl_preview PRIVATE
        "${QT_INCLUDE}" "${QT_INCLUDE}/QtCore" "${QT_INCLUDE}/QtGui" "${QT_INCLUDE}/QtOpenGL" "${QT_INCLUDE}/QtOpenGLWidgets" "${QT_INCLUDE}/QtWidgets")
set_target_properties(${PROJECT_NAME}_mdl_preview PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)

set_target_properties(${PROJECT_NAME}_mdl_preview PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/previews"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/previews")
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_mdl_preview)
