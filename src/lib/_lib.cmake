# For hashing parts of a VPK and stored files
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/thirdparty/md5")

# For parsing BSP pack lumps and very basic uncompressed/LZMA-compressed ZIP files
# If the target already exists, someone else is providing it. We can assume they've enabled the necessary features...
if (NOT TARGET MINIZIP::minizip)
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
endif()

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

		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/format/BSP.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/format/GCF.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/format/GMA.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/format/GRP.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/format/PAK.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/format/PCK.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/format/VPK.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/format/ZIP.h"

		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Attribute.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Entry.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Options.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/PackFile.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/PackFileType.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/vpkedit/Version.h"

		"${CMAKE_CURRENT_LIST_DIR}/detail/Adler32.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/detail/CRC32.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/detail/FileStream.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/detail/Misc.cpp"

		"${CMAKE_CURRENT_LIST_DIR}/format/BSP.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/format/GCF.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/format/GMA.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/format/GRP.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/format/PAK.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/format/PCK.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/format/VPK.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/format/ZIP.cpp"

		"${CMAKE_CURRENT_LIST_DIR}/Entry.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/PackFile.cpp")

target_link_libraries(lib${PROJECT_NAME} PUBLIC MD5 MINIZIP::minizip)

target_include_directories(
		lib${PROJECT_NAME} PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/include")

if(VPKEDIT_BUILD_FOR_STRATA_SOURCE)
	target_compile_definitions(lib${PROJECT_NAME} PUBLIC -DVPKEDIT_BUILD_FOR_STRATA_SOURCE)
endif()
