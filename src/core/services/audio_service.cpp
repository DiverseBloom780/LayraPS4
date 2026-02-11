#include "audio_service.h"
#include <iostream>

namespace Core {
namespace Services {

AudioService::AudioService() {
  std::cout << "[Audio] sceAudio service created.\n";
}

AudioService::~AudioService() {
  if (audioStream) {
    SDL_DestroyAudioStream(audioStream);
  }
}

void AudioService::Initialize() {
  std::cout << "[Audio] Initializing SDL3 audio backend...\n";

  // In SDL3, we use SDL_OpenAudioDevice and SDL_AudioStream
  SDL_AudioSpec spec;
  spec.format = SDL_AUDIO_S16LE;
  spec.channels = 2;
  spec.freq = 48000;

  audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                          &spec, nullptr, nullptr);
  if (!audioStream) {
    std::cerr << "[Audio] Failed to open audio device stream: "
              << SDL_GetError() << "\n";
    return;
  }

  SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioStream));
  initialized = true;
  std::cout << "[Audio] SDL3 audio initialized successfully.\n";
}

int AudioService::AudioOutOpen(int portType, int numChannels, int sampleRate,
                               int sampleFormat) {
  std::cout << "[Audio] sceAudioOutOpen(portType=" << portType
            << ", channels=" << numChannels << ", rate=" << sampleRate << ")\n";
  return 1; // Return a dummy handle
}

void AudioService::AudioOutClose(int handle) {
  std::cout << "[Audio] sceAudioOutClose(handle=" << handle << ")\n";
}

int AudioService::AudioOutOutput(int handle, const void *data) {
  if (initialized && audioStream) {
    // PS4 usually outputs 256 samples per block
    // SDL_PutAudioStreamData will queue the samples
    if (SDL_PutAudioStreamData(audioStream, data, 256 * 2 * 2) == 0) {
      return 0;
    }
  }
  return -1;
}

} // namespace Services
} // namespace Core
