// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "emulator.h"
#include "sdl_window.h"
#include "common/config.h"
#include "common/log.h"
#include "input/controller.h"

int main(int argc, char* argv[]) {
    // Initialize logging
    Log::init();

    // Load configuration
    Config::load(Config::GetFoolproofInputConfigFile());

    // Create the emulator instance
    Core::Emulator emulator;

    // Create the game controller
    Input::GameController controller;

    // Create the SDL window and ImGui context
    Frontend::WindowSDL window(1280, 720, &controller, "LayraPS4");

    // Main loop
    while (window.IsOpen()) {
        window.WaitEvent();
    }

    // Cleanup
    Log::shutdown();

    return 0;
}
