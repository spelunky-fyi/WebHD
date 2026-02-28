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
    net.h net.cpp
    game_mode.h game_mode.cpp
    interactions.h interactions.cpp
    ui/settings.h ui/settings.cpp
    ui/log.h ui/log.cpp
    ui/game_mode_browser.h ui/game_mode_browser.cpp
    ui/toast.h ui/toast.cpp
    game_modes/vs_chat.h game_modes/vs_chat.cpp
)
target_link_libraries(webhd PUBLIC hddll ixwebsocket nlohmann_json::nlohmann_json)
