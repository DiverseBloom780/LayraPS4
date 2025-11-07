# LayraPS4 Emulator - Input Device Design

## 1. Introduction

This document outlines the design for integrating specialized input devices, specifically lightguns, PlayStation Move controllers, and the PlayStation Aim controller, into the LayraPS4 emulator. The goal is to provide a robust emulation layer that allows users to experience games designed for these peripherals.

## 2. Core Principles

*   **Accuracy**: Emulation should accurately mimic the behavior and input of the original hardware.
*   **Flexibility**: The system should allow for both physical device passthrough (if feasible) and software emulation using standard input devices (mouse, keyboard, gamepad).
*   **Modularity**: Input device support should be modular, allowing for easy addition of new devices and updates to existing ones.

## 3. PlayStation Move Controllers

### 3.1. Overview

PlayStation Move controllers consist of a motion controller (wand) and a navigation controller, primarily tracked by the PlayStation Camera. They provide motion tracking, button input, and haptic feedback.

### 3.2. Emulation Approach

*   **Motion Tracking**: Emulating the motion tracking will be challenging without a physical camera. For initial implementation, a mouse or gamepad joystick could be mapped to simulate movement and pointing. More advanced emulation might involve using PC-compatible VR headsets or webcams with custom tracking software.
*   **Button Input**: Standard gamepad buttons or keyboard keys will be mapped to the Move controller's buttons (Trigger, Move, Face Buttons, Start/Select).
*   **Haptic Feedback**: Basic haptic feedback could be emulated using gamepad rumble features.
*   **Device Detection**: The emulator will need a mechanism to detect if a virtual Move controller is active and present it to the emulated PS4 system.

### 3.3. Integration with LayraPS4

*   **Input System Extension**: Extend `input_device.cpp` (or equivalent in LayraPS4) to include new device classes for PlayStation Move.
*   **HID Emulation**: Implement logic to translate PC input (mouse/gamepad) into data packets that mimic the PlayStation Move's communication protocol with the PS4.

## 4. Lightgun Support

### 4.1. Overview

Lightguns, such as the Sinden Lightgun, provide precise aiming and shooting input for arcade-style games. Modern lightguns often use camera-based tracking for screen position.

### 4.2. Emulation Approach

*   **Aiming**: The primary input for a lightgun is aiming. This can be directly mapped to the mouse cursor on the PC. The mouse click will simulate the trigger pull.
*   **Off-screen Reload/Actions**: Some lightguns have off-screen reload mechanisms or additional buttons. These can be mapped to keyboard keys or extra mouse buttons.
*   **Calibration**: A calibration routine will be necessary to map the mouse cursor's movement accurately to the emulated screen space.

### 4.3. Integration with LayraPS4

*   **Input System Extension**: Add a new device class for lightguns within the input system.
*   **Mouse Input Handling**: Capture raw mouse input to provide precise, low-latency aiming. This will bypass any OS-level mouse acceleration.
*   **HID Emulation**: Translate mouse events into appropriate lightgun data packets for the emulated PS4 system.

## 5. PlayStation Aim Controller

### 5.1. Overview

The PlayStation Aim controller is a dedicated two-handed gun peripheral designed for PSVR shooter games, offering accurate 1:1 tracking and traditional gamepad controls.

### 5.2. Emulation Approach

*   **Tracking**: Similar to PlayStation Move, the Aim controller relies on camera tracking. Emulation will initially involve mapping PC gamepad or mouse/keyboard combinations to its positional and rotational data. Advanced emulation might consider integrating with PC VR tracking systems.
*   **Button Input**: All buttons on the Aim controller (analog sticks, triggers, face buttons) will be mapped to a standard PC gamepad or keyboard.

### 5.3. Integration with LayraPS4

*   **Input System Extension**: Extend the input system with a dedicated class for the PlayStation Aim controller.
*   **Combined Input**: The emulation will need to combine positional data (from mouse/VR tracking) with button inputs (from gamepad/keyboard) to accurately represent the Aim controller's state.

## 6. General Input System Considerations

*   **Device Manager**: A central input device manager will be required to handle enumeration, configuration, and mapping of physical and virtual input devices.
*   **Configuration UI**: An ImGui-based configuration interface will allow users to customize key bindings and input mappings for each device type.
*   **Hot-swapping**: The system should ideally support hot-swapping of input devices during runtime.

## 7. References

*   Reddit. (2024). *Playstation Move as light gun for Mame and other emulators*. Retrieved from [https://www.reddit.com/r/MAME/comments/17d9yf6/playstation_move_as_light_gun_for_mame_and_other/](https://www.reddit.com/r/MAME/comments/17d9yf6/playstation_move_as_light_gun_for_mame_and_other/)
*   Sinden Lightgun. (n.d.). *Sinden Lightgun® – The official site for the Sinden Lightgun®*. Retrieved from [https://sindenlightgun.com/](https://sindenlightgun.com/)
*   PlayStation. (n.d.). *PlayStation Move motion controller support (US)*. Retrieved from [https://www.playstation.com/en-us/support/hardware/ps-move-help/](https://www.playstation.com/en-us/support/hardware/ps-move-help/)
*   PlayStation. (n.d.). *PlayStation VR Aim Controller | Control PS VR shooter games*. Retrieved from [https://www.playstation.com/en-us/accessories/playstation-vr-aim-controller/](https://www.playstation.com/en-us/accessories/playstation-vr-aim-controller/)

