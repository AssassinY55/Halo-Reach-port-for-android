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

target_link_libraries(
        xbox360androidport
        ${log-lib}
)
