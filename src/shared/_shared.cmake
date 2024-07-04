# Configure version header
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/config/Version.h.in"
        "${CMAKE_CURRENT_LIST_DIR}/config/Version.h")


# sourcepp
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/shared/thirdparty/sourcepp")
