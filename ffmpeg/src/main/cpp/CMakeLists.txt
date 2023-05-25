cmake_minimum_required(VERSION 3.10.2)
project("cxyPlayer")

include_directories(include)
include_directories(soundtouch/include)
include_directories(soundtouch/SoundTouch)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}")

file(GLOB cpp_source *.cpp)

# 这一句用来导出编译生成的so库
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/jniLibs/${ANDROID_ABI})

aux_source_directory(./soundtouch/SoundTouch soundSource)


add_library(cxyPlayer SHARED ${cpp_source} ${soundSource})


target_link_libraries(
        cxyPlayer
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
        log
        OpenSLES
        android)