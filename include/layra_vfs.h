// Path: /LayraPS4/include/layra_vfs.h

#ifndef LAYRA_VFS_H
#define LAYRA_VFS_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct layra_vfs_file_t layra_vfs_file_t;

int layra_vfs_init(void);
int layra_vfs_mount(const char* mount_point, const char* source_path);
layra_vfs_file_t* layra_vfs_fopen(const char* path, const char* mode);
int layra_vfs_fclose(layra_vfs_file_t* file);
size_t layra_vfs_fread(void* buffer, size_t size, size_t count, layra_vfs_file_t* file);
int layra_vfs_fseek(layra_vfs_file_t* file, long offset, int origin);
long layra_vfs_ftell(layra_vfs_file_t* file);

#ifdef __cplusplus
}
#endif

#endif // LAYRA_VFS_H
