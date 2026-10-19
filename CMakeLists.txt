cmake_minimum_required(VERSION 3.20)
project(StreamPlanner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Núcleo portable (C++ estándar, sin nada de Windows) ---
add_library(sp_core STATIC src/core/model.cpp)
target_include_directories(sp_core PUBLIC src)
if(MSVC)
  target_compile_options(sp_core PRIVATE /utf-8 /W4)
endif()

# --- Interfaz Win32 ---
if(WIN32)
  add_executable(StreamPlanner WIN32 src/platform/win32/main.cpp)
  target_link_libraries(StreamPlanner PRIVATE sp_core user32 gdi32 comctl32)
  target_compile_definitions(StreamPlanner PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
  if(MSVC)
    target_compile_options(StreamPlanner PRIVATE /utf-8 /W4)
  endif()
endif()