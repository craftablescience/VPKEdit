cmake_minimum_required(VERSION 3.15)
project(VPKTool)
set(CMAKE_CXX_STANDARD 17)

option(VPKTOOL_BUILD_TESTS "Run library tests" OFF)
if(VPKTOOL_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

add_library(
        ${PROJECT_NAME}

        ${CMAKE_CURRENT_SOURCE_DIR}/include/vpktool/FileInputStream.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/vpktool/VPK.h

        ${CMAKE_CURRENT_SOURCE_DIR}/src/FileInputStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VPK.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(VPKTOOL_BUILD_TESTS)
    set(BUILD_SHARED_LIBS OFF)
    add_executable(VPKToolTests ${CMAKE_CURRENT_SOURCE_DIR}/tests/VPKTest.cpp)
    target_link_libraries(VPKToolTests PUBLIC ${PROJECT_NAME} gtest_main)
    target_include_directories(VPKToolTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    include(GoogleTest)
    gtest_discover_tests(VPKToolTests)
endif()
