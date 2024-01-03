include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1)
FetchContent_MakeAvailable(googletest)
enable_testing()

add_executable(${PROJECT_NAME}test
        "${CMAKE_CURRENT_LIST_DIR}/VPKTest.cpp")

target_link_libraries(${PROJECT_NAME}test PUBLIC lib${PROJECT_NAME} gtest_main)

target_include_directories(
        ${PROJECT_NAME}test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}test)
