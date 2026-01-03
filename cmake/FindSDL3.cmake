# FindSDL3.cmake - Find SDL3 library for Windows/Linux
# This module finds SDL3 library and include directories
find_path(SDL3_INCLUDE_DIR SDL3/SDL.h
    PATHS
    C:/SDL3/include
    C:/Libraries/SDL3/include
    ${SDL3_ROOT}/include
    $ENV{SDL3_ROOT}/include
    /usr/include/SDL3
    /usr/local/include/SDL3
    DOC "Path to SDL3 include directory"
)

find_library(SDL3_LIBRARY 
    NAMES SDL3-3.0 SDL3
    PATHS
    C:/SDL3/lib/x64
    C:/SDL3/lib
    C:/Libraries/SDL3/lib/x64
    C:/Libraries/SDL3/lib
    ${SDL3_ROOT}/lib/x64
    ${SDL3_ROOT}/lib
    $ENV{SDL3_ROOT}/lib/x64
    $ENV{SDL3_ROOT}/lib
    /usr/lib/x86_64-linux-gnu
    /usr/local/lib
    DOC "Path to SDL3 library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL3 DEFAULT_VARS SDL3_LIBRARY SDL3_INCLUDE_DIR)

if(SDL3_FOUND)
    set(SDL3_LIBRARIES ${SDL3_LIBRARY})
    set(SDL3_INCLUDE_DIRS ${SDL3_INCLUDE_DIR})
    message(STATUS "Found SDL3:")
    message(STATUS "  Include: ${SDL3_INCLUDE_DIR}")
    message(STATUS "  Library: ${SDL3_LIBRARY}")
endif()

mark_as_advanced(SDL3_LIBRARY SDL3_INCLUDE_DIR)