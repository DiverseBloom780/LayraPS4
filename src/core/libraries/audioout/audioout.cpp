// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// AudioOut HLE — manages audio output ports with SDL3 audio backend.

#include "audioout.h"
#include <SDL3/SDL.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

namespace Core {
namespace Libraries {
namespace AudioOut {

// PS4 audio format constants
enum OrbisAudioOutParam : u32 {
  ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_MONO = 0,
  ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO = 1,
  ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH = 2,
  ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_MONO = 3,
  ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO = 4,
  ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH = 5,
};

enum OrbisAudioOutPort : s32 {
  ORBIS_AUDIO_OUT_PORT_TYPE_MAIN = 0,
  ORBIS_AUDIO_OUT_PORT_TYPE_BGM = 1,
  ORBIS_AUDIO_OUT_PORT_TYPE_VOICE = 2,
  ORBIS_AUDIO_OUT_PORT_TYPE_PERSONAL = 3,
  ORBIS_AUDIO_OUT_PORT_TYPE_PADSPK = 4,
  ORBIS_AUDIO_OUT_PORT_TYPE_AUX = 127,
};

static constexpr int MAX_AUDIO_PORTS = 8;

struct AudioPort {
  bool is_open = false;
  s32 port_type = 0;
  u32 sample_rate = 48000;
  u32 format = 0;
  u32 grain = 256;          // Samples per output call
  u32 channels = 2;
  u32 sample_size = 2;      // Bytes per sample (2 for S16, 4 for float)
  s32 volume = 0x8000;      // Linear 0-0x8000 (max)

