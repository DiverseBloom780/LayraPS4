// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "service_manager.h"
#include <SDL3/SDL.h>
#include <mutex>
#include <vector>


namespace Core {
namespace Services {

struct OrbisPadData {
  uint32_t buttons;
  uint8_t leftStickX;
  uint8_t leftStickY;
  uint8_t rightStickX;
  uint8_t rightStickY;
  uint8_t analogButtons[12];
  uint32_t reserved;
};

class PadService : public Service {
public:
  PadService();
  ~PadService() override;

  std::string GetName() const override { return "scePad"; }
  void Initialize() override;

  // scePad functions
  int PadInit();
  int PadOpen(int userId, int type, int index, void *param);
  int PadRead(int handle, OrbisPadData *data, int numPads);

private:
  std::mutex padMutex;
  std::vector<SDL_Gamepad *> gamepads;

  void UpdatePadData(OrbisPadData *data, SDL_Gamepad *controller);
};

} // namespace Services
} // namespace Core
