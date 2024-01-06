# argparse
set(ARGPARSE_INSTALL OFF CACHE BOOL "" FORCE)
set(ARGPARSE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/argparse")

# Add sources and create executable
add_executable(${PROJECT_NAME}cli
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp")

target_link_libraries(${PROJECT_NAME}cli PUBLIC lib${PROJECT_NAME} argparse::argparse)
