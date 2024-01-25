# For hashing parts of the VPK and stored files
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/md5")

# Configure version header
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Version.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Version.h"
        @ONLY)

add_library(
        lib${PROJECT_NAME}

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/CRC.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/FileStream.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/Misc.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Entry.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Options.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/PackFile.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Version.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/VPK.h"

        "${CMAKE_CURRENT_LIST_DIR}/detail/CRC.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/detail/FileStream.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/detail/Misc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Entry.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/PackFile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/VPK.cpp")

target_link_libraries(lib${PROJECT_NAME} PUBLIC MD5)

target_include_directories(
        lib${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
