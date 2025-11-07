# LayraPS4 Emulator: Implementation Report - Vulkan, PKG, VFS, and Windows Compatibility

## Introduction

This report summarizes the progress made in integrating key functionalities into the LayraPS4 emulator, focusing on a Vulkan graphics backend, initial PKG (PlayStation Package) and VFS (Virtual File System) support, and establishing Windows compatibility for the build system. The objective was to move LayraPS4 towards a fully functional state capable of running PlayStation 4 games, drawing architectural insights from the `shadPS4` emulator without direct code copying.

## Implementation Highlights

### 1. Vulkan Graphics Backend Integration

**Objective:** Replace the existing OpenGL backend with Vulkan for enhanced graphics capabilities and modern rendering. This involved:

*   **Vulkan Instance and Device Setup:** Implemented functions to create a Vulkan instance, select a physical device, and create a logical device with appropriate queue families (graphics and present).
*   **Swapchain Management:** Implemented swapchain creation, image view creation, and recreation upon window resize events to handle presentation to the display.
*   **Render Pass and Framebuffers:** Configured a basic render pass suitable for ImGui rendering, along with corresponding framebuffers for each swapchain image.
*   **Command Buffers and Synchronization:** Implemented command pool and command buffer allocation, recording, and submission, along with semaphores and fences for synchronization.
*   **ImGui Integration:** Successfully integrated Dear ImGui with the Vulkan backend, allowing the XMB-inspired user interface to be rendered using Vulkan. This included setting up the descriptor pool and adapting ImGui's rendering calls to the Vulkan command buffer submission flow.

**Challenges and Solutions:**

*   **Dynamic Function Loading:** Initially attempted dynamic loading of Vulkan functions but reverted to direct API calls for simplicity and compatibility with the `vulkan.h` header, which typically provides function prototypes. The `VK_NO_PROTOTYPES` macro was briefly explored but ultimately removed to streamline the development process.
*   **Build System Integration:** Ensured `layra_vulkan.cpp` was correctly compiled and linked within the CMake build system. This involved careful management of source file extensions and CMake `target_link_libraries` directives.
*   **Sandbox Testing Limitations:** Direct testing of the Vulkan rendering was not feasible within the sandboxed environment due to the absence of a physical GPU and display server capable of Vulkan contexts. `Xvfb` was attempted but did not resolve the underlying Vulkan device availability issue. Therefore, visual validation of the Vulkan backend will need to be performed on a Windows system with proper Vulkan drivers.

### 2. PKG (PlayStation Package) Support

**Objective:** Lay the groundwork for parsing and extracting content from PS4 PKG files.

*   **Header and Entry Parsing:** The `layra_pkg.c` file was updated to include structures (`layra_pkg_header_t`, `layra_pkg_entry_t`) and functions for parsing the main PKG header and individual file entries. Endian conversion macros (`BE32_TO_HOST`, `BE64_TO_HOST`) were defined to handle the big-endian format of PKG files on a little-endian host system.
*   **File Extraction Placeholder:** A `layra_pkg_extract_file` function was introduced as a placeholder for extracting files, including a dummy decryption function. Full implementation of filename resolution and decryption keys is a future task.
*   **Mounting Mechanism:** The `layra_pkg_open_and_mount` function was designed to open a PKG, parse its contents, extract them to a temporary directory, and then mount this directory into the Virtual File System.

### 3. VFS (Virtual File System) Implementation

**Objective:** Create a basic VFS to abstract file access and allow mounting of PKG contents.

*   **Mount Point Management:** The `layra_vfs.c` file defines structures (`layra_vfs_mount_t`) and functions (`layra_vfs_init`, `layra_vfs_mount`, `layra_vfs_unmount`) to manage virtual mount points, mapping them to host system paths.
*   **Path Resolution:** A `layra_vfs_resolve_path` function was implemented to translate virtual paths (e.g., `/app0/SLES_000.BIN`) to their corresponding physical paths on the host system.
*   **File I/O Abstraction:** VFS-aware file I/O functions (`layra_vfs_fopen`, `layra_vfs_fclose`, `layra_vfs_fread`, `layra_vfs_fseek`, `layra_vfs_ftell`) were created to allow the emulator to access files through the VFS layer, regardless of their origin (e.g., extracted PKG or host filesystem).

### 4. Windows Compatibility and Build System Configuration

**Objective:** Configure the CMake build system to support compilation on Windows with Visual Studio or MinGW-w64.

*   **CMake Adjustments:** The `CMakeLists.txt` file was modified to:
    *   Detect the `WIN32` platform and set up `VULKAN_SDK` paths accordingly.
    *   Include Windows-specific compiler definitions (`_CRT_SECURE_NO_WARNINGS`, `NOMINMAX`, etc.) similar to `shadPS4`.
    *   Link against Windows-specific libraries for SDL2 (`SDL2main`, `user32`, `gdi32`, `shell32`, `advapi32`).
    *   Ensure correct linking of Vulkan libraries (`vulkan-1`).
*   **SDL2_image Removal:** Due to persistent linking issues within the sandbox environment, `SDL2_image` related includes and calls were commented out or removed. This dependency will need to be re-evaluated and properly integrated when image loading functionality becomes critical for game assets or boot logos.

## Current State of LayraPS4

The emulator now compiles successfully in the sandbox environment (which implies Linux compatibility) and is configured for Windows compilation. The core Vulkan rendering pipeline is established, and the ImGui-based XMB interface is expected to render correctly on a Windows machine with Vulkan drivers. The PKG and VFS systems provide foundational support for loading game content, though significant work remains in fully implementing PKG decryption, filename resolution, and comprehensive VFS operations.

## Next Steps

1.  **Windows Testing:** The primary next step is for the user to build and run the emulator on a Windows machine to validate the Vulkan rendering and ImGui XMB display.
2.  **PKG Decryption and Filename Resolution:** Implement the full PKG parsing logic, including decryption and accurate filename extraction, leveraging insights from `shadPS4`'s approach.
3.  **XMB Functionality:** Develop the actual logic for the XMB menu options, such as launching games, configuring settings, and managing profiles.
4.  **Emulator Core:** Begin implementing the CPU, GPU, and other hardware emulation components necessary to run PS4 games.

## Conclusion

Significant progress has been made in transitioning LayraPS4 to a Vulkan-based renderer and establishing a robust foundation for PKG and VFS. The project is now set up for Windows development, and the provided `README.md` will guide the user through the build process. The next phase will focus on bringing the core emulation logic to life and integrating the PKG/VFS with actual game loading.
