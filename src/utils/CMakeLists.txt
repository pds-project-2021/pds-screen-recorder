cmake_minimum_required(VERSION 3.20)
project(utils)

add_library(${PROJECT_NAME} Screen.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)