_# LayraPS4 Emulator Project
# SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.22.1)
project(LayraPS4 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2 and its required libraries
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Find Vulkan SDK
find_package(Vulkan REQUIRED)

# Find ImGui dependencies
find_package(ImGui REQUIRED)

# Find TOML library
find_package(toml11 REQUIRED)

# Define source files
file(GLOB_RECURSE LAYRAPS4_SOURCES
    "src/*.cpp"
    "src/*.h"
    "src/common/*.cpp"
    "src/common/*.h"
    "src/core/*.cpp"
    "src/core/*.h"
    "src/core/networking/*.cpp"
    "src/core/networking/*.h"
    "src/core/libraries/network/*.cpp"
    "src/core/libraries/network/*.h"
    "src/core/libraries/np/*.cpp"
    "src/core/libraries/np/*.h"
)

# Add include directories
include_directories(
    src
    ${SDL2_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
    ${ImGui_INCLUDE_DIRS}
    ${toml11_INCLUDE_DIRS}
)

# Add the executable
add_executable(LayraPS4 ${LAYRAPS4_SOURCES})

# Link libraries
target_link_libraries(LayraPS4
    PRIVATE
    SDL2::SDL2
    SDL2::SDL2_image
    SDL2::SDL2_ttf
    Vulkan::Vulkan
    ImGui::ImGui
    toml11::toml11
)

# Set output directory
set_target_properties(LayraPS4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Add a placeholder for VFS implementation
# This will be replaced with actual VFS code in a later phase
add_custom_target(VFS_PLACEHOLDER ALL)_
