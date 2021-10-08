cmake_minimum_required(VERSION 3.21)
project(pds_screen_recorder)

set(CMAKE_CXX_STANDARD 14)

find_package(PkgConfig REQUIRED)

# GTK4
pkg_check_modules(GTK4 REQUIRED gtk4)
include_directories(${GTK4_INCLUDE_DIRS})
link_directories(${GTK4_LIBRARY_DIRS})
add_definitions(${GTK4_CFLAGS_OTHER})

# ffmpeg
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

# SDL2
find_package(SDL2 REQUIRED)
include_directories(SDL2 ${SDL2_INCLUDE_DIRS})

add_executable(pds_screen_recorder src/main.cpp src/ScreenRecorder.cpp src/ScreenRecorder.h src/exceptions/fsException.h src/exceptions/dataException.h src/exceptions/avException.h src/ffmpeg/ffmpeg.h src/ffmpeg/ffmpegcpp.h)
target_link_libraries(${PROJECT_NAME} ${GTK4_LIBRARIES} ${SDL2_LIBRARIES} PkgConfig::LIBAV)