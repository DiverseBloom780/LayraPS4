#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H
#ifdef __cplusplus
extern "C" {
#endif

// Initialize boot screen system
void boot_screen_init(void);

// Render the boot screen (call each frame)
void boot_screen_render(void);

// Check if boot sequence is complete
bool boot_screen_is_complete(void);

// Reset boot screen for next boot
void boot_screen_reset(void);

#ifdef __cplusplus
}
#endif

#endif // BOOT_SCREEN_H
