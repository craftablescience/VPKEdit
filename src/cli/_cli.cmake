# argparse
set(ARGPARSE_INSTALL OFF CACHE BOOL "" FORCE)
set(ARGPARSE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/argparse")

# Add sources and create executable
list(APPEND ${PROJECT_NAME}cli_SOURCES "${CMAKE_CURRENT_LIST_DIR}/Main.cpp")
if(WIN32)
    list(APPEND ${PROJECT_NAME}cli_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../gui/res/icon.rc")
endif()

add_executable(${PROJECT_NAME}cli ${${PROJECT_NAME}cli_SOURCES})

target_link_libraries(${PROJECT_NAME}cli PUBLIC lib${PROJECT_NAME} argparse::argparse)

# Create PDBs in release
if(WIN32)
    target_compile_options(
            ${PROJECT_NAME}cli PRIVATE
            "$<$<CONFIG:Release>:/Zi>")
    target_link_options(
            ${PROJECT_NAME}cli PRIVATE
            "$<$<CONFIG:Release>:/DEBUG>"
            "$<$<CONFIG:Release>:/OPT:REF>"
            "$<$<CONFIG:Release>:/OPT:ICF>")
endif()
