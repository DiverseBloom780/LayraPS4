#ifndef INPUT_H
#define INPUT_H
#include <stdint.h>

// Input module definitions
typedef struct {
    uint32_t magic;
    uint32_t version;
} input_header_t;

// Input module functions
void input_init(void);
void input_shutdown(void);

#endif // INPUT_H
