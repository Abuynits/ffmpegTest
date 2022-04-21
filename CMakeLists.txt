
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
add_executable(${PROJECT_NAME}
        main.cpp
        )

target_link_libraries(${PROJECT_NAME}
        PkgConfig::LIBAV
        )

#find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
#find_library(AVCODEC_LIBRARY avcodec)
#
#find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
#find_library(AVFORMAT_LIBRARY avformat)
#
#find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
#find_library(AVUTIL_LIBRARY avutil)
#
#find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
#find_library(AVDEVICE_LIBRARY avdevice)
#
#add_executable (ffmpegTest5 "main.cpp")
#target_include_directories(ffmpegTest5 PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR})
#target_link_libraries(ffmpegTest5 PRIVATE ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY})
#
#

