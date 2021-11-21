cmake_minimum_required(VERSION 3.20)
project(ffmpegcpp)

add_library(${PROJECT_NAME} Codec.cpp Format.cpp Frame.cpp Packet.cpp Stream.cpp Dictionary.cpp Rescaler.cpp platform.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)