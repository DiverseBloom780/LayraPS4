// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "emulator.h"
#include "sdl_window.h"
#include "common/config.h"
#include "common/log4cpp.h"
#include "input/controller.h"


int main(int argc, char* argv[]) {
    // Initialize logging
    Log4Cpp::Logger logger = Log4Cpp::Logger::getLogger("LayraPS4");

    // Configure logging
    Log4Cpp::Configurator::configure("log4cpp.properties");

    try {
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
        logger.info("Emulator shutting down...");

        // Shutdown logging
        Log4Cpp::Configurator::shutdown();
    } catch (const std::exception& e) {
        // Handle unexpected errors and exceptions
        logger.error("Unexpected error: %s", e.what());

        // Return error code
        return 1;
    }

    // Return success code
    return 0;
}
