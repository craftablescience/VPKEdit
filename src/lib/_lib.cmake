# For hashing parts of the VPK and stored files
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/md5")

add_library(
        lib${PROJECT_NAME}

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/CRC.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/FileStream.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/VPK.h"

        "${CMAKE_CURRENT_LIST_DIR}/detail/CRC.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/detail/FileStream.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/VPK.cpp")

target_link_libraries(lib${PROJECT_NAME} PUBLIC MD5)

target_include_directories(
        lib${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

# libvpkedit doesn't use C++20 yet
set_target_properties(lib${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
