// gui/settings_ui.h
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "imgui.h"
#include <string>

namespace Gui {

struct LayraSettings {
  std::string games_directory;
  std::string firmware_directory;

  int gpu_backend = 0;
  int resolution_scale = 100;
  bool vsync = true;
  int anisotropic_filter = 8;
  bool fullscreen = false;

  int audio_backend = 0;
  int volume = 80;
  bool audio_stretching = false;

  int system_language = 1;
  bool show_splash = true;
  bool show_fps = true;
  bool log_to_file = true;
  std::string username = "LayraUser";
};

class SettingsUI {
public:
  static void Open();
  static void Close();
  static bool IsOpen();
  static void Render();
  static const LayraSettings& GetSettings();
  static LayraSettings& GetMutableSettings();
  static void SaveSettings();
  static void LoadSettings();

private:
  static void RenderOverviewTab();
  static void RenderGraphicsTab();
  static void RenderAudioTab();
  static void RenderSystemTab();
  static void RenderPathsTab();
  static std::string BrowseForFolder(const char* title);
};

} // namespace Gui
