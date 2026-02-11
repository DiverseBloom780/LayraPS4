// ps4_ui.h - Authentic PS4 Dashboard UI
// Place this in: src/gui/ps4_ui.h
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "imgui.h"

namespace Gui {

class PS4UI {
public:
    // Initialize the PS4 UI system
    static void Initialize();
    
    // Render the PS4 dashboard
    static void Render();
    
    // Handle user input
    static void HandleInput();

private:
    // Render individual components
    static void RenderStatusBar(ImDrawList* draw_list, ImVec2 screen_size);
    static void RenderAppsRow(ImDrawList* draw_list, ImVec2 screen_size, float y_pos);
    static void RenderFunctionMenu(ImDrawList* draw_list, ImVec2 screen_size, float y_pos);
    static void RenderSelectionInfo(ImDrawList* draw_list, ImVec2 screen_size);
};

} // namespace Gui
