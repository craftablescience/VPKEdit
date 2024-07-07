# Configure version header
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/include/Version.h.in"
        "${CMAKE_CURRENT_LIST_DIR}/include/Version.h")


# sourcepp
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/shared/thirdparty/sourcepp")
