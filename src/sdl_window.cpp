// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "common/elfinfo.h"
#include "core/debugstate.h"
#include "core/devtools/layer.h"
#include "core/libraries/kernel/ time.h"
#include "core/libraries/pad/pad.h"
#include "imgui/renderer/imguicore.h"
#include "input/controller.h"
#include "input/inputhandler.h"
#include "input/inputmouse.h"
#include "sdlwindow.h"
#include "videocore/ renderdoc.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_vulkan.h>

#ifdef APPLE
#include "SDL3/SDLmetal.h"
#endif

namespace Input {

using Libraries::Pad::OrbisPadButtonDataOffset;

static OrbisPadButtonDataOffset SDLGamepadToOrbisButton(u8 button) {
  using OPBDO = OrbisPadButtonDataOffset;

  switch (button) {
  case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
    return OPBDO::Down;
  case SDL_GAMEPAD_BUTTON_DPAD_UP:
    return OPBDO::Up;
  case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
    return OPBDO::Left;
  case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
    return OPBDO::Right;
  case SDL_GAMEPAD_BUTTON_SOUTH:
    return OPBDO::Cross;
  case SDL_GAMEPAD_BUTTON_NORTH:
    return OPBDO::Triangle;
  case SDL_GAMEPAD_BUTTON_WEST:
    return OPBDO::Square;
  case SDL_GAMEPAD_BUTTON_EAST:
    return OPBDO::Circle;
  case SDL_GAMEPAD_BUTTON_START:
    return OPBDO::Options;
  case SDL_GAMEPAD_BUTTON_TOUCHPAD:
    return OPBDO::TouchPad;
  case SDL_GAMEPAD_BUTTON_BACK:
    return OPBDO::TouchPad;
  case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
    return OPBDO::L1;
  case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
    return OPBDO::R1;
  case SDL_GAMEPAD_BUTTON_LEFT_STICK:
    return OPBDO::L3;
  case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
    return OPBDO::R3;
  default:
    return OPBDO::None;
  }
}

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
    UNREACHABLE();
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
  if (!gamepads) {
    LOGERROR(Input, "Cannot get gamepad list: {}", SDL_GetError());
    return;
  }
  if (gamepadcount == 0) {
    LOGINFO(Input, "No gamepad found!");
    SDL_free(gamepads);
    return;
  }

  int selectedIndex = GamepadSelect::GetIndexfromGUID(
      gamepads, gamepadcount, GamepadSelect::GetSelectedGamepad());
  int defaultIndex = GamepadSelect::GetIndexfromGUID(
      gamepads, gamepadcount, Config::getDefaultControllerID());

  // If user selects a gamepad in the GUI, use that, otherwise try the default
  if (!mgamepad) {
    if (selectedIndex != -1) {
      mgamepad = SDL_OpenGamepad(gamepads[selectedIndex]);
      LOGINFO(Input, "Opening gamepad selected in GUI.");
    } else if (defaultIndex != -1) {
      mgamepad = SDL_OpenGamepad(gamepads[defaultIndex]);
      LOGINFO(Input, "Opening default gamepad.");
    } else {
      mgamepad = SDL_OpenGamepad(gamepads[0]);
 LOGINFO(Input, "Got {} game