  SDL_AudioStream *sdl_stream = nullptr;
  u64 samples_written = 0;
};

static AudioPort g_audio_ports[MAX_AUDIO_PORTS];
static bool g_audio_initialized = false;
static std::mutex g_audio_mutex;

static u32 GetChannelCount(u32 format) {
  switch (format) {
    case ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_MONO:
    case ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_MONO:
      return 1;
    case ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO:
    case ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO:
      return 2;
    case ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_8CH:
    case ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH:
      return 8;
    default:
      return 2;
  }
}

static u32 GetSampleSize(u32 format) {
  switch (format) {
    case ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_MONO:
    case ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO:
    case ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_8CH:
      return 4;
    default:
      return 2; // S16
  }
}

static s32 PS4_SYSV_ABI sceAudioOutInit() {
  std::lock_guard<std::mutex> lock(g_audio_mutex);
  if (g_audio_initialized) return 0;

  printf("[AudioOut] sceAudioOutInit\n");
  g_audio_initialized = true;
  return 0;
}

static s32 PS4_SYSV_ABI sceAudioOutOpen(s32 userId, s32 portType,
                                          s32 index, u32 sampleRate,
                                          u32 format) {
  std::lock_guard<std::mutex> lock(g_audio_mutex);

  u32 grain = 256; // Default
  u32 channels = GetChannelCount(format);
  u32 sampleSize = GetSampleSize(format);

  printf("[AudioOut] sceAudioOutOpen: user=%d, type=%d, rate=%u, "
         "fmt=%u, ch=%u\n",
         userId, portType, sampleRate, format, channels);

  // Find a free port
  for (int i = 0; i < MAX_AUDIO_PORTS; i++) {
    if (!g_audio_ports[i].is_open) {
      auto &port = g_audio_ports[i];
      port.is_open = true;
      port.port_type = portType;
      port.sample_rate = sampleRate;
      port.format = format;
      port.grain = grain;
      port.channels = channels;
      port.sample_size = sampleSize;
      port.samples_written = 0;

      // Create SDL audio stream
      SDL_AudioSpec srcSpec{};
      srcSpec.freq = static_cast<int>(sampleRate);
      srcSpec.channels = static_cast<int>(channels);
      srcSpec.format = (sampleSize == 4) ? SDL_AUDIO_F32
                                          : SDL_AUDIO_S16;

      port.sdl_stream = SDL_OpenAudioDeviceStream(
          SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &srcSpec, nullptr, nullptr);

      if (port.sdl_stream) {
        SDL_ResumeAudioStreamDevice(port.sdl_stream);
        printf("[AudioOut]   Port %d opened with SDL stream\n", i);
      } else {
        printf("[AudioOut]   Port %d opened (SDL stream failed: %s)\n",
               i, SDL_GetError());
      }

      return i; // Return port handle
    }
  }

  fprintf(stderr, "[AudioOut] ERROR: No free audio ports!\n");
  return -1; // SCE_AUDIO_OUT_ERROR_PORT_FULL
}

static s32 PS4_SYSV_ABI sceAudioOutClose(s32 handle) {
  std::lock_guard<std::mutex> lock(g_audio_mutex);
  if (handle < 0 || handle >= MAX_AUDIO_PORTS) return -1;

  auto &port = g_audio_ports[handle];
  if (!port.is_open) return -1;

  printf("[AudioOut] sceAudioOutClose: handle=%d (%llu samples written)\n",
         handle, (unsigned long long)port.samples_written);

  if (port.sdl_stream) {
    SDL_DestroyAudioStream(port.sdl_stream);
    port.sdl_stream = nullptr;
  }
  port.is_open = false;
  return 0;
}

static s32 PS4_SYSV_ABI sceAudioOutOutput(s32 handle, const void *buffer) {
  if (handle < 0 || handle >= MAX_AUDIO_PORTS) return -1;

  auto &port = g_audio_ports[handle];
  if (!port.is_open) return -1;

  u32 bufferSize = port.grain * port.channels * port.sample_size;

  if (buffer && port.sdl_stream) {
    SDL_PutAudioStreamData(port.sdl_stream, buffer,
                            static_cast<int>(bufferSize));
  }

  port.samples_written += port.grain;
  return 0;
}

static s32 PS4_SYSV_ABI sceAudioOutSetVolume(s32 handle, s32 flag,
                                                s32 *vol) {
  if (handle < 0 || handle >= MAX_AUDIO_PORTS) return -1;
  auto &port = g_audio_ports[handle];
  if (!port.is_open) return -1;

  if (vol) {
    port.volume = vol[0];
  }
  return 0;
}

static s32 PS4_SYSV_ABI sceAudioOutGetPortState(s32 handle, void *state) {
  if (handle < 0 || handle >= MAX_AUDIO_PORTS) return -1;
  if (!g_audio_ports[handle].is_open) return -1;
  // State struct: first byte = 1 (connected), rest zeroed
  if (state) {
    memset(state, 0, 64);
    *static_cast<u8*>(state) = 1; // output = connected
  }
  return 0;
}

static s32 PS4_SYSV_ABI sceAudioOutSetMixLevelPadSpk(s32 handle,
                                                        s32 mixLevel) {
  return 0;
}

void RegisterAudioOut(::Core::Kernel::ModuleManager *module_manager) {
  printf("[AudioOut] Registering AudioOut\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  LIB_FUNCTION("JfEPXVxhFqA", "libSceAudioOut", 1, "libSceAudioOut",
               sceAudioOutInit);
  LIB_FUNCTION("ekNvsT22rsY", "libSceAudioOut", 1, "libSceAudioOut",
               sceAudioOutOpen);
  LIB_FUNCTION("b+uAV89IlxE", "libSceAudioOut", 1, "libSceAudioOut",
               sceAudioOutClose);
  LIB_FUNCTION("w3PdaSTSwGE", "libSceAudioOut", 1, "libSceAudioOut",
               sceAudioOutOutput);
  LIB_FUNCTION("QOQtbeDqsT4", "libSceAudioOut", 1, "libSceAudioOut",
               sceAudioOutSetVolume);
  LIB_FUNCTION("qLpFWJHnPMQ", "libSceAudioOut", 1, "libSceAudioOut",
               sceAudioOutGetPortState);
  LIB_FUNCTION("oPLghhAKgMo", "libSceAudioOut", 1, "libSceAudioOut",
               sceAudioOutSetMixLevelPadSpk);

#undef LIB_FUNCTION
}

} // namespace AudioOut
} // namespace Libraries
} // namespace Core
