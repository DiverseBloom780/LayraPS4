cmake_minimum_required(VERSION 3.22)
project(LayraPS4 C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set include directories
include_directories(include)
include_directories(lib/imgui)
include_directories(lib/imgui/backends)
include_directories(src)
# include_directories(externals/toml11)
# Find SDL2
find_package(SDL2 REQUIRED)
#find_package(SDL2_image REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Add Vulkan SDK path if available (for Windows)
if(WIN32)
    # Assuming Vulkan SDK is installed in a standard location or specified by an environment variable
    if(NOT DEFINED ENV{VULKAN_SDK})
        message(FATAL_ERROR "VULKAN_SDK environment variable not set. Please install Vulkan SDK.")
    endif()
    set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
    include_directories(${VULKAN_SDK_PATH}/Include)
    link_directories(${VULKAN_SDK_PATH}/Lib)
endif()

# Add source files
file(GLOB_RECURSE CORE_SOURCES
    "src/*.cpp"
    "src/core/pkg/*.c"
    "src/core/vfs/*.c"
    "src/video/layra_vulkan.cpp"
)

set(IMGUI_SOURCES
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/backends/imgui_impl_sdl2.cpp
#    lib/imgui/backends/imgui_impl_opengl3.cpp # OpenGL3 backend is not used with Vulkan
    # Add Vulkan backend for ImGui
    lib/imgui/backends/imgui_impl_vulkan.cpp
)

add_executable(LayraPS4 ${CORE_SOURCES} ${IMGUI_SOURCES})

# Link necessary libraries
target_link_libraries(LayraPS4
    ${SDL2_LIBRARIES}

    vulkan
)

# Add Vulkan libraries for Windows
if(WIN32)
    target_link_libraries(LayraPS4
        vulkan-1
        SDL2main
        ${SDL2_LIBRARIES}
    )


    # Windows-specific compiler definitions from shadPS4
    target_compile_definitions(LayraPS4 PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        _SCL_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _TIMESPEC_DEFINED
        NTDDI_VERSION=0x0A000006
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00
    )
    # Disable ASLR (similar to shadPS4)
    # target_link_options(LayraPS4 PRIVATE /DYNAMICBASE:NO) # This might cause issues with modern Windows security
    # Increase stack commit area (similar to shadPS4)
    # target_link_options(LayraPS4 PRIVATE /STACK:0x200000,0x200000) # This can be adjusted if stack overflow issues occur

    # Add preprocessor definitions for Windows
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -DNOMINMAX)
    # Link against Win32 libraries for SDL2
    target_link_libraries(LayraPS4 user32 gdi32 shell32 advapi32)
endif()

# Add required include for Vulkan in ImGui
target_include_directories(LayraPS4 PRIVATE lib/imgui/backends)

# Add shadPS4's vk_platform.h and vk_instance.h to LayraPS4's include path temporarily for reference
# This is for study purposes and will be removed once the concepts are reimplemented.
# It's important to not directly copy these files into LayraPS4's source tree permanently.
#include_directories(/home/ubuntu/shadPS4/shadPS4-v.0.11.0/src/video_core/renderer_vulkan)

# Placeholder for other dependencies from shadPS4 (e.g., Boost, fmt, toml11, tsl-robin-map, xbyak, Tracy, RenderDoc, gcn, half, ZLIB, PNG, RenderDoc, SDL3, pugixml, stb, libusb, lfreist-hwinfo, VulkanHeaders, VulkanMemoryAllocator, xxHash, Zydis)
# These will be added as needed during the implementation of specific features.
# For now, focus on core Vulkan integration.
