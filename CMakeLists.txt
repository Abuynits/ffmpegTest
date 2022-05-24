#set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_VERBOSE_MAKEFILE on)
cmake_minimum_required(VERSION 3.21)
project(ffmpegTest5)

set(CMAKE_CXX_STANDARD 14)

set(FFMPEG_DIR /usr/local/Cellar/ffmpeg/5.0.1)
include_directories(${FFMPEG_DIR}/include/)
link_directories(${FFMPEG_DIR}/lib/)

add_executable(ffmpegTest5 main.cpp AudioDecoder.cpp AudioDecoder.h AudioFilter.cpp AudioFilter.h OutputAnalysis.cpp OutputAnalysis.h Resampler.cpp Resampler.h main2.cpp main2.h)
target_link_libraries(ffmpegTest5 avformat avcodec avutil avfilter swresample)
# libbar.so is found in ${CMAKE_SOURCE_DIR}/lib
#
#add_executable(ffmpegBrewTest main.cpp)
#
#list(APPEND CMAKE_PREFIX_PATH "/usr/local/Cellar/ffmpeg/5.0.1")
#
##find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
##find_library(AVCODEC_LIBRARY avcodec)
#
#find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h
#
#        )
#find_library(AVFORMAT_LIBRARY avformat
#        )
##
##find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
##find_library(AVUTIL_LIBRARY avutil)
##
##find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
##find_library(AVDEVICE_LIBRARY avdevice)
#
##include_directories(${AVCODEC_INCLUDE_DIR})
#include_directories(${AVFORMAT_INCLUDE_DIR})
##include_directories(${AVUTIL_INCLUDE_DIR})
##include_directories(${AVDEVICE_INCLUDE_DIR})
##link_libraries(${AVCODEC_LIBRARY})
#link_libraries(${AVFORMAT_LIBRARY})
##link_libraries(${AVUTIL_LIBRARY})
##link_libraries(${AVDEVICE_LIBRARY})