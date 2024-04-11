# argparse
set(ARGPARSE_INSTALL OFF CACHE BOOL "" FORCE)
set(ARGPARSE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/argparse")

# indicators
set(INDICATORS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(INDICATORS_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/indicators")

# Add sources and create executable
list(APPEND ${PROJECT_NAME}cli_SOURCES "${CMAKE_CURRENT_LIST_DIR}/Main.cpp")
if(WIN32)
    list(APPEND ${PROJECT_NAME}cli_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../gui/res/icon.rc")
endif()

add_executable(${PROJECT_NAME}cli ${${PROJECT_NAME}cli_SOURCES})

vpkedit_configure_target(${PROJECT_NAME}cli)

target_link_libraries(${PROJECT_NAME}cli PUBLIC lib${PROJECT_NAME} argparse::argparse indicators::indicators)
