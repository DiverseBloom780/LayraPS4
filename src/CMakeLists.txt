# LayraPS4/src/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(LayraPS4 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Core Emulator Library
add_library(layra_core STATIC
    # Core CPU Emulation
    core/cpu/jaguar_core.cpp
    core/cpu/x64_decoder.cpp
    core/cpu/cpu_manager.cpp
    
    # Memory Management
    core/memory/memory_manager.cpp
    core/memory/mmu.cpp
    core/memory/unified_memory.cpp
    
    # OS Emulation Layer
    core/os/orbis_system.cpp
    core/os/syscalls/syscall_dispatcher.cpp
    core/os/modules/module_parser.cpp
    core/os/modules/module_loader.cpp
    
    # System Management
    core/system/emulator.cpp
    core/system/scheduler.cpp
    core/system/interrupts.cpp
    
    # Hardware Devices
    core/devices/apu.cpp
    core/devices/southbridge.cpp
    core/devices/device_manager.cpp
    
    # PKG Handling (existing)
    core/pkg/layra_pkg.c
    
    # Add more files as you create them
)

# Set properties for the core library
set_target_properties(layra_core PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Include directories for core library
target_include_directories(layra_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cpu
    ${CMAKE_CURRENT_SOURCE_DIR}/core/memory
    ${CMAKE_CURRENT_SOURCE_DIR}/core/os
    ${CMAKE_CURRENT_SOURCE_DIR}/core/system
    ${CMAKE_CURRENT_SOURCE_DIR}/core/devices
    ${CMAKE_CURRENT_SOURCE_DIR}/core/pkg
)

# Main Executable
add_executable(layra_ps4
    main.cpp
    # Vulkan renderer (existing)
    layra_vulkan.cpp
)

# Set properties for main executable
set_target_properties(layra_ps4 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "LayraPS4"
    WIN32_EXECUTABLE ON
)

# Include directories for main executable
target_include_directories(layra_ps4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/imgui/backends
)

# Link libraries
target_link_libraries(layra_ps4 PRIVATE
    layra_core
    # SDL3
    SDL3::SDL3
    # Vulkan
    Vulkan::Vulkan
    # ImGui
    imgui
)

# Copy resources to build directory
file(GLOB_RECURSE RESOURCE_FILES 
    ${CMAKE_SOURCE_DIR}/resources/*.png
    ${CMAKE_SOURCE_DIR}/resources/*.wav
    ${CMAKE_SOURCE_DIR}/resources/*.ttf
)

foreach(resource ${RESOURCE_FILES})
    file(RELATIVE_PATH rel_path ${CMAKE_SOURCE_DIR} ${resource})
    get_filename_component(dir ${rel_path} DIRECTORY)
    add_custom_command(TARGET layra_ps4 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/${dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${resource}" "${CMAKE_BINARY_DIR}/bin/${rel_path}"
        COMMENT "Copying resource: ${rel_path}"
    )
endforeach()

# Installation (optional)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Installation directory" FORCE)
endif()

install(TARGETS layra_ps4
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
    DESTINATION resources
    PATTERN "*.md" EXCLUDE
)

# Testing configuration (optional - for future)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Package configuration (optional)
include(CPack)
