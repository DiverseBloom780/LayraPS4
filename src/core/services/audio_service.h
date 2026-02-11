// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "service_manager.h"
#include <SDL3/SDL.h>
#include <string>

namespace Core {
namespace Services {

class AudioService : public Service {
public:
  AudioService();
  ~AudioService() override;

  std::string GetName() const override { return "sceAudio"; }
  void Initialize() override;

  // sceAudio functions
  int AudioOutOpen(int portType, int numChannels, int sampleRate,
                   int sampleFormat);
  void AudioOutClose(int handle);
  int AudioOutOutput(int handle, const void *data);

private:
  bool initialized = false;
  SDL_AudioStream *audioStream = nullptr;
};

} // namespace Services
} // namespace Core
