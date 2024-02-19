# Configure version header
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/Version.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/Version.h")

add_library(
        lib${PROJECT_NAME}c SHARED

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/VPKWrapper.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/AttributeWrapper.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/EntryWrapper.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/OptionsWrapper.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/PackFileTypeWrapper.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/PackFileWrapper.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/StringArray.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/Version.h"

        "${CMAKE_CURRENT_LIST_DIR}/format/VPKWrapper.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/API.h"
        "${CMAKE_CURRENT_LIST_DIR}/EntryWrapper.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Helpers.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Helpers.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/PackFileWrapper.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/StringArray.cpp")

target_link_libraries(lib${PROJECT_NAME}c PUBLIC lib${PROJECT_NAME})

target_include_directories(lib${PROJECT_NAME}c PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
