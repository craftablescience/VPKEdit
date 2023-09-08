cmake_minimum_required(VERSION 3.16)
project(MD5)

add_library(${PROJECT_NAME} STATIC
        ${CMAKE_CURRENT_LIST_DIR}/MD5.cpp
        ${CMAKE_CURRENT_LIST_DIR}/MD5.h)
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})
