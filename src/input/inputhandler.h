#pragma once

#include "common/types.h"
#include <string_view>

namespace Input {

enum class Axis { LeftX, LeftY, RightX, RightY, TriggerLeft, TriggerRight };

struct State {
  // Dummy state
};

class Engine {
public:
  virtual ~Engine() = default;
  virtual void Init() = 0;
  virtual void SetLightBarRGB(u8 r, u8 g, u8 b) = 0;
  virtual void SetVibration(u8 smallMotor, u8 largeMotor) = 0;
  virtual float GetGyroPollRate() const = 0;
  virtual float GetAccelPollRate() const = 0;
  virtual State ReadState() = 0;
};

class GameController {
public:
  GameController() = default;
};

struct GamepadSelect {
  static int GetIndexfromGUID(void *, int, const char *) { return -1; }
  static const char *GetSelectedGamepad() { return ""; }
};

} // namespace Input
