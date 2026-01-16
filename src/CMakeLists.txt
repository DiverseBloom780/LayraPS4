add_library(layra_core STATIC
    cpu/jaguar_core.cpp
    memory/memory_manager.cpp
    system/emulator.cpp
    # Add more files as you create them
)

target_include_directories(layra_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(layra_core PRIVATE
    # Add dependencies as needed
)
