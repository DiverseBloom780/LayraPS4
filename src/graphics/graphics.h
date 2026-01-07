#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <stdint.h>

// Graphics module definitions
typedef struct {
    uint32_t magic;
    uint32_t version;
} graphics_header_t;

// Graphics module functions
void graphics_init(void);
void graphics_shutdown(void);

#endif // GRAPHICS_H
