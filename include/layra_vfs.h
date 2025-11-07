#ifndef LAYRA_VFS_H
#define LAYRA_VFS_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// Maximum path length for VFS entries
#define MAX_VFS_PATH 256

// Structure to represent a VFS mount point
typedef struct {
    char virtual_path[MAX_VFS_PATH]; // e.g., "/app0"
    char host_path[MAX_VFS_PATH];    // e.g., "/tmp/pkg_extracted_data"
} layra_vfs_mount_t;

// Structure to represent a VFS file handle
typedef struct {
    FILE* host_file_ptr;             // Pointer to the actual host file
    char virtual_path[MAX_VFS_PATH]; // Virtual path of the opened file
    long current_offset;             // Current read/write offset
} layra_vfs_file_t;

// Initialize the VFS system
void layra_vfs_init();

// Mount a host path to a virtual path in the VFS
bool layra_vfs_mount(const char* virtual_path, const char* host_path);

// Unmount a virtual path from the VFS
bool layra_vfs_unmount(const char* virtual_path);

// Resolve a virtual path to its corresponding host path
const char* layra_vfs_resolve_path(const char* virtual_path);

// Open a file in the VFS
layra_vfs_file_t* layra_vfs_fopen(const char* virtual_path, const char* mode);

// Close a file in the VFS
int layra_vfs_fclose(layra_vfs_file_t* file);

// Read from a file in the VFS
size_t layra_vfs_fread(void* ptr, size_t size, size_t count, layra_vfs_file_t* file);

// Seek in a file in the VFS
int layra_vfs_fseek(layra_vfs_file_t* file, long offset, int whence);

// Get current position in a file in the VFS
long layra_vfs_ftell(layra_vfs_file_t* file);

#endif // LAYRA_VFS_H

