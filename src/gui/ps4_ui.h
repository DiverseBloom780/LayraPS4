// ps4_ui.h - Authentic PS4 Dashboard UI
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "imgui.h"
#include <functional>
#include <string>

namespace Gui {

// Callback type for launching a game executable
using LaunchCallback = std::function<void(const std::string &exe_path)>;

class PS4UI {
public:
  // Initialize the PS4 UI system
  static void Initialize();

  // Render the PS4 dashboard
  static void Render();

  // Handle user input
  static void HandleInput();

  // Set the games directory for discovery
  static void SetGamesDirectory(const std::string &path);

  // Set callback for when a game is launched from the UI
  static void SetLaunchCallback(LaunchCallback callback);

  // Returns true if a game is currently running (hides dashboard)
  static bool IsGameRunning();

private:
  // Scan for games in the configured directory
  static void ScanGamesDirectory();

  // Render individual components
  static void RenderStatusBar(ImDrawList *draw_list, ImVec2 screen_size);
  static void RenderAppsRow(ImDrawList *draw_list, ImVec2 screen_size,
                            float y_pos);
  static void RenderFunctionMenu(ImDrawList *draw_list, ImVec2 screen_size,
                                 float y_pos);
  static void RenderSelectionInfo(ImDrawList *draw_list, ImVec2 screen_size);
};

} // namespace Gui
