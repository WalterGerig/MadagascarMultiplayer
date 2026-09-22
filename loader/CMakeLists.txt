cmake_minimum_required(VERSION 3.20)
project(MadLoader CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "Configure with a 32-bit toolchain (vcvars32, or -A Win32)")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- winmm.dll : proxy route (needs the app dir to win the DLL search order)
add_library(madloader_proxy SHARED src/proxy_winmm.cpp src/loader.cpp src/winmm.def)
set_target_properties(madloader_proxy PROPERTIES
  OUTPUT_NAME "winmm" PREFIX "" IMPORT_SUFFIX "_impl.lib")
target_compile_options(madloader_proxy PRIVATE /wd4273)

# --- d3d8.dll : proxy route that actually works for this game.
#     winmm cannot be used: System32 winmm is already resident before Game.exe's
#     import is resolved, so the loader reuses it by base name (confirmed live).
#     d3d8 is the first import descriptor and nothing preloads it.
add_library(madloader_d3d8 SHARED src/proxy_d3d8.cpp src/loader.cpp src/d3d8.def)
set_target_properties(madloader_d3d8 PROPERTIES
  OUTPUT_NAME "d3d8" PREFIX "" IMPORT_SUFFIX "_impl.lib")

# --- MadLoaderCore.dll : injection route (immune to search order)
add_library(madloader_core SHARED src/core_dllmain.cpp src/loader.cpp)
set_target_properties(madloader_core PROPERTIES
  OUTPUT_NAME "MadLoaderCore" PREFIX "")

# --- MadLauncher.exe : starts the game suspended and injects the core
add_executable(madlauncher src/launcher.cpp)
set_target_properties(madlauncher PROPERTIES
  OUTPUT_NAME "MadLauncher")

foreach(t madloader_proxy madloader_d3d8 madloader_core madlauncher)
  target_include_directories(${t} PRIVATE src)
  target_compile_definitions(${t} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
endforeach()
