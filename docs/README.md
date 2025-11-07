# LayraPS4 Emulator - Windows Build and Run Instructions

This document provides instructions on how to build and run the LayraPS4 emulator on a Windows system. The emulator is currently under development, with a focus on implementing a Vulkan graphics backend, basic PKG (PlayStation Package) and VFS (Virtual File System) support, and an XMB (Cross-Media Bar) inspired user interface using ImGui.

## Current Status

*   **Graphics:** Initial Vulkan rendering backend is implemented. The emulator window should display the ImGui-based XMB interface.
*   **PKG/VFS:** Basic structures for PKG parsing and a Virtual File System are in place. Further development is required for full functionality.
*   **UI:** An XMB-inspired UI is rendered using ImGui.
*   **Dependencies:** SDL2 is used for window management and input. Vulkan is used for rendering.
*   **Limitations:** SDL2_image functionality has been temporarily disabled in the provided codebase to resolve build issues in the sandbox environment. If image loading (e.g., for boot logos, game covers) is required, `SDL2_image` will need to be re-enabled and properly linked.

## Prerequisites

Before you begin, ensure you have the following software installed on your Windows system:

1.  **Git:** For cloning the repository.
    *   [Download Git](https://git-scm.com/download/win)
2.  **CMake (version 3.24 or higher):** For generating build files.
    *   [Download CMake](https://cmake.org/download/)
3.  **Visual Studio (2019 or newer) or MinGW-w64:** A C/C++ compiler and build environment.
    *   **Visual Studio:** Install with 

the "Desktop development with C++" workload.
    *   [Download Visual Studio](https://visualstudio.microsoft.com/downloads/)
    *   **MinGW-w64:** For GCC/G++ on Windows.
    *   [Download MinGW-w64](https://mingw-w64.org/doku.php/download)
4.  **Vulkan SDK:** Essential for Vulkan development.
    *   [Download Vulkan SDK](https://sdk.lunarg.com/sdk/download/)
5.  **SDL2 Development Libraries:**
    *   [Download SDL2 Development Libraries (Visual C++ 32/64-bit)](https://github.com/libsdl-org/SDL/releases)
    *   Extract the contents to a known location, e.g., `C:\Libs\SDL2-devel`.

## Building the Emulator

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/your-repo/LayraPS4.git # Replace with actual repo URL
    cd LayraPS4
    ```

2.  **Set up environment variables (if not already configured by Vulkan SDK installer):**
    Ensure `VULKAN_SDK` environment variable points to your Vulkan SDK installation directory (e.g., `C:\VulkanSDK\x.y.z.0`).

3.  **Create a build directory and run CMake:**
    ```bash
    mkdir build
    cd build
    ```

    **For Visual Studio:**
    ```bash
    cmake .. -G "Visual Studio 17 2022" -A x64 # Adjust generator and architecture as needed
    ```
    *   *Note:* Replace `"Visual Studio 17 2022"` with your installed Visual Studio version (e.g., `"Visual Studio 16 2019"`).

    **For MinGW-w64 (MSYS2/MinGW-w64 Shell):**
    ```bash
    cmake .. -G "MinGW Makefiles"
    ```

4.  **Build the project:**

    **For Visual Studio:**
    Open the generated `LayraPS4.sln` file in Visual Studio and build the `ALL_BUILD` project, or run from the command line:
    ```bash
    cmake --build . --config Release # Or Debug
    ```

    **For MinGW-w64:**
    ```bash
    make
    ```

## Running the Emulator

After a successful build, the executable `LayraPS4.exe` will be located in the `build/Release` (or `build/Debug`) directory for Visual Studio, or directly in the `build` directory for MinGW-w64.

To run the emulator, simply execute it:

```bash
./LayraPS4.exe
```

### Expected Behavior

Upon running, you should see an SDL2 window with the ImGui-based XMB interface. Initially, a placeholder boot screen text will be displayed for a few seconds before transitioning to the XMB menu.

## Next Steps for Development

*   **Vulkan Graphics:** Implement the full rendering pipeline for game content.
*   **PKG/VFS:** Enhance PKG parsing to extract filenames and implement a more robust VFS for accessing game assets.
*   **XMB Functionality:** Implement the actual logic behind the XMB menu items (e.g., launching games, accessing settings).
*   **Input Handling:** Further refine input handling for game controls.
*   **Audio:** Integrate an audio backend.

## Contributing

Contributions are welcome! Please follow the project's coding style and submit pull requests.

## License

[Specify your license here, e.g., MIT, GPLv3]

