
cmake_minimum_required(VERSION 3.21)
project(ffmpegTest5)

set(CMAKE_CXX_STANDARD 14)

add_executable(ffmpegTest5 main.cpp)
#/Users/abuynits/CLionProjects/ffmpegTest5/ffmpeg/libavcodec/avcodec.h
find_path(
        AVCODEC_INCLUDE_DIR libavcodec/avcodec.h
        NAMES libavcodec/avcodec.h
)
if (NOT AVCODEC_INCLUDE_DIR)
    message(FATAL_ERROR "Can't find directory that contains avcodec.h")
else ()
    message("found path")
    find_library(AVCODEC_LIB libavcodec)
endif ()
find_library(AVCODEC_LIBRARY avcodec)

target_include_directories(ffmpegTest5 PRIVATE ${AVCODEC_INCLUDE_DIR})
target_link_libraries(ffmpegTest5 PRIVATE ${AVCODEC_LIBRARY})