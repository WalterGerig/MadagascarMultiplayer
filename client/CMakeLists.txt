cmake_minimum_required(VERSION 3.20)
project(MadMultiplayer LANGUAGES CXX)

# Erzwinge 32-Bit x86 Toolchain
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "MadMultiplayer MUSS als 32-Bit x86 kompiliert werden!")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/d3d8
)

set(SOURCES
    src/dllmain.cpp
    src/MemoryManager.cpp
    src/D3D8Hook.cpp
    src/Logger.cpp
    src/Config.cpp

    # Dear ImGui Core
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp

    # Dear ImGui Win32 & DX8 Backends
    vendor/imgui/imgui_impl_win32.cpp
    vendor/imgui/imgui_impl_dx8.cpp
)

set(HEADERS
    include/MemoryManager.h
    include/PlayerTransform.h
    include/NetworkProtocol.h
    include/D3D8Hook.h
    include/Logger.h
    include/Config.h

    vendor/d3d8/d3d8_minimal.h
    vendor/imgui/imgui.h
    vendor/imgui/imgui_impl_win32.h
    vendor/imgui/imgui_impl_dx8.h
)

add_library(MadMultiplayer SHARED ${SOURCES} ${HEADERS})

set_target_properties(MadMultiplayer PROPERTIES
    OUTPUT_NAME "MadMultiplayer"
    PREFIX ""
    SUFFIX ".dll"
)

target_compile_definitions(MadMultiplayer PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(MadMultiplayer PRIVATE
    kernel32
    user32
    gdi32
    psapi
)
