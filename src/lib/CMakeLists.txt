cmake_minimum_required(VERSION 3.20)
project(lib)

# add ffmpeg bindings
add_subdirectory(ffmpeg_cpp)

add_library(${PROJECT_NAME} Recorder.cpp Screen.cpp utils.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC ffmpeg_cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)