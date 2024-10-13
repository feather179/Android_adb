cmake_minimum_required(VERSION 3.21)

project(diagnose_usb LANGUAGES CXX)

set(${PROJECT_NAME}_SRCS
    diagnose_usb.cpp
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PUBLIC
    base
)

