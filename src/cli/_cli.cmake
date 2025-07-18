# Create executable
add_executable(${PROJECT_NAME}cli
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Tree.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Tree.h")

vpkedit_configure_target(${PROJECT_NAME}cli)

target_link_libraries(
        ${PROJECT_NAME}cli PUBLIC
        argparse::argparse
        indicators::indicators
        sourcepp::bsppp
        sourcepp::vpkpp)

target_include_directories(
        ${PROJECT_NAME}cli PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/shared")
