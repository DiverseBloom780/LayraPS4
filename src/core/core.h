#ifndef CORE_H
#define CORE_H
#include <stdint.h>

// Core module definitions
typedef struct {
    uint32_t magic;
    uint32_t version;
} core_header_t;

// Core module functions
void core_init(void);
void core_shutdown(void);

#endif // CORE_H
