# argparse
set(ARGPARSE_INSTALL OFF CACHE BOOL "" FORCE)
set(ARGPARSE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/argparse")

# indicators
set(INDICATORS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(INDICATORS_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/indicators")

# Create executable
add_executable(${PROJECT_NAME}cli "${CMAKE_CURRENT_LIST_DIR}/Main.cpp")

vpkedit_configure_target(${PROJECT_NAME}cli)

target_link_libraries(
        ${PROJECT_NAME}cli PUBLIC
        argparse::argparse
        indicators::indicators
        vpkpp)

target_include_directories(
        ${PROJECT_NAME}cli PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/shared/config")
