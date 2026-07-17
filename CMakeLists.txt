cmake_minimum_required(VERSION 3.24)
project(LayraPS4 VERSION 0.1 LANGUAGES C CXX)

# Language standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.26100.0")

# Project root
set(LAYRA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# SDL3 shipped in lib/SDL3 (adjust arch folder if you support multiple)
set(SDL3_ROOT ${LAYRA_ROOT}/lib/SDL3)
set(SDL3_INCLUDE_DIR ${SDL3_ROOT}/include)
set(SDL3_LIB_PATH ${SDL3_ROOT}/x64/SDL3.lib)
set(SDL3_DLL_PATH ${SDL3_ROOT}/x64/SDL3.dll)

# Vulkan
find_package(Vulkan REQUIRED)

# Sources (GLOB kept for convenience, but critical files added explicitly)
file(GLOB CORE_SOURCES
    ${LAYRA_ROOT}/src/core/pkg/*.cpp
    ${LAYRA_ROOT}/src/core/pkg/*.c
    ${LAYRA_ROOT}/src/core/memory/*.cpp
    ${LAYRA_ROOT}/src/core/memory/*.c
    ${LAYRA_ROOT}/src/core/services/*.cpp
    ${LAYRA_ROOT}/src/core/services/*.c
    ${LAYRA_ROOT}/src/core/loader/*.cpp
    ${LAYRA_ROOT}/src/core/loader/*.c
    ${LAYRA_ROOT}/src/core/kernel/*.cpp
    ${LAYRA_ROOT}/src/core/kernel/*.c
    ${LAYRA_ROOT}/src/core/file_sys/*.cpp
    ${LAYRA_ROOT}/src/core/filesys/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/kernel/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/libc/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/videoout/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/gnmdriver/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/network/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/np/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/pad/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/system/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/sysmodule/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/app_content/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/audioout/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/nptrophy/*.cpp
    ${LAYRA_ROOT}/src/core/libraries/savedata/*.cpp
    ${LAYRA_ROOT}/src/common/*.cpp
    ${LAYRA_ROOT}/src/core/*.cpp
    ${LAYRA_ROOT}/src/core/*.c
)

# Explicitly add critical files (in case GLOB misses them or cache is stale)
list(APPEND CORE_SOURCES
    ${LAYRA_ROOT}/src/core/loader/elf_loader.cpp
    ${LAYRA_ROOT}/src/core/loader/cpu_patcher.cpp
    ${LAYRA_ROOT}/src/core/kernel/module_manager.cpp
    ${LAYRA_ROOT}/src/core/kernel/kernel_manager.cpp
    ${LAYRA_ROOT}/src/core/kernel/kernel_memory.c
    ${LAYRA_ROOT}/src/core/kernel/syscalls.cpp
    ${LAYRA_ROOT}/src/core/libraries/kernel/libkernel.cpp
    ${LAYRA_ROOT}/src/core/libraries/libc/libc.cpp
    ${LAYRA_ROOT}/src/core/file_sys/fs.cpp
    ${LAYRA_ROOT}/src/common/io_file.cpp
    ${LAYRA_ROOT}/src/core/memory/memory_manager.cpp
    ${LAYRA_ROOT}/src/core/memory/memory_manager_bridge.cpp
    ${LAYRA_ROOT}/src/core/memory/address_space.cpp
    ${LAYRA_ROOT}/src/core/orbis_system.cpp
    ${LAYRA_ROOT}/src/emulator.cpp
)

# Remove duplicates
list(REMOVE_DUPLICATES CORE_SOURCES)

# Remove platform-specific and non-functional modules
if(WIN32)
    list(FILTER CORE_SOURCES EXCLUDE REGEX ".*unix_sockets\\.cpp$")
    list(FILTER CORE_SOURCES EXCLUDE REGEX ".*posix_sockets\\.cpp$")
endif()

# Exclude network module (not yet adapted from shadPS4 reference)
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/libraries/network/.*")
# Exclude old filesys directory (conflicts with file_sys/fs.h HandleTable)
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/core/filesys/.*")

file(GLOB APP_SOURCES
    ${LAYRA_ROOT}/src/*.cpp
    ${LAYRA_ROOT}/src/video/*.cpp
    ${LAYRA_ROOT}/src/gui/*.cpp
)
list(FILTER APP_SOURCES EXCLUDE REGEX ".*vfs.*")

if(WIN32)
    list(FILTER APP_SOURCES EXCLUDE REGEX ".*unix_sockets\\.cpp$")
    list(FILTER APP_SOURCES EXCLUDE REGEX ".*posix_sockets\\.cpp$")
endif()

set(IMGUI_SOURCES
    ${LAYRA_ROOT}/lib/imgui/imgui.cpp
    ${LAYRA_ROOT}/lib/imgui/imgui_draw.cpp
    ${LAYRA_ROOT}/lib/imgui/imgui_widgets.cpp
    ${LAYRA_ROOT}/lib/imgui/imgui_tables.cpp
    ${LAYRA_ROOT}/lib/imgui/imgui_demo.cpp
    ${LAYRA_ROOT}/lib/imgui/backends/imgui_impl_sdl3.cpp
    ${LAYRA_ROOT}/lib/imgui/backends/imgui_impl_vulkan.cpp
)

set(BOOT_SOURCES
    ${LAYRA_ROOT}/src/boot/boot_screen.cpp
)

# Debug: Print what files are being compiled
message(STATUS "=== Core Sources ===")
foreach(src ${CORE_SOURCES})
    message(STATUS "  ${src}")
endforeach()

# Create an imported SDL3 target if the shipped lib exists
if(EXISTS "${SDL3_LIB_PATH}" AND EXISTS "${SDL3_INCLUDE_DIR}")
    add_library(SDL3::SDL3 UNKNOWN IMPORTED)
    set_target_properties(SDL3::SDL3 PROPERTIES
        IMPORTED_LOCATION "${SDL3_LIB_PATH}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
    )
else()
    message(WARNING "SDL3 not found at ${SDL3_ROOT}; consider installing SDL3 or providing a CMake config")
endif()

# Core library
add_library(layra_core STATIC ${CORE_SOURCES})

# CRITICAL: Include directories for layra_core
target_include_directories(layra_core PUBLIC
    ${LAYRA_ROOT}/src
    ${LAYRA_ROOT}/src/gui
    ${LAYRA_ROOT}/include
    ${LAYRA_ROOT}/lib/imgui              # ImGui main directory
    ${LAYRA_ROOT}/lib/imgui/backends     # ImGui backends
)

# Link dependencies for layra_core
if(TARGET Vulkan::Vulkan)
    target_link_libraries(layra_core PUBLIC Vulkan::Vulkan)
endif()

if(TARGET SDL3::SDL3)
    target_link_libraries(layra_core PRIVATE SDL3::SDL3)
endif()

if(WIN32)
    target_compile_definitions(layra_core PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _AMD64_
    )
endif()

# Main executable
add_executable(LayraPS4
    ${LAYRA_ROOT}/src/main.cpp
    ${APP_SOURCES}
    ${IMGUI_SOURCES}
    ${BOOT_SOURCES}
)

# Remove duplicates if explicitly added
list(REMOVE_ITEM APP_SOURCES ${LAYRA_ROOT}/src/main.cpp)
list(REMOVE_ITEM APP_SOURCES ${LAYRA_ROOT}/src/boot/boot_screen.cpp)

# CRITICAL: Include directories for main executable
target_include_directories(LayraPS4 PRIVATE
    ${LAYRA_ROOT}/src
    ${LAYRA_ROOT}/src/boot
    ${LAYRA_ROOT}/src/gui
    ${LAYRA_ROOT}/include
    ${LAYRA_ROOT}/lib/imgui              # ImGui main directory
    ${LAYRA_ROOT}/lib/imgui/backends     # ImGui backends
)

target_link_libraries(LayraPS4 PRIVATE layra_core)

if(TARGET Vulkan::Vulkan)
    target_link_libraries(LayraPS4 PRIVATE Vulkan::Vulkan)
endif()

if(TARGET SDL3::SDL3)
    target_link_libraries(LayraPS4 PRIVATE SDL3::SDL3)
endif()

# Windows specific settings
if(WIN32)
    target_sources(LayraPS4 PRIVATE ${LAYRA_ROOT}/resources/layra_logo.rc)

    target_link_libraries(LayraPS4 PRIVATE
        user32
        gdi32
        shell32
        advapi32
    )

    target_compile_definitions(LayraPS4 PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )

    # Copy SDL3.dll to the output directory if it exists
    if(EXISTS "${SDL3_DLL_PATH}")
        add_custom_command(TARGET LayraPS4 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_DLL_PATH}"
            $<TARGET_FILE_DIR:LayraPS4>
        )
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "=== LayraPS4 Build Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Vulkan Found: ${Vulkan_FOUND}")
message(STATUS "SDL3 Library: ${SDL3_LIB_PATH}")
message(STATUS "ImGui Directory: ${LAYRA_ROOT}/lib/imgui")
message(STATUS "=====================================")
message(STATUS "")