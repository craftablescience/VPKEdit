# Create a preview plugin
function(vpkedit_add_preview_plugin)
    cmake_parse_arguments(PARSE_ARGV 0 OPTIONS "" "NAME;CLASS_NAME" "DEPS")
    if("${OPTIONS_NAME}" STREQUAL "")
        message(FATAL_ERROR "Plugin must be named!")
    endif()
    if("${OPTIONS_CLASS_NAME}" STREQUAL "")
        message(FATAL_ERROR "Plugin must have a valid class name!")
    endif()

    set(PLUGIN_DIR "${PROJECT_NAME}_previews")
    set(PLUGIN_TARGET "${PROJECT_NAME}_${OPTIONS_NAME}_previews")
    qt_add_plugin(${PLUGIN_TARGET} CLASS_NAME "${OPTIONS_CLASS_NAME}"
            "${CMAKE_CURRENT_LIST_DIR}/../IVPKEditWindowAccess.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/../IVPKEditWindowAccess.hpp"
            "${CMAKE_CURRENT_LIST_DIR}/IVPKEditPreviewPlugin.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/IVPKEditPreviewPlugin.hpp"
            "${CMAKE_CURRENT_LIST_DIR}/${OPTIONS_NAME}/${OPTIONS_CLASS_NAME}.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/${OPTIONS_NAME}/${OPTIONS_CLASS_NAME}.hpp")
    cs_configure_target(${PLUGIN_TARGET})

    target_use_qt(${PLUGIN_TARGET})
    target_link_libraries(${PLUGIN_TARGET} PRIVATE ${OPTIONS_DEPS})

    set_target_properties(${PLUGIN_TARGET} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_DIR}")

    if(VPKEDIT_BUILD_GUI)
        add_dependencies(${PROJECT_NAME} ${PLUGIN_TARGET})
    endif()
endfunction()

# todo: installer

# Add preview plugins
vpkedit_add_preview_plugin(NAME "dmx" CLASS_NAME "DMXPreview" DEPS sourcepp::kvpp)
