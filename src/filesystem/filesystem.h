#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <stdint.h>

// Filesystem module definitions
typedef struct {
    uint32_t magic;
    uint32_t version;
} filesystem_header_t;

// Filesystem module functions
void filesystem_init(void);
void filesystem_shutdown(void);

#endif // FILESYSTEM_H
