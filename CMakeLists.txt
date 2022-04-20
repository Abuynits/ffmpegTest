
cmake_minimum_required(VERSION 3.21)
project(ffmpegTest5)

set(CMAKE_CXX_STANDARD 14)

add_executable(ffmpegTest5 main.cpp)



include_directories("/Users/abuynits/CLionProjects/ffmpegTest5/ffmpeg/")
target_link_libraries(ffmpegTest5 "/Users/abuynits/CLionProjects/ffmpegTest5/ffmpeg/")