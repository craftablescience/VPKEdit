# Configure version header
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/include/Version.h.in"
        "${CMAKE_CURRENT_LIST_DIR}/include/Version.h")

# sourcepp
set(SOURCEPP_BUILD_WIN7_COMPAT ON CACHE INTERNAL "")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/shared/thirdparty/sourcepp")
