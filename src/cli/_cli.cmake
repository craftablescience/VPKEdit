add_executable(${PROJECT_NAME}cli
        "${CMAKE_CURRENT_SOURCE_DIR}/src/common/Config.hpp"

        "${CMAKE_CURRENT_LIST_DIR}/VPKEdit.cpp")
cs_configure_target(${PROJECT_NAME}cli LOGO MANIFEST)

if(VPKEDIT_BUILD_FOR_STRATA_SOURCE)
    target_compile_definitions(${PROJECT_NAME}cli PUBLIC MARETF_BUILD_FOR_STRATA_SOURCE)
endif()

target_include_directories(${PROJECT_NAME}cli PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/cli" "${CMAKE_CURRENT_SOURCE_DIR}/src/common")

target_link_libraries(${PROJECT_NAME}cli PRIVATE argparse::argparse sourcepp::bsppp sourcepp::vpkpp)
