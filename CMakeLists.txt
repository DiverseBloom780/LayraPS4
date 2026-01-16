# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(LayraPS4 VERSION 0.1.0 LANGUAGES C CXX)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Options
option(BUILD_WITH_VULKAN "Build with Vulkan support" ON)
option(BUILD_WITH_OPENGL "Build with OpenGL support" OFF)
option(ENABLE_LOGGING "Enable detailed logging" ON)

# Find packages
find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

# Add subdirectories
add_subdirectory(lib/imgui)
add_subdirectory(src)

# Configure header for version info
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.h
)

# Add binary directory to include path
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# Summary
message(STATUS "==========================================")
message(STATUS "LayraPS4 Configuration Summary")
message(STATUS "==========================================")
message(STATUS "Build Type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "Vulkan Support:      ${BUILD_WITH_VULKAN}")
message(STATUS "OpenGL Support:      ${BUILD_WITH_OPENGL}")
message(STATUS "Detailed Logging:    ${ENABLE_LOGGING}")
message(STATUS "SDL3 Found:          YES")
message(STATUS "Vulkan Found:        YES")
message(STATUS "==========================================")
