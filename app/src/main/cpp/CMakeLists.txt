# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.
project("shujieplayer")

set(FFMPEG ${CMAKE_SOURCE_DIR}/ffmpeg) # ffmpeg的路径
set(RTMP ${CMAKE_SOURCE_DIR}/rtmp) # rtmp的路径

include_directories(${FFMPEG}/include) # 导入ffmpeg的头文件

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -L${FFMPEG}/libs/${CMAKE_ANDROID_ARCH_ABI}") # ffmpeg库指定

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -L${RTMP}/libs/${CMAKE_ANDROID_ARCH_ABI}") # rtmp库指定

# 批量导入 源文件
file(GLOB src_files *.cpp)


add_library( # Sets the name of the library.
        shujieplayer

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${src_files})

target_link_libraries( # Specifies the target library.
        shujieplayer

        # 忽略顺序的方式，导入
        -Wl,--start-group
        avcodec avfilter avformat avutil swresample swscale
        -Wl,--end-group

        log # 日志库，打印日志用的
        z # libz.so库，是FFmpeg需要用ndk的z库，FFMpeg需要额外支持  libz.so
        rtmp # rtmp 后面会专门介绍
        android # android ANativeWindow 用来渲染画面的
        OpenSLES # 用来播放声音的
        )