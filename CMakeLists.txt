cmake_minimum_required(VERSION 3.22)
project(LayraPS4 C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set include directories
include_directories(include)
include_directories(lib/imgui)
include_directories(lib/imgui/backends)
include_directories(src)

# SDL2 (prebuilt, local, Windows only)
include_directories(${CMAKE_SOURCE_DIR}/SDL2/include)
link_directories(${CMAKE_SOURCE_DIR}/SDL2/lib)

# Add Vulkan SDK path if available (for Windows)
if(WIN32)
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
    lib/imgui/backends/imgui_impl_vulkan.cpp
)

add_executable(LayraPS4 ${CORE_SOURCES} ${IMGUI_SOURCES})

# Link necessary libraries
target_link_libraries(LayraPS4
    $<$<CONFIG:Debug>:SDL2d>
    $<$<CONFIG:Release>:SDL2>
    SDL2main
    vulkan-1
    user32
    gdi32
    shell32
    advapi32
)

# Windows-specific compiler definitions
if(WIN32)
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
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -DNOMINMAX)
endif()

# Add required include for Vulkan in ImGui
target_include_directories(LayraPS4 PRIVATE lib/imgui/backends)

# Optional: copy SDL2.dll after build so the exe runs without manual copying
add_custom_command(TARGET LayraPS4 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/SDL2/lib/SDL2.dll
        $<TARGET_FILE_DIR:LayraPS4>)
if(WIN32)
    set(VULKAN_SDK_PATH "C:\VulkanSDK\1.4.328.1")
    include_directories(${VULKAN_SDK_PATH}/Include)
    link_directories(${VULKAN_SDK_PATH}/Lib)
endif()
