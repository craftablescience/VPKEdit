set(VPKEDIT_PLUGIN_PREVIEW_TARGETS "")

# Create a preview plugin
function(vpkedit_add_preview_plugin)
    cmake_parse_arguments(PARSE_ARGV 0 OPTIONS "" "NAME;CLASS_NAME" "DEPS")
    if("${OPTIONS_NAME}" STREQUAL "")
        message(FATAL_ERROR "Plugin must be named!")
    endif()
    if("${OPTIONS_CLASS_NAME}" STREQUAL "")
        message(FATAL_ERROR "Plugin must have a valid class name!")
    endif()

    set(PLUGIN_TARGET "${PROJECT_NAME}_${OPTIONS_NAME}_preview")
    qt_add_plugin(${PLUGIN_TARGET} CLASS_NAME "${OPTIONS_CLASS_NAME}"
            "${CMAKE_CURRENT_LIST_DIR}/IVPKEditPreviewPlugin_V1_0.h"
            "${CMAKE_CURRENT_LIST_DIR}/${OPTIONS_NAME}/${OPTIONS_CLASS_NAME}.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/${OPTIONS_NAME}/${OPTIONS_CLASS_NAME}.h")
    vpkedit_configure_target(${PLUGIN_TARGET})

    target_use_qt(${PLUGIN_TARGET})
    target_link_libraries(${PLUGIN_TARGET} PRIVATE ${OPTIONS_DEPS})

    set_target_properties(${PLUGIN_TARGET} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/previews"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/previews")
    add_dependencies(${PROJECT_NAME} ${PLUGIN_TARGET})

    list(APPEND VPKEDIT_PLUGIN_PREVIEW_TARGETS ${PLUGIN_TARGET})
    return(PROPAGATE VPKEDIT_PLUGIN_PREVIEW_TARGETS)
endfunction()

# Add preview plugins
vpkedit_add_preview_plugin(NAME "dmx" CLASS_NAME "DMXPreview" DEPS sourcepp::dmxpp)
vpkedit_add_preview_plugin(NAME "mdl" CLASS_NAME "MDLPreview" DEPS sourcepp::kvpp sourcepp::mdlpp sourcepp::vtfpp)
