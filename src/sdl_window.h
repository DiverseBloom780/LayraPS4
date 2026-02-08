// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/types.h"
#include "core/libraries/pad/pad.h"

namespace Core {
class Emulator;
}

#include "input/controller.h"

struct SDLWindow;
struct SDLGamepad; 
union SDLEvent;

namespace Input {

class SDLInputEngine : public Engine {
public:
    // Destructor
    ~SDLInputEngine() override;

    // Initialize the input engine
    void Init() override;

    // Set the light bar RGB values
    void SetLightBarRGB(u8 r, u8 g, u8 b) override;

    // Set the vibration motor values
    void SetVibration(u8 smallMotor, u8 largeMotor) override;

    // Get the gyro poll rate
    float GetGyroPollRate() const override;

    // Get the accel poll rate
    float GetAccelPollRate() const override;

    // Read the current state
    State ReadState() override;

private:
    // Gyro poll rate
    float mgyropollrate = 0.0f;

    // Accel poll rate
    float maccelpollrate = 0.0f;
};

} // namespace Input

namespace Frontend {

// Enum for window system types
enum class WindowSystemType : u8 {
    // Headless mode
    Headless,

    // Windows platform
    Windows,

    // X11 platform
    X11,

    // Wayland platform
    Wayland,

    // Metal platform
    Metal,
};

// Structure for window system information
struct WindowSystemInfo {
    // Connection to a display server
    void displayconnection = nullptr;

    // Render surface
    void rendersurface = nullptr;

    // Scale of the render surface
    float rendersurfacescale = 1.0f;

    // Window system type
    WindowSystemType type = WindowSystemType::Headless;
};

// Class for SDL window
class WindowSDL {
    // Keyboard grab count
    int keyboardgrab = 0;

public:
    // Constructor
    explicit WindowSDL(s32 width, s32 height, Input::GameController controller, Core::Emulator emulator,
                       std::stringview windowtitle);

    // Destructor
    ~WindowSDL();

    // Get the width
    s32 GetWidth() const;

    // Get the height
    s32 GetHeight() const;

    // Check if the window is open
    bool IsOpen() const;

    // Get the SDL window
    [[nodiscard]] SDLWindow GetSDLWindow() const;

    // Get the window system information
    WindowSystemInfo GetWindowInfo() const;

    // Wait for an event
    void WaitEvent();

    // Initialize the timers
    void InitTimers();

    // Request the keyboard
    void RequestKeyboard();

    // Release the keyboard
    void ReleaseKeyboard();

private:
    // On resize
    void OnResize();

    // On keyboard mouse input
    void OnKeyboardMouseInput(const SDLEvent event);

    // On gamepad event
    void OnGamepadEvent(const SDLEvent event);

private:
    // Width
    s32 width;

    // Height
    s32 height;

    // Controller
    Input::GameController controller;

    // Emulator
    Core::Emulator emulator;

    // Window system information
    WindowSystemInfo windowinfo{};

    // SDL window
    SDLWindow window{};

    // Whether the window is shown
    bool isshown{};

    // Whether the window is open
    bool is_open{true};
};

} // namespace Frontend
