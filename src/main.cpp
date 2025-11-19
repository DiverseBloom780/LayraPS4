// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "emulator.h"
#include "sdl_window.h"
#include "common/config.h"
#include "common/log.h"

int main(int argc, char* argv[]) {
    // Initialize logging
    Log::init();

    // Load configuration
    Config::load(Config::GetFoolproofInputConfigFile());

    // Create the emulator instance
    Emulator emulator;

    // Create the SDL window and ImGui context
    WindowSDL window(&emulator);

    // Main loop
    while (window.isRunning()) {
        window.handleEvents();
        window.render();
    }

    // Cleanup
    window.cleanup();
    Log::shutdown();

    return 0;
}
