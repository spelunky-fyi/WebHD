cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
project(WebHD)

add_subdirectory(3rdparty)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

option(DEV "Build with local development features" OFF)
if(DEV)
    add_compile_definitions(DEV)
endif()

add_library(webhd SHARED
    dllmain.cpp
    webhd.h webhd.cpp
)
target_link_libraries(webhd PUBLIC hddll)
