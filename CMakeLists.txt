cmake_minimum_required(VERSION 3.20)
project(pds_screen_recorder)

set(CMAKE_CXX_STANDARD 14)


if(UNIX)
    # GTK4
    find_package(GTK REQUIRED gtk4)
    include_directories(${GTK4_INCLUDE_DIRS})
    link_directories(${GTK4_LIBRARY_DIRS})
    add_definitions(${GTK4_CFLAGS_OTHER})
    # ffmpeg
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
        )
endif()
if(WIN32)
    find_package(GTK REQUIRED gtk4)
    set(VCPKG_INCLUDE_PATH C:/src/vcpkg/installed/x64-windows)
    set(GTK4_INCLUDE_DIRS "${VCPKG_INCLUDE_PATH}/include/gtk-4.0"
            "${VCPKG_INCLUDE_PATH}/include/gio-unix-2.0"
            "${VCPKG_INCLUDE_PATH}/include/cairo"
            "${VCPKG_INCLUDE_PATH}/include/pango-1.0"
            "${VCPKG_INCLUDE_PATH}/include/harfbuzz"
            "${VCPKG_INCLUDE_PATH}/include/pango-1.0"
            "${VCPKG_INCLUDE_PATH}/include/fribidi"
            "${VCPKG_INCLUDE_PATH}/include/harfbuzz"
            "${VCPKG_INCLUDE_PATH}/include/gdk-pixbuf-2.0"
            "${VCPKG_INCLUDE_PATH}/include/pixman-1"
            "${VCPKG_INCLUDE_PATH}/include/uuid"
            "${VCPKG_INCLUDE_PATH}/include/freetype2"
            "${VCPKG_INCLUDE_PATH}/include/libpng16"
            "${VCPKG_INCLUDE_PATH}/include/graphene-1.0"
            "${VCPKG_INCLUDE_PATH}/include/glib-2.0"
            "${VCPKG_INCLUDE_PATH}/lib/glib-2.0/include"
            "${VCPKG_INCLUDE_PATH}/lib/graphene-1.0/include")
    set(GTK4_LIBRARIES "gtk-4"
            "pangocairo-1.0"
            "pango-1.0"
            "harfbuzz"
            "gdk_pixbuf-2.0"
            "cairo-gobject"
            "graphene-1.0"
            "gio-2.0"
            "gobject-2.0"
            "glib-2.0")
    include_directories(${GTK4_INCLUDE_DIRS})
    link_directories("${VCPKG_INCLUDE_PATH}/lib" "${VCPKG_INCLUDE_PATH}/bin")
    find_package(FFMPEG REQUIRED)
    include_directories(${FFMPEG_INCLUDE_DIRS})
endif()
# SDL2
find_package(SDL2 REQUIRED)
include_directories(SDL2 ${SDL2_INCLUDE_DIRS})

add_executable(pds_screen_recorder src/main.cpp src/ScreenRecorder.cpp src/ScreenRecorder.h src/exceptions/fsException.h src/exceptions/dataException.h src/exceptions/avException.h src/ffmpeg/ffmpeg.h src/ffmpeg/ffmpegcpp.h)
if(UNIX)
    target_link_libraries(${PROJECT_NAME} ${GTK4_LIBRARIES} ${SDL2_LIBRARIES} PkgConfig::LIBAV)
endif()
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ${GTK4_LIBRARIES} ${SDL2_LIBRARIES} ${FFMPEG_LIBRARIES})
endif()