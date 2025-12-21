// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class Emulator {
public:
    // Constructor
    Emulator();

    // Destructor
    ~Emulator();

    // Run the emulator
    void run();

    // Stop the emulator
    void stop();

    // Check if the emulator is running
    bool isRunning() const {
        return running;
    }

private:
    // Flag to indicate if the emulator is running
    bool running = true;
};
