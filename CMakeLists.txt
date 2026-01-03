cmake_minimum_required(VERSION 3.24)
project(LayraPS4 C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Windows SDK for Visual Studio 2022
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.26100.0")

# Set include directories
include_directories(include)
include_directories(lib/imgui)
include_directories(lib/imgui/backends)

# SDL3 Configuration - EXACT PATH FOR YOUR SETUP
set(SDL3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/include")
set(SDL3_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/x64/SDL3.lib")

# Debug: Show what we're looking for
message(STATUS "Looking for SDL3 at:")
message(STATUS "  Include: ${SDL3_INCLUDE_DIR}/SDL3/SDL.h")
message(STATUS "  Library: ${SDL3_LIBRARY}")

# Check if SDL3 exists at the exact location
if(EXISTS "${SDL3_INCLUDE_DIR}/SDL3/SDL.h")
    set(SDL3_FOUND TRUE)
    set(SDL3_INCLUDE_DIRS ${SDL3_INCLUDE_DIR})
    set(SDL3_LIBRARIES ${SDL3_LIBRARY})
    message(STATUS "✅ Found SDL3 for Windows:")
    message(STATUS "  Include: ${SDL3_INCLUDE_DIRS}")
    message(STATUS "  Library: ${SDL3_LIBRARIES}")
else()
    # Let's try to find SDL3 in alternative locations
    message(STATUS "❌ SDL3 not found at primary location, searching alternatives...")
    
    # Check if we have SDL3 headers at all
    find_path(SDL3_INCLUDE_DIR SDL3/SDL.h
        PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL3/include
        DOC "SDL3 include directory"
    )
    
    find_library(SDL3_LIBRARY 
        NAMES SDL3-3.0 SDL3
        PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/x64
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL3/lib/x64
        ${CMAKE_CURRENT_SOURCE_DIR}/arm64
        DOC "SDL3 library"
    )
    
    if(SDL3_INCLUDE_DIR AND SDL3_LIBRARY)
        set(SDL3_FOUND TRUE)
        set(SDL3_INCLUDE_DIRS ${SDL3_INCLUDE_DIR})
        set(SDL3_LIBRARIES ${SDL3_LIBRARY})
        message(STATUS "✅ Found SDL3 at alternative location:")
        message(STATUS "  Include: ${SDL3_INCLUDE_DIRS}")
        message(STATUS "  Library: ${SDL3_LIBRARIES}")
    else()
        message(FATAL_ERROR "❌ SDL3 not found! Please check your SDL3 installation.")
    endif()
endif()

# Vulkan SDK for Windows
if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
    include_directories(${VULKAN_SDK_PATH}/Include)
    link_directories(${VULKAN_SDK_PATH}/Lib)
else()
    message(WARNING "⚠ VULKAN_SDK not set - continuing without Vulkan")
endif()

# C sources (VFS, PKG, etc.)
file(GLOB_RECURSE C_SOURCES
    "src/core/pkg/*.c"
    "src/core/vfs/*.c"
)

# C++ sources (main, video, etc.)
file(GLOB_RECURSE CXX_SOURCES
    "src/*.cpp"
    "src/video/layra_vulkan.cpp"
)

# Boot system sources
set(BOOT_SOURCES
    src/boot/boot_screen.cpp
)

# ImGui sources - FIXED: Use SDL3 backend instead of SDL2
set(IMGUI_SOURCES
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/backends/imgui_impl_sdl3.cpp      # Changed from SDL2 to SDL3
    lib/imgui/backends/imgui_impl_vulkan.cpp
)

# Core library for C modules (VFS, PKG, etc.)
add_library(layra_core STATIC ${C_SOURCES})
target_include_directories(layra_core PUBLIC include ${SDL3_INCLUDE_DIRS})
target_link_libraries(layra_core PUBLIC ${SDL3_LIBRARIES})

# Main executable - FIXED: Single definition with all sources
add_executable(LayraPS4 ${CXX_SOURCES} ${IMGUI_SOURCES} ${BOOT_SOURCES})
target_include_directories(LayraPS4 PUBLIC include ${SDL3_INCLUDE_DIRS} src/boot)
target_link_libraries(LayraPS4 PRIVATE layra_core)

# Windows-specific configurations
if(WIN32)
    target_link_libraries(LayraPS4 PRIVATE 
        vulkan-1 
        ${SDL3_LIBRARIES}
        SDL3main                                      # Added SDL3 main library
        user32 
        gdi32 
        shell32 
        advapi32
    )
    target_compile_definitions(LayraPS4 PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        _SCL_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Add required include for Vulkan in ImGui
target_include_directories(LayraPS4 PRIVATE lib/imgui/backends)

# Windows resources and boot system
if(WIN32)
    # Application icon for Windows executable
    set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/layra_logo.ico")
    
    # Add Windows resource file
    if(EXISTS "${APP_ICON}")
        enable_language(RC)
        target_sources(LayraPS4 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/resources/layra_logo.rc")
        message(STATUS "✅ Windows icon resource added")
    else()
        message(WARNING "⚠ Application icon not found - using default Windows icon")
    endif()
endif()
