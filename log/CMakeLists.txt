cmake_minimum_required(VERSION 3.21)

project(log-header-only VERSION 1.0.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE include)

target_link_libraries(${PROJECT_NAME} INTERFACE log)

