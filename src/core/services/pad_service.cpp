#include "pad_service.h"
#include <iostream>

namespace Core {
namespace Services {

PadService::PadService() { std::cout << "[Input] scePad service created.\n"; }

PadService::~PadService() {
  for (auto *gamepad : gamepads) {
    SDL_CloseGamepad(gamepad);
  }
}

void PadService::Initialize() {
  std::cout << "[Input] Initializing SDL3 Gamepad backend...\n";
  // SDL3 detects gameports automatically, but we can list initially
  int numJoysticks = 0;
  SDL_JoystickID *joysticks = SDL_GetGamepads(&numJoysticks);
  if (joysticks) {
    for (int i = 0; i < numJoysticks; ++i) {
      if (SDL_IsGamepad(joysticks[i])) {
        SDL_Gamepad *gamepad = SDL_OpenGamepad(joysticks[i]);
        if (gamepad) {
          std::cout << "[Input] Opened Gamepad: " << SDL_GetGamepadName(gamepad)
                    << "\n";
          gamepads.push_back(gamepad);
        }
      }
    }
    SDL_free(joysticks);
  }
}

int PadService::PadInit() { return 0; }

int PadService::PadOpen(int userId, int type, int index, void *param) {
  std::cout << "[Input] scePadOpen(userId=" << userId << ", type=" << type
            << ")\n";
  return 123; // Dummy handle
}

int PadService::PadRead(int handle, OrbisPadData *data, int numPads) {
  if (numPads <= 0 || !data)
    return -1;

  std::lock_guard<std::mutex> lock(padMutex);

  // Default empty state
  data->buttons = 0;
  data->leftStickX = 128;
  data->leftStickY = 128;
  data->rightStickX = 128;
  data->rightStickY = 128;

  if (!gamepads.empty()) {
    UpdatePadData(data, gamepads[0]);
  }

  return 0;
}

void PadService::UpdatePadData(OrbisPadData *data, SDL_Gamepad *controller) {
  if (!controller)
    return;

  // Map SDL buttons to Orbis buttons
  // This is a simplified mapping
  if (SDL_GetGamepadButton(controller, SDL_GAMEPAD_BUTTON_SOUTH))
    data->buttons |= (1 << 0); // Cross
  if (SDL_GetGamepadButton(controller, SDL_GAMEPAD_BUTTON_EAST))
    data->buttons |= (1 << 1); // Circle
  if (SDL_GetGamepadButton(controller, SDL_GAMEPAD_BUTTON_WEST))
    data->buttons |= (1 << 2); // Square
  if (SDL_GetGamepadButton(controller, SDL_GAMEPAD_BUTTON_NORTH))
    data->buttons |= (1 << 3); // Triangle

  // D-Pad
  if (SDL_GetGamepadButton(controller, SDL_GAMEPAD_BUTTON_DPAD_UP))
    data->buttons |= (1 << 4);
  if (SDL_GetGamepadButton(controller, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
    data->buttons |= (1 << 5);
  if (SDL_GetGamepadButton(controller, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
    data->buttons |= (1 << 6);
  if (SDL_GetGamepadButton(controller, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
    data->buttons |= (1 << 7);

  // Sticks (SDL returns -32768 to 32767, Orbis expects 0-255)
  auto mapAxis = [](int16_t val) -> uint8_t {
    return (uint8_t)((val + 32768) / 256);
  };

  data->leftStickX =
      mapAxis(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFTX));
  data->leftStickY =
      mapAxis(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFTY));
  data->rightStickX =
      mapAxis(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_RIGHTX));
  data->rightStickY =
      mapAxis(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_RIGHTY));
}

} // namespace Services
} // namespace Core
