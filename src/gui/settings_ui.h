// gui/settings_ui.h
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Settings UI — Tabbed configuration dialog (RPCS3-style)

#pragma once

#include "imgui.h"
#include <string>

namespace Gui {

// Persisted emulator settings
struct LayraSettings {
  // ── Paths ──
  std::string games_directory;
  std::string firmware_directory;

  // ── Graphics ──
  int gpu_backend = 0;          // 0=Vulkan (only option for now)
  int resolution_scale = 100;   // % of native 1920x1080
  bool vsync = true;
  int anisotropic_filter = 8;   // 1, 2, 4, 8, 16
  bool fullscreen = false;

  // ── Audio ──
  int audio_backend = 0;        // 0=SDL, 1=XAudio2
  int volume = 80;              // 0-100
  bool audio_stretching = false;

  // ── System ──
  int system_language = 1;      // 0=Japanese, 1=English, ...
  bool show_splash = true;
  bool show_fps = true;
  bool log_to_file = true;
  std::string username = "LayraUser";
};

class SettingsUI {
public:
  // Opens the settings dialog
  static void Open();

  // Closes the settings dialog
  static void Close();

  // Returns true if the settings dialog is currently open
  static bool IsOpen();

  // Renders the settings dialog (call every frame when open)
  static void Render();

  // Get the current settings (read-only reference)
  static const LayraSettings& GetSettings();

  // Get mutable settings for modification
  static LayraSettings& GetMutableSettings();

  // Save settings to disk
  static void SaveSettings();

  // Load settings from disk
  static void LoadSettings();

private:
  static void RenderGraphicsTab();
  static void RenderAudioTab();
  static void RenderSystemTab();
  static void RenderPathsTab();

  // Opens a native folder picker dialog (Windows IFileDialog)
  static std::string BrowseForFolder(const char* title);
};

} // namespace Gui
