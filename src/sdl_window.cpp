// LayraPS4 – SDL Window Implementation
#include "sdl_window.h"
#include "common/assert.h"
#include "common/config.h"
#include "common/elfinfo.h"
#include "core/debugstate.h"
#include "core/devtools/layer.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/pad/pad.h"
#include "emulator.h"
#include "input/inputhandler.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_vulkan.h>
#include <iostream>

#ifdef APPLE
#include "SDL3/SDLmetal.h"
#endif

namespace {
static constexpr auto LibWindow = "LibWindow";
static constexpr auto LibInput = "LibInput";
} // namespace

namespace Input {

// Helper for SDL Gamepad Axis mapping
static SDL_GamepadAxis InputAxisToSDL(Axis axis) {
  switch (axis) {
  case Axis::LeftX:
    return SDL_GAMEPAD_AXIS_LEFTX;
  case Axis::LeftY:
    return SDL_GAMEPAD_AXIS_LEFTY;
  case Axis::RightX:
    return SDL_GAMEPAD_AXIS_RIGHTX;
  case Axis::RightY:
    return SDL_GAMEPAD_AXIS_RIGHTY;
  case Axis::TriggerLeft:
    return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
  case Axis::TriggerRight:
    return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
  default:
    return SDL_GAMEPAD_AXIS_INVALID;
  }
}

SDLInputEngine::~SDLInputEngine() {
  if (mgamepad) {
    SDL_CloseGamepad(mgamepad);
  }
}

void SDLInputEngine::Init() {
  if (mgamepad) {
    SDL_CloseGamepad(mgamepad);
    mgamepad = nullptr;
  }

  int gamepadcount;
  SDL_JoystickID *gamepads = SDL_GetGamepads(&gamepadcount);
  if (!gamepads)
    return;

  int selectedIndex = GamepadSelect::GetIndexfromGUID(
      gamepads, gamepadcount, Config::getSelectedGamepad());
  int defaultIndex = GamepadSelect::GetIndexfromGUID(
      gamepads, gamepadcount, Config::getDefaultControllerID());

  if (selectedIndex != -1) {
    mgamepad = SDL_OpenGamepad(gamepads[selectedIndex]);
  } else if (defaultIndex != -1) {
    mgamepad = SDL_OpenGamepad(gamepads[defaultIndex]);
  } else if (gamepadcount > 0) {
    mgamepad = SDL_OpenGamepad(gamepads[0]);
  }

  SDL_free(gamepads);
}

} // namespace Input

namespace Frontend {

WindowSDL::WindowSDL(s32 width, s32 height, Input::GameController controller,
                     Core::Emulator &emulator, std::string_view windowtitle)
    : width(width), height(height), controller(controller), emulator(emulator) {

  std::cout << "[Window] Creating SDL window: " << windowtitle << " (" << width
            << "x" << height << ")\n";
}

WindowSDL::~WindowSDL() {
  if (window) {
    SDL_DestroyWindow(window);
  }
}

s32 WindowSDL::GetWidth() const { return width; }
s32 WindowSDL::GetHeight() const { return height; }
bool WindowSDL::IsOpen() const { return is_open; }
SDL_Window *WindowSDL::GetSDLWindow() const { return window; }

WindowSystemInfo WindowSDL::GetWindowInfo() const { return windowinfo; }

void WindowSDL::WaitEvent() {
  SDL_Event event;
  if (SDL_WaitEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      is_open = false;
    }
    OnKeyboardMouseInput(event);
    OnGamepadEvent(event);
  }
}

void WindowSDL::InitTimers() {}
void WindowSDL::RequestKeyboard() {}
void WindowSDL::ReleaseKeyboard() {}
void WindowSDL::OnResize() {}

void WindowSDL::OnKeyboardMouseInput(const SDL_Event &event) {
  // Basic event handling
}

void WindowSDL::OnGamepadEvent(const SDL_Event &event) {
  // Basic gamepad event handling
}

} // namespace Frontend
