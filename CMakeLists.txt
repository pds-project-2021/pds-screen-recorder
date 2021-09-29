cmake_minimum_required(VERSION 3.21)
project(pds_screen_recorder)

set(CMAKE_CXX_STANDARD 14)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

include_directories(${GTK4_INCLUDE_DIRS})
link_directories(${GTK4_LIBRARY_DIRS})

add_definitions(${GTK4_CFLAGS_OTHER})

add_executable(pds_screen_recorder pds-screen-recorder/pds-screen-recorder.cpp)

target_link_libraries(pds_screen_recorder ${GTK4_LIBRARIES})