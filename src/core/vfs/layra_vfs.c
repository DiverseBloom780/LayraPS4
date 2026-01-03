// FILE 2: LayraPS4/src/core/vfs/layra_vfs.c           [BACKEND ENGINE]
#include "layra_vfs.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    char mount_point[256];
    char source_path[512];
    void* handler;
    int is_read_only;
} layra_vfs_mount_t;

static layra_vfs_mount_t g_mount_points[10];
static int g_mount_count = 0;
static int g_vfs_initialized = 0;

int layra_vfs_init(void) {
    if (g_vfs_initialized) return 0;
    
    // Initialize SDL3 subsystems needed for VFS
    if (SDL_Init(SDL_INIT_IO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL3 initialization failed: %s", SDL_GetError());
        return -1;
    }
    
    g_mount_count = 0;
    g_vfs_initialized = 1;
    return 0;
}

int layra_vfs_mount(const char* mount_point, const char* source_path) {
    if (!mount_point || !source_path) return -1;
    if (g_mount_count >= 10) return -1;
    
    // Use SDL3 string copying for safety
    SDL_strlcpy(g_mount_points[g_mount_count].mount_point, mount_point, sizeof(g_mount_points[g_mount_count].mount_point));
    SDL_strlcpy(g_mount_points[g_mount_count].source_path, source_path, sizeof(g_mount_points[g_mount_count].source_path));
    g_mount_points[g_mount_count].is_read_only = 1;
    g_mount_count++;
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "VFS: Mounted %s at %s", source_path, mount_point);
    return 0;
}

layra_vfs_file_t* layra_vfs_fopen(const char* path, const char* mode) {
    if (!path || !mode) return NULL;
    
    // Use SDL3 file operations for better cross-platform support
    SDL_IOStream* stream = SDL_IOFromFile(path, mode);
    if (!stream) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "VFS: Failed to open file %s: %s", path, SDL_GetError());
        return NULL;
    }
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "VFS: Opened file %s in mode %s", path, mode);
    return (layra_vfs_file_t*)stream;
}

int layra_vfs_fclose(layra_vfs_file_t* file) {
    if (!file) return -1;
    
    SDL_IOStream* stream = (SDL_IOStream*)file;
    if (SDL_CloseIO(stream) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "VFS: Failed to close file: %s", SDL_GetError());
        return -1;
    }
    
    return 0;
}

size_t layra_vfs_fread(void* buffer, size_t size, size_t count, layra_vfs_file_t* file) {
    if (!buffer || !file) return 0;
    
    SDL_IOStream* stream = (SDL_IOStream*)file;
    size_t total_bytes = size * count;
    size_t bytes_read = SDL_ReadIO(stream, buffer, total_bytes);
    
    if (bytes_read != total_bytes && SDL_GetError()[0] != '\0') {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "VFS: Read error: %s", SDL_GetError());
    }
    
    return bytes_read / size;  // Return number of items read
}

int layra_vfs_fseek(layra_vfs_file_t* file, long offset, int origin) {
    if (!file) return -1;
    
    SDL_IOStream* stream = (SDL_IOStream*)file;
    Sint64 result = SDL_SeekIO(stream, offset, origin);
    
    if (result < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "VFS: Seek error: %s", SDL_GetError());
        return -1;
    }
    
    return 0;
}

long layra_vfs_ftell(layra_vfs_file_t* file) {
    if (!file) return -1;
    
    SDL_IOStream* stream = (SDL_IOStream*)file;
    Sint64 position = SDL_TellIO(stream);
    
    if (position < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "VFS: Tell error: %s", SDL_GetError());
        return -1;
    }
    
    return (long)position;
}

