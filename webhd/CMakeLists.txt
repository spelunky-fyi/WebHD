cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
project(WebHD)

add_subdirectory(3rdparty)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

option(DEV "Build with local development features" OFF)
if(DEV)
    add_compile_definitions(DEV)
endif()

# Embed target.png as a C byte array
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../assets/target.png" TARGET_PNG_HEX HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," TARGET_PNG_C "${TARGET_PNG_HEX}")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/ui/target_png.h"
  "#pragma once\n#include <cstddef>\nstatic const unsigned char kTargetPng[] = {${TARGET_PNG_C}};\nstatic const size_t kTargetPngSize = sizeof(kTargetPng);\n")

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
    ui/target_texture.h ui/target_texture.cpp
    game_modes/vs_chat.h game_modes/vs_chat.cpp
)
target_include_directories(webhd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stb
)
target_link_libraries(webhd PUBLIC hddll ixwebsocket nlohmann_json::nlohmann_json)
