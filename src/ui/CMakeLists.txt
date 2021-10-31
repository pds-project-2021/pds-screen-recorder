cmake_minimum_required(VERSION 3.20)
project(ui)

add_library(${PROJECT_NAME} Interface.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)