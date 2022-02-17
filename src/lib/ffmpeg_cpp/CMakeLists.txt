cmake_minimum_required(VERSION 3.20)
project(ffmpeg_cpp)

add_library(${PROJECT_NAME}
        ffmpeg_cpp.cpp
        Codec.cpp
        Format.cpp
        Rescaler.cpp
        platform.cpp
        Frame.cpp
        )
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PUBLIC ../include)
