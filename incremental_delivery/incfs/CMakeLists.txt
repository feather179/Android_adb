cmake_minimum_required(VERSION 3.21)

project(incfs_utils LANGUAGES CXX)

set(${PROJECT_NAME}_SRCS
    util/map_ptr.cpp
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC util/include)
target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PUBLIC
    base
    utils
)

