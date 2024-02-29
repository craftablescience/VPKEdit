# Configure version header
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/Version.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/Version.h")

add_library(
        lib${PROJECT_NAME}c SHARED

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/BSP.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/FPX.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/GCF.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/GMA.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/GRP.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/PAK.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/PCK.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/VPK.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/format/ZIP.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/API.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/Attribute.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/Entry.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/Options.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/PackFile.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/PackFileType.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/StringArray.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkeditc/Version.h"

        "${CMAKE_CURRENT_LIST_DIR}/format/BSP.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/format/FPX.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/format/GCF.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/format/GMA.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/format/GRP.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/format/PAK.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/format/PCK.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/format/VPK.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/format/ZIP.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/Entry.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Helpers.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Helpers.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/PackFile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/StringArray.cpp")

set_target_properties(lib${PROJECT_NAME}c PROPERTIES PREFIX "")

target_link_libraries(lib${PROJECT_NAME}c PUBLIC lib${PROJECT_NAME})
