add_executable(${PROJECT_NAME}cli
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp")

target_link_libraries(${PROJECT_NAME}cli PUBLIC lib${PROJECT_NAME})

target_include_directories(
        ${PROJECT_NAME}cli PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
