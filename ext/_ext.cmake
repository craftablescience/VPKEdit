cs_include_dirset_property(
            DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
    set_target_properties(
            ${PROJECT_NAME} PROPERTIES
            VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>"
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
            VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${QT_BASEDIR}/bin;")

    # Qt hacks
    set_target_properties(${PROJECT_NAME}_lrelease PROPERTIES FOLDER "Qt")
    set_target_properties(${PROJECT_NAME}_lupdate PROPERTIES FOLDER "Qt")
    set_target_properties(${PROJECT_NAME}_other_files PROPERTIES FOLDER "Qt")ectory(ext/cli)
cs_include_directory(ext/gui)
cs_include_directory(ext/shared)
