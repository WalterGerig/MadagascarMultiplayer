cmake_minimum_required(VERSION 3.20)
project(MadagascarMultiplayer LANGUAGES CXX)

# Enforce 32-bit x86 compilation (Madagascar Game.exe is a 32-bit x86 PE executable)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Madagascar Multiplayer MUST be built for 32-bit x86! Please configure using '-A Win32'.")
endif()

# Global build settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Add subprojects:
# 1. client/ -> Builds MadMultiplayer.dll (The core multiplayer mod & in-game overlay)
add_subdirectory(client)

# 2. loader/ -> Builds d3d8.dll (Proxy loader that auto-loads patches/MadMultiplayer.dll on game launch)
add_subdirectory(loader)
