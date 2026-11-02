cmake_minimum_required(VERSION 3.22.1)
project("xbox360androidport")

add_library(
        xbox360androidport
        SHARED
        src/main/cpp/native-lib.cpp
)

find_library(
        log-lib
        log
)

find_library(
        gles-lib
        GLESv2
)

find_library(
        android-lib
        android
)

target_link_libraries(
        xbox360androidport
        ${android-lib}
        ${gles-lib}
        ${log-lib}
)
