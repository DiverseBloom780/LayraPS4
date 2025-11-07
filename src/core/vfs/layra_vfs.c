#include "layra_vfs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_MOUNTS 16

static layra_vfs_mount_t mounts[MAX_MOUNTS];
static int num_mounts = 0;

void layra_vfs_init() {
    num_mounts = 0;
    // Optionally, initialize default mounts here
}

bool layra_vfs_mount(const char* virtual_path, const char* host_path) {
    if (num_mounts >= MAX_MOUNTS) {
        fprintf(stderr, "VFS: Max mount points reached.\n");
        return false;
    }
    if (strlen(virtual_path) >= MAX_VFS_PATH || strlen(host_path) >= MAX_VFS_PATH) {
        fprintf(stderr, "VFS: Path too long.\n");
        return false;
    }

    strcpy(mounts[num_mounts].virtual_path, virtual_path);
    strcpy(mounts[num_mounts].host_path, host_path);
    num_mounts++;
    fprintf(stdout, "VFS: Mounted %s to %s\n", virtual_path, host_path);
    return true;
}

bool layra_vfs_unmount(const char* virtual_path) {
    for (int i = 0; i < num_mounts; ++i) {
        if (strcmp(mounts[i].virtual_path, virtual_path) == 0) {
            // Shift remaining mounts
            for (int j = i; j < num_mounts - 1; ++j) {
                mounts[j] = mounts[j + 1];
            }
            num_mounts--;
            fprintf(stdout, "VFS: Unmounted %s\n", virtual_path);
            return true;
        }
    }
    fprintf(stderr, "VFS: Mount point %s not found.\n", virtual_path);
    return false;
}

const char* layra_vfs_resolve_path(const char* virtual_path) {
    static char resolved_path[MAX_VFS_PATH];
    for (int i = 0; i < num_mounts; ++i) {
        if (strncmp(virtual_path, mounts[i].virtual_path, strlen(mounts[i].virtual_path)) == 0) {
            // Found a matching mount point
            snprintf(resolved_path, MAX_VFS_PATH, "%s%s", mounts[i].host_path,
                     virtual_path + strlen(mounts[i].virtual_path));
            return resolved_path;
        }
    }
    // If no mount point matches, assume it's a direct host path or invalid
    return NULL;
}

layra_vfs_file_t* layra_vfs_fopen(const char* virtual_path, const char* mode) {
    const char* host_path = layra_vfs_resolve_path(virtual_path);
    if (host_path == NULL) {
        fprintf(stderr, "VFS: Could not resolve virtual path %s\n", virtual_path);
        return NULL;
    }

    FILE* host_file = fopen(host_path, mode);
    if (host_file == NULL) {
        fprintf(stderr, "VFS: Failed to open host file %s for virtual path %s\n", host_path, virtual_path);
        return NULL;
    }

    layra_vfs_file_t* vfs_file = (layra_vfs_file_t*)malloc(sizeof(layra_vfs_file_t));
    if (vfs_file == NULL) {
        fprintf(stderr, "VFS: Memory allocation failed for VFS file handle.\n");
        fclose(host_file);
        return NULL;
    }

    vfs_file->host_file_ptr = host_file;
    strncpy(vfs_file->virtual_path, virtual_path, MAX_VFS_PATH - 1);
    vfs_file->virtual_path[MAX_VFS_PATH - 1] = '\0';
    vfs_file->current_offset = 0;

    return vfs_file;
}

int layra_vfs_fclose(layra_vfs_file_t* file) {
    if (file == NULL) return EOF;
    int result = fclose(file->host_file_ptr);
    free(file);
    return result;
}

size_t layra_vfs_fread(void* ptr, size_t size, size_t count, layra_vfs_file_t* file) {
    if (file == NULL) return 0;
    size_t bytes_read = fread(ptr, size, count, file->host_file_ptr);
    file->current_offset += (long)(bytes_read * size);
    return bytes_read;
}

int layra_vfs_fseek(layra_vfs_file_t* file, long offset, int whence) {
    if (file == NULL) return -1;
    int result = fseek(file->host_file_ptr, offset, whence);
    if (result == 0) {
        file->current_offset = ftell(file->host_file_ptr);
    }
    return result;
}

long layra_vfs_ftell(layra_vfs_file_t* file) {
    if (file == NULL) return -1;
    return file->current_offset;
}

