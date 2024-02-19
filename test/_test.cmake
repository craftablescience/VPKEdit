include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1)
FetchContent_MakeAvailable(googletest)
enable_testing()

list(APPEND ${PROJECT_NAME}_test_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/lib/format/VPK.cpp")

add_executable(${PROJECT_NAME}_test ${${PROJECT_NAME}_test_SOURCES})

target_link_libraries(${PROJECT_NAME}_test PUBLIC lib${PROJECT_NAME} gtest_main)
if(VPKEDIT_BUILD_LIBC)
    target_link_libraries(${PROJECT_NAME}_test PUBLIC lib${PROJECT_NAME}c)
endif()

target_include_directories(${PROJECT_NAME}_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_test)
