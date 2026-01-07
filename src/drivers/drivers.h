#ifndef DRIVERS_H
#define DRIVERS_H
#include <stdint.h>

// Driver module definitions
typedef struct {
    uint32_t magic;
    uint32_t version;
} driver_header_t;

// Driver module functions
void driver_init(void);
void driver_shutdown(void);

#endif // DRIVERS_H
