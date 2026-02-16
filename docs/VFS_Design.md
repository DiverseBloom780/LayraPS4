# LayraPS4 Emulator - Virtual File System (VFS) Design

## 1. Introduction

This document outlines the design for the Virtual File System (VFS) within the LayraPS4 emulator. The VFS is crucial for abstracting the underlying storage mechanisms, allowing the emulator to access game data from various sources (e.g., PKG files, mounted directories) as if they were a unified filesystem.

## 2. Core Principles

*   **Abstraction**: Provide a consistent interface for file access, regardless of the data source.
*   **Modularity**: Allow different types of file systems (e.g., PKG, physical directory) to be easily integrated.
*   **Performance**: Optimize file access to minimize overhead and ensure smooth game loading and execution.
*   **Simplicity**: Keep the VFS API straightforward for ease of use by other emulator components.

## 3. VFS Architecture

The VFS will operate as a layer between the emulator's core logic and the actual storage. It will manage a list of mounted file systems, each responsible for handling file operations within its designated mount point.

### 3.1. Mount Points

*   The VFS will support multiple mount points, allowing different parts of the emulated file system (e.g., `/app0`, `/data`) to be backed by different physical or virtual sources.
*   Each mount point will be associated with a specific VFS handler responsible for resolving file paths and performing I/O operations.

### 3.2. VFS Handlers

*   A VFS handler is an interface that defines how a specific type of storage provides file access. Examples include:
    *   **Directory Handler**: Maps a VFS path to a physical directory on the host system.
    *   **PKG Handler**: Extracts files from a loaded PKG archive and presents them as a virtual directory structure.

## 4. Key VFS Components

### 4.1. `layra_vfs_init()`

*   Initializes the VFS system, preparing it to accept mount points and file operations.

### 4.2. `layra_vfs_mount(const char* mount_point, const char* source_path)`

*   Registers a new mount point with the VFS.
*   `mount_point`: The virtual path within the VFS (e.g., "/app0").
*   `source_path`: The physical path or identifier for the data source (e.g., a host directory, a PKG file path).
*   Determines the appropriate VFS handler based on `source_path` (e.g., if it's a PKG, use the PKG handler; otherwise, use the directory handler).

### 4.3. `layra_vfs_fopen(const char* path, const char* mode)`

*   Opens a file within the VFS.
*   `path`: The virtual path to the file (e.g., "/app0/eboot.bin").
*   `mode`: Standard file open modes (e.g., "rb", "wb").
*   The VFS will resolve the `path` to the correct mount point and delegate the file open operation to the corresponding VFS handler.

### 4.4. `layra_vfs_fclose(layra_vfs_file_t* file)`

*   Closes a file opened via the VFS.

### 4.5. `layra_vfs_fread(void* buffer, size_t size, size_t count, layra_vfs_file_t* file)`

*   Reads data from a VFS file.

### 4.6. `layra_vfs_fseek(layra_vfs_file_t* file, long offset, int origin)`

*   Seeks to a specific position within a VFS file.

### 4.7. `layra_vfs_ftell(layra_vfs_file_t* file)`

*   Returns the current position of the file pointer.

## 5. PKG Integration with VFS

As designed in `PKG_Design.md`, the `layra_pkg_open_and_mount` function will be responsible for:

1.  Parsing the PKG file header and entries.
2.  Extracting the contents of the PKG to a temporary host directory.
3.  Calling `layra_vfs_mount` to register this temporary directory as a mount point (e.g., `/app0`) within the VFS.

This approach allows the VFS to treat PKG contents as a standard directory, simplifying file access for the rest of the emulator.

## 6. Directory Structure for VFS

*   `LayraPS4/include/layra_vfs.h`: VFS API and data structures.
*   `LayraPS4/src/core/vfs/layra_vfs.c`: VFS implementation, including mount point management and dispatching to handlers.
*   `LayraPS4/src/core/vfs/vfs_dir_handler.c`: (Future) Implementation for mounting physical directories.
*   `LayraPS4/src/core/vfs/vfs_pkg_handler.c`: (Future) Implementation for handling PKG-backed mount points (currently integrated into `layra_pkg.c`).

## 7. Future Considerations

*   **Read-only vs. Read-write**: The current VFS will primarily support read-only access for game data. Write support might be added for save games or configuration files.
*   **Caching**: Implement caching mechanisms to improve performance for frequently accessed files.
*   **Error Handling**: Robust error reporting and handling for file operations.
*   **Unmounting**: A mechanism to unmount file systems when they are no longer needed.
