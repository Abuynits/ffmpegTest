
cmake_minimum_required(VERSION 3.21)
project(ffmpegTest5)

set(CMAKE_CXX_STANDARD 14)

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
add_executable (ffmpegTest5 "main.cpp")
target_link_libraries(ffmpegTest5 PkgConfig::LIBAV)

