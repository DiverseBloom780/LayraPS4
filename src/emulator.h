// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class Emulator {
public:
    Emulator();
    ~Emulator();

    void run();
    void stop();
    bool isRunning() const { return running; }

private:
    bool running = true;
};
