cmake_minimum_required(VERSION 3.21)

project(crypto_utils LANGUAGES CXX)

set(${PROJECT_NAME}_SRCS
    android_pubkey.cpp
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ssl
    crypto
)

