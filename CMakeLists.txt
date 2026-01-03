cmake_minimum_required(VERSION 3.24)
project(LayraPS4 C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.26100.0")
# ----------  search / include dirs  ----------
include_directories(include lib/imgui lib/imgui/backends)

# ----------  SDL3  ----------
set(SDL3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/include")
set(SDL3_LIBRARY   "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/x64/SDL3.lib")

find_path(SDL3_INCLUDE_DIR SDL3/SDL.h
          PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/include
                ${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3
                ${CMAKE_CURRENT_SOURCE_DIR}/SDL3/include)
find_library(SDL3_LIBRARY NAMES SDL3 SDL3-3.0
             PATHS  ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/x64
                     ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/lib
                     ${CMAKE_CURRENT_SOURCE_DIR}/SDL3/lib/x64)
if(NOT SDL3_INCLUDE_DIR OR NOT SDL3_LIBRARY)
    message(FATAL_ERROR "SDL3 not found – check lib/SDL3")
endif()
set(SDL3_INCLUDE_DIRS ${SDL3_INCLUDE_DIR})
set(SDL3_LIBRARIES    ${SDL3_LIBRARY})

# ----------  Vulkan  ----------
if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    include_directories(${VULKAN_SDK_PATH}/Include)
    link_directories(${VULKAN_SDK_PATH}/Lib)
else()
    message(WARNING "VULKAN_SDK not set – continuing without Vulkan")
endif()

# ----------  source file lists  ----------
file(GLOB_RECURSE C_SOURCES   src/core/pkg/*.c  src/core/vfs/*.c)
file(GLOB_RECURSE CXX_SOURCES src/*.cpp src/video/layra_vulkan.cpp)

set(BOOT_SOURCES src/boot/boot_screen.cpp)

set(IMGUI_SOURCES
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/backends/imgui_impl_sdl3.cpp   # SDL3 backend
    lib/imgui/backends/imgui_impl_vulkan.cpp
)

# ----------  core static lib  ----------
add_library(layra_core STATIC ${C_SOURCES})
target_include_directories(layra_core PUBLIC include ${SDL3_INCLUDE_DIRS})
target_link_libraries(layra_core PUBLIC ${SDL3_LIBRARIES})

# ----------  main executable (single definition)  ----------
add_executable(LayraPS4
    src/main.cpp          # always present entry point
    ${CXX_SOURCES}
    ${IMGUI_SOURCES}
    ${BOOT_SOURCES}
)
target_include_directories(LayraPS4 PRIVATE include ${SDL3_INCLUDE_DIRS} src/boot)
target_link_libraries(LayraPS4 PRIVATE layra_core)

# ----------  Windows specifics  ----------
if(WIN32)
    target_link_libraries(LayraPS4 PRIVATE
        vulkan-1 ${SDL3_LIBRARIES} SDL3main user32 gdi32 shell32 advapi32)
    target_compile_definitions(LayraPS4 PRIVATE
        _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE
        _SCL_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ----------  icon (optional)  ----------
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/layra_logo.ico")
    enable_language(RC)
    target_sources(LayraPS4 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/resources/layra_logo.rc")
endif()
