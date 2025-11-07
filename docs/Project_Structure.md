# LayraPS4 Emulator - Project Structure Documentation

## 1. Introduction

This document details the overall project structure of the LayraPS4 emulator, providing an overview of its directories, key files, and their respective roles. This structure is designed to promote modularity, maintainability, and scalability as the emulator develops.

## 2. Root Directory (`LayraPS4/`)

This is the top-level directory of the LayraPS4 project, containing all source code, build artifacts, documentation, and external dependencies.

| Directory/File | Description |
| :------------- | :---------- |
| `CMakeLists.txt` | The primary CMake build script for configuring and building the entire project. |
| `src/` | Contains all the core source code for the emulator. |
| `include/` | Stores public header files that define interfaces for various modules. |
| `lib/` | Houses external libraries and their dependencies, such as ImGui. |
| `build/` | The output directory for CMake-generated build files and compiled executables. |
| `docs/` | Contains all design documents, technical specifications, and user manuals. |
| `resources/` | Stores assets like images, icons, and other media used by the emulator (e.g., boot screen logo). |

## 3. Source Code Directory (`LayraPS4/src/`)

This directory contains the main application logic and various core components of the emulator.

| Directory/File | Description |
| :------------- | :---------- |
| `main.cpp` | The entry point of the LayraPS4 emulator application. Initializes SDL, OpenGL, ImGui, and manages the main application loop, including boot screen and XMB rendering. |
| `core/` | Contains fundamental emulator components, such as PKG parsing and the Virtual File System. |
| `common/` | (Planned) General utility functions and common data structures used across multiple modules. |
| `crypto/` | (Planned) Cryptographic functions and utilities, potentially adapted from v0.7.0. |

### 3.1. Core Components (`LayraPS4/src/core/`)

This subdirectory groups essential, low-level emulator functionalities.

| Directory/File | Description |
| :------------- | :---------- |
| `pkg/` | Contains source code related to PlayStation Package (PKG) file parsing and handling. |
| `vfs/` | Contains source code for the Virtual File System (VFS) implementation. |

#### 3.1.1. PKG Module (`LayraPS4/src/core/pkg/`)

| File | Description |
| :--- | :---------- |
| `layra_pkg.c` | Implementation of functions for parsing PKG headers, entries, and extracting/mounting PKG contents. |

#### 3.1.2. VFS Module (`LayraPS4/src/core/vfs/`)

| File | Description |
| :--- | :---------- |
| `layra_vfs.c` | Implementation of the Virtual File System, managing mount points and dispatching file operations. |

## 4. Include Directory (`LayraPS4/include/`)

This directory holds public header files that define the interfaces for various modules, allowing other parts of the emulator to interact with them.

| File | Description |
| :--- | :---------- |
| `layra_pkg.h` | Defines data structures and function prototypes for the PKG parsing module. |
| `layra_vfs.h` | Defines data structures and function prototypes for the Virtual File System. |

## 5. Libraries Directory (`LayraPS4/lib/`)

This directory manages third-party libraries used by LayraPS4.

| Directory | Description |
| :-------- | :---------- |
| `imgui/` | Contains the source code for the Dear ImGui library, including its core files and backend implementations for SDL2 and OpenGL. |

## 6. Documentation Directory (`LayraPS4/docs/`)

This directory contains all project documentation, including design specifications, API references, and future user guides.

| File | Description |
| :--- | :---------- |
| `PKG_Design.md` | Details the design and implementation approach for PKG file parsing and mounting. |
| `UI_UX_Design.md` | Outlines the design for the ImGui-based user interface, including boot animation, XMB, themes, and profile system. |
| `VFS_Design.md` | Describes the architecture and implementation of the Virtual File System. |
| `Project_Structure.md` | This document, detailing the overall directory and file organization. |

## 7. Resources Directory (`LayraPS4/resources/`)

This directory holds static assets used by the emulator.

| File | Description |
| :--- | :---------- |
| `layra_logo.png` | The primary logo image used for the emulator, including the boot screen. |

