// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SDL3/SDLevents.h"
#include "SDL3/SDLhints.h"
#include "SDL3/SDLinit.h"
#include "SDL3/SDLproperties.h"
#include "SDL3/SDLtimer.h"
#include "SDL3/SDLvideo.h"
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

#ifdef APPLE
#include "SDL3/SDLmetal.h"
#endif

namespace Input {

using Libraries::Pad::OrbisPadButtonDataOffset;

static OrbisPadButtonDataOffset SDLGamepadToOrbisButton(u8 button) {
    using OPBDO = OrbisPadButtonDataOffset;

 switch (button) {
 case SDLGAMEPADBUTTONDPADDOWN:
 return OPBDO::Down;
 case SDLGAMEPADBUTTONDPADUP: 
 return OPBDO::Up; 
 case SDLGAMEPADBUTTONDPADLEFT:
 return OPBDO::Left; 
 case SDLGAMEPADBUTTONDPADRIGHT:
 return OPBDO::Right; 
 case SDLGAMEPADBUTTONSOUTH:
 return OPBDO::Cross;
 case SDLGAMEPADBUTTONNORTH:
 return OPBDO::Triangle; 
 case SDLGAMEPADBUTTONWEST:
 return OPBDO::Square;
 case SDLGAMEPADBUTTONEAST:
 return OPBDO::Circle; 
 case SDLGAMEPADBUTTONSTART:
 return OPBDO::Options;
 case SDLGAMEPADBUTTONTOUCHPAD:
 return OPBDO::TouchPad; 
 case SDLGAMEPADBUTTONBACK:
 return OPBDO::TouchPad;
 case SDLGAMEPADBUTTONLEFTSHOULDER:
 return OPBDO::L1; 
 case SDLGAMEPADBUTTONRIGHTSHOULDER:
 return OPBDO::R1;
 case SDLGAMEPADBUTTONLEFTSTICK: 
 return OPBDO::L3; 
 case SDLGAMEPADBUTTONRIGHTSTICK:
 return OPBDO::R3;
 default:
 return OPBDO::None;
 } 
}

static SDLGamepadAxis InputAxisToSDL(Axis axis) {
 switch (axis) {
 case Axis::LeftX:
 return SDLGAMEPADAXISLEFTX;
 case Axis::LeftY :
 return SDLGAMEPADAXISLEFTY; 
 case Axis::RightX:
 return SDLGAMEPADAXISRIGHTX;
 case Axis::RightY:
 return SDLGAMEPADAXISRIGHTY; 
 case Axis::TriggerLeft:
 return SDLGAMEPADAXISLEFTTRIGGER; 
 case Axis::TriggerRight:
 return SDLGAMEPADAXISRIGHTTRIGGER; 
 default:
 UNREACHABLE();
    }
}

SDLInputEngine::~SDLInputEngine() {
 if (mgamepad) {
 SDLCloseGamepad(mgamepad);
 } 
}

void SDLInputEngine::Init() {
    if (mgamepad) {
        SDLCloseGamepad(mgamepad);
        mgamepad = nullptr;
    }

 int gamepadcount; 
 SDLJoystickID* gamepads = SDLGetGamepads(&gamepadcount);
 if (!gamepads) {
 LOGERROR(Input, "Cannot get gamepad list: {}" , SDLGetError()); 
 return; 
 }
 if (gamepadcount == 0) {
 LOGINFO(Input, "No gamepad found!"); 
 SDLfree(gamepads); 
 return;
    }

    int selectedIndex = GamepadSelect::GetIndexfromGUID(gamepads, gamepadcount,
                                                        GamepadSelect::GetSelectedGamepad());
    int defaultIndex =
        GamepadSelect::GetIndexfromGUID(gamepads, gamepadcount, Config::getDefaultControllerID());

 // If user selects a gamepad in the GUI, use that, otherwise try the default
 if (!mgamepad) {
 if (selectedIndex != -1) {
 mgamepad = SDLOpenGamepad(gamepads[selectedIndex]); 
 LOGINFO(Input, "Opening gamepad selected in GUI."); 
 } else if (defaultIndex != -1) {
 mgamepad = SDLOpenGamepad(gamepads[defaultIndex]);
 LOGINFO(Input, "Opening default gamepad."); 
 } else {
 mgamepad = SDLOpenGamepad(gamepads[0]);
 LOGINFO(Input, "Got {} game
