# For hashing parts of a VPK and stored files
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/md5")

# For parsing BSP pack lumps and very basic uncompressed/LZMA-compressed ZIP files
set(MZ_COMPAT           OFF CACHE INTERNAL "")
set(MZ_ZLIB             OFF CACHE INTERNAL "")
set(MZ_BZIP2            OFF CACHE INTERNAL "")
set(MZ_ZSTD             OFF CACHE INTERNAL "")
set(MZ_LIBCOMP          OFF CACHE INTERNAL "")
set(MZ_PKCRYPT          OFF CACHE INTERNAL "")
set(MZ_WZAES            OFF CACHE INTERNAL "")
set(MZ_OPENSSL          OFF CACHE INTERNAL "")
set(MZ_FETCH_LIBS       ON  CACHE INTERNAL "")
set(MZ_FORCE_FETCH_LIBS ON  CACHE INTERNAL "")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/minizip-ng")

# Configure version header
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Version.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Version.h"
        @ONLY)

add_library(
        lib${PROJECT_NAME}

        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/Adler32.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/CRC32.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/FileStream.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/detail/Misc.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/BSP.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Entry.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/GCF.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/GMA.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Options.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/PackFile.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Version.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/VPK.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/ZIP.h"

        "${CMAKE_CURRENT_LIST_DIR}/detail/Adler32.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/detail/CRC32.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/detail/FileStream.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/detail/Misc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/BSP.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Entry.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/GCF.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/GMA.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/PackFile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/VPK.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ZIP.cpp")

target_link_libraries(lib${PROJECT_NAME} PUBLIC MD5 MINIZIP::minizip)

target_include_directories(
        lib${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(VPKEDIT_BUILD_FOR_STRATA_SOURCE)
    target_compile_definitions(lib${PROJECT_NAME} PUBLIC -DVPKEDIT_BUILD_FOR_STRATA_SOURCE)
endif()
