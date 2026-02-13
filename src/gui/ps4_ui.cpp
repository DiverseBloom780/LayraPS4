// ps4_ui.cpp - Authentic PS4 Dashboard UI
// Place this in: src/gui/ps4_ui.cpp
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ps4_ui.h"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>

namespace Gui {

// Application/Game tile data
struct AppTile {
    std::string name;
    std::string icon; // Future: actual icon path
    bool installed;
    float hover_anim; // Animation value 0.0-1.0
};

// PS4 UI State
static int selected_app = 0;
static int selected_menu = -1; // -1 = apps row selected, 0+ = function menu
static float scroll_offset = 0.0f;
static float scroll_target = 0.0f;
static std::vector<AppTile> apps;
static bool show_function_menu = false;

// Animation state
static float menu_fade = 0.0f;
static float time_accumulator = 0.0f;

// Initialize PS4 UI
void PS4UI::Initialize() {
    // Add some default PS4 apps/games
    apps = {
        {"What's New", "", true, 0.0f},
        {"TV & Video", "", true, 0.0f},
        {"Browser", "", true, 0.0f},
        {"Library", "", true, 0.0f},
        {"Settings", "", true, 0.0f},
        {"Power", "", true, 0.0f},
        {"Bloodborne", "", true, 0.0f},
        {"The Last of Us", "", true, 0.0f},
        {"God of War", "", true, 0.0f},
        {"Spider-Man", "", true, 0.0f},
        {"Horizon Zero Dawn", "", true, 0.0f},
        {"Uncharted 4", "", true, 0.0f},
    };
    
    selected_app = 0;
    selected_menu = -1;
    scroll_offset = 0.0f;
}

// Render the PS4 Dashboard
void PS4UI::Render() {
    ImGuiIO& io = ImGui::GetIO();
    time_accumulator += io.DeltaTime;
    
    // Full screen window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
    ImGui::Begin("##PS4UI", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    
    // Background gradient (PS4 blue)
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 screen_size = io.DisplaySize;
    
    // Animated background
    float wave = sinf(time_accumulator * 0.5f) * 0.1f + 0.9f;
    ImU32 bg_top = IM_COL32(0, 30, 60, 255);
    ImU32 bg_bottom = IM_COL32(int(20 * wave), int(60 * wave), int(100 * wave), 255);
    draw_list->AddRectFilledMultiColor(
        ImVec2(0, 0),
        screen_size,
        bg_top, bg_top, bg_bottom, bg_bottom
    );
    
    // Status bar at top
    RenderStatusBar(draw_list, screen_size);
    
    // Main content area
    float content_start_y = 60.0f;
    float apps_row_y = content_start_y + 100.0f;
    
    // Render app icons row
    RenderAppsRow(draw_list, screen_size, apps_row_y);
    
    // Function menu (if apps are selected)
    if (selected_menu == -1) {
        show_function_menu = true;
        menu_fade = std::min(menu_fade + io.DeltaTime * 4.0f, 1.0f);
    } else {
        menu_fade = std::max(menu_fade - io.DeltaTime * 4.0f, 0.0f);
        if (menu_fade <= 0.0f) show_function_menu = false;
    }
    
    if (show_function_menu) {
        RenderFunctionMenu(draw_list, screen_size, apps_row_y + 300.0f);
    }
    
    // Selection info at bottom
    RenderSelectionInfo(draw_list, screen_size);
    
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void PS4UI::RenderStatusBar(ImDrawList* draw_list, ImVec2 screen_size) {
    float bar_height = 50.0f;
    
    // Semi-transparent background
    draw_list->AddRectFilled(
        ImVec2(0, 0),
        ImVec2(screen_size.x, bar_height),
        IM_COL32(0, 0, 0, 180)
    );
    
    // User icon and name (left)
    ImVec2 user_pos(20.0f, 15.0f);
    draw_list->AddCircleFilled(
        ImVec2(user_pos.x + 15, user_pos.y + 10),
        15.0f,
        IM_COL32(100, 150, 200, 255),
        16
    );
    
    ImGui::SetCursorPos(ImVec2(user_pos.x + 40, user_pos.y + 5));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGui::Text("Player");
    ImGui::PopStyleColor();
    
    // Time (right)
    char time_str[32];
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    strftime(time_str, sizeof(time_str), "%I:%M %p", timeinfo);
    
    ImVec2 time_size = ImGui::CalcTextSize(time_str);
    ImGui::SetCursorPos(ImVec2(screen_size.x - time_size.x - 20, user_pos.y + 5));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGui::Text("%s", time_str);
    ImGui::PopStyleColor();
    
    // Notifications icon
    float notif_x = screen_size.x - time_size.x - 80;
    draw_list->AddCircleFilled(
        ImVec2(notif_x, user_pos.y + 10),
        12.0f,
        IM_COL32(255, 255, 255, 200),
        16
    );
}

void PS4UI::RenderAppsRow(ImDrawList* draw_list, ImVec2 screen_size, float y_pos) {
    float tile_size = 200.0f;
    float tile_spacing = 40.0f;
    float selected_scale = 1.3f;
    
    // Center the selected tile
    float center_x = screen_size.x * 0.5f;
    scroll_target = selected_app * (tile_size + tile_spacing);
    scroll_offset += (scroll_target - scroll_offset) * 0.15f; // Smooth scroll
    
    // Render tiles
    for (size_t i = 0; i < apps.size(); i++) {
        float tile_x = center_x - scroll_offset + i * (tile_size + tile_spacing);
        
        // Skip tiles outside screen
        if (tile_x + tile_size < -200 || tile_x > screen_size.x + 200) {
            apps[i].hover_anim = 0.0f;
            continue;
        }
        
        bool is_selected = (i == selected_app && selected_menu == -1);
        
        // Animate selection
        float target_anim = is_selected ? 1.0f : 0.0f;
        apps[i].hover_anim += (target_anim - apps[i].hover_anim) * 0.2f;
        
        float scale = 1.0f + apps[i].hover_anim * (selected_scale - 1.0f);
        float current_size = tile_size * scale;
        float offset = (current_size - tile_size) * 0.5f;
        
        ImVec2 tile_pos(tile_x - offset, y_pos - offset);
        
        // Tile shadow (for selected)
        if (apps[i].hover_anim > 0.01f) {
            float shadow_alpha = apps[i].hover_anim * 150;
            draw_list->AddRectFilled(
                ImVec2(tile_pos.x + 10, tile_pos.y + 10),
                ImVec2(tile_pos.x + current_size + 10, tile_pos.y + current_size + 10),
                IM_COL32(0, 0, 0, (int)shadow_alpha),
                10.0f
            );
        }
        
        // Tile background
        ImU32 tile_color = is_selected 
            ? IM_COL32(60, 120, 180, 255)
            : IM_COL32(40, 40, 50, 220);
        
        draw_list->AddRectFilled(
            tile_pos,
            ImVec2(tile_pos.x + current_size, tile_pos.y + current_size),
            tile_color,
            10.0f
        );
        
        // Selection border
        if (is_selected) {
            float border_thickness = 3.0f;
            draw_list->AddRect(
                tile_pos,
                ImVec2(tile_pos.x + current_size, tile_pos.y + current_size),
                IM_COL32(255, 255, 255, 255),
                10.0f,
                0,
                border_thickness
            );
        }
        
        // App icon placeholder (would be actual icon)
        ImVec2 icon_pos(
            tile_pos.x + current_size * 0.5f,
            tile_pos.y + current_size * 0.4f
        );
        draw_list->AddCircleFilled(
            icon_pos,
            current_size * 0.25f,
            IM_COL32(200, 200, 200, 180),
            32
        );
        
        // App name
        ImVec2 text_size = ImGui::CalcTextSize(apps[i].name.c_str());
        ImVec2 text_pos(
            tile_pos.x + (current_size - text_size.x) * 0.5f,
            tile_pos.y + current_size - 40.0f
        );
        
        draw_list->AddText(
            text_pos,
            IM_COL32(255, 255, 255, 255),
            apps[i].name.c_str()
        );
    }
}

void PS4UI::RenderFunctionMenu(ImDrawList* draw_list, ImVec2 screen_size, float y_pos) {
    const char* menu_items[] = {
        "Close Application",
        "Information",
        "Add to Favorites",
        "Options"
    };
    int menu_count = 4;
    
    float menu_alpha = menu_fade;
    float menu_height = 50.0f;
    float menu_width = 400.0f;
    float menu_x = (screen_size.x - menu_width) * 0.5f;
    
    for (int i = 0; i < menu_count; i++) {
        float item_y = y_pos + i * (menu_height + 10.0f);
        
        // Menu item background
        ImU32 bg_color = (selected_menu == i) 
            ? IM_COL32(255, 255, 255, (int)(220 * menu_alpha))
            : IM_COL32(50, 50, 60, (int)(180 * menu_alpha));
        
        draw_list->AddRectFilled(
            ImVec2(menu_x, item_y),
            ImVec2(menu_x + menu_width, item_y + menu_height),
            bg_color,
            5.0f
        );
        
        // Text
        ImVec2 text_size = ImGui::CalcTextSize(menu_items[i]);
        ImU32 text_color = (selected_menu == i)
            ? IM_COL32(0, 0, 0, (int)(255 * menu_alpha))
            : IM_COL32(255, 255, 255, (int)(255 * menu_alpha));
        
        draw_list->AddText(
            ImVec2(menu_x + 20, item_y + (menu_height - text_size.y) * 0.5f),
            text_color,
            menu_items[i]
        );
    }
}

void PS4UI::RenderSelectionInfo(ImDrawList* draw_list, ImVec2 screen_size) {
    if (selected_app >= 0 && selected_app < apps.size()) {
        float info_y = screen_size.y - 60.0f;
        
        // Background bar
        draw_list->AddRectFilled(
            ImVec2(0, info_y),
            ImVec2(screen_size.x, screen_size.y),
            IM_COL32(0, 0, 0, 180)
        );
        
        // Selected app name (larger)
        const char* app_name = apps[selected_app].name.c_str();
        ImGui::PushFont(ImGui::GetFont()); // Would use larger font here
        ImVec2 text_size = ImGui::CalcTextSize(app_name);
        
        draw_list->AddText(
            ImVec2((screen_size.x - text_size.x) * 0.5f, info_y + 15),
            IM_COL32(255, 255, 255, 255),
            app_name
        );
        ImGui::PopFont();
        
        // Navigation hints
        const char* hint = "◁ ▷ Navigate    ✕ Select    ⬜ Options";
        ImVec2 hint_size = ImGui::CalcTextSize(hint);
        draw_list->AddText(
            ImVec2((screen_size.x - hint_size.x) * 0.5f, screen_size.y - 20),
            IM_COL32(180, 180, 180, 255),
            hint
        );
    }
}

void PS4UI::HandleInput() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Keyboard navigation
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        if (selected_menu == -1) {
            // Navigate apps
            selected_app = std::max(0, selected_app - 1);
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        if (selected_menu == -1) {
            // Navigate apps
            selected_app = std::min((int)apps.size() - 1, selected_app + 1);
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if (selected_menu == -1) {
            // Move to function menu
            selected_menu = 0;
        } else {
            // Navigate menu down
            selected_menu = std::min(3, selected_menu + 1);
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if (selected_menu > 0) {
            // Navigate menu up
            selected_menu--;
        } else if (selected_menu == 0) {
            // Back to apps
            selected_menu = -1;
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space)) {
        if (selected_menu == -1 && selected_app >= 0) {
            // Launch selected app
            printf("[PS4UI] Launching: %s\n", apps[selected_app].name.c_str());
        } else if (selected_menu >= 0) {
            // Execute menu action
            printf("[PS4UI] Menu action: %d\n", selected_menu);
        }
    }
}

} // namespace Gui
