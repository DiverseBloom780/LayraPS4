// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ps4_ui.h"
#include "settings_ui.h"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Gui {

struct AppTile {
  std::string name;
  std::string icon;
  std::string exe_path;
  bool installed;
  bool is_game;
  float hover_anim;
};

static int selected_app = 0;
static int selected_menu = -1;
static int selected_sidebar = 0;
static float scroll_offset = 0.0f;
static float scroll_target = 0.0f;
static std::vector<AppTile> apps;
static bool show_function_menu = false;
static std::string games_directory;
static bool s_game_running = false;
static Gui::LaunchCallback s_launch_callback;

static float menu_fade = 0.0f;
static float time_accumulator = 0.0f;

static std::string FindGameIconPath(const std::filesystem::path &game_root) {
  namespace fs = std::filesystem;
  fs::path icon_file = game_root / "sce_sys" / "icon0.png";
  if (fs::is_regular_file(icon_file)) {
    return icon_file.string();
  }
  return {};
}

static void DrawRoundedPanel(ImDrawList *draw_list, ImVec2 pos, ImVec2 size,
                             ImU32 fill, ImU32 outline, float rounding = 18.0f) {
  draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), fill,
                           rounding);
  draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), outline,
                     rounding, 0, 1.5f);
}

void PS4UI::SetGamesDirectory(const std::string &path) {
  games_directory = path;
  printf("[PS4UI] Games directory set to: %s\n", path.c_str());
}

void PS4UI::SetLaunchCallback(LaunchCallback callback) {
  s_launch_callback = std::move(callback);
}

bool PS4UI::IsGameRunning() { return s_game_running; }

void PS4UI::ScanGamesDirectory() {
  namespace fs = std::filesystem;

  if (games_directory.empty()) {
    printf("[PS4UI] No games directory configured\n");
    return;
  }

  if (!fs::exists(games_directory) || !fs::is_directory(games_directory)) {
    printf("[PS4UI] Games directory does not exist: %s\n", games_directory.c_str());
    return;
  }

  printf("[PS4UI] Scanning for games in: %s\n", games_directory.c_str());
  int found = 0;

  std::error_code ec;
  for (const auto &entry : fs::recursive_directory_iterator(games_directory, ec)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    auto filename = entry.path().filename().string();
    if (filename == "eboot.bin" || entry.path().extension() == ".elf" ||
        entry.path().extension() == ".self") {
      std::string game_name = entry.path().parent_path().filename().string();
      std::string exe = entry.path().string();
      std::string icon = FindGameIconPath(entry.path().parent_path());

      bool exists = false;
      for (const auto &app : apps) {
        if (app.exe_path == exe) {
          exists = true;
          break;
        }
      }
      if (exists) {
        continue;
      }

      apps.push_back({game_name, icon, exe, true, true, 0.0f});
      printf("[PS4UI]   Found: %s -> %s\n", game_name.c_str(), exe.c_str());
      found++;
    }
  }

  printf("[PS4UI] Scan complete: %d game(s) found\n", found);
}

void PS4UI::Initialize() {
  SettingsUI::LoadSettings();

  const auto &settings = SettingsUI::GetSettings();
  if (!settings.games_directory.empty()) {
    games_directory = settings.games_directory;
  }

  apps = {
      {"What's New", "", "", true, false, 0.0f},
      {"Games", "", "", true, false, 0.0f},
      {"Media", "", "", true, false, 0.0f},
      {"Library", "", "", true, false, 0.0f},
      {"Settings", "", "", true, false, 0.0f},
  };

  ScanGamesDirectory();

  if (std::none_of(apps.begin(), apps.end(), [](const AppTile &a) { return a.is_game; })) {
    apps.push_back({"No Games Found", "", "", false, true, 0.0f});
  }

  selected_app = 0;
  selected_menu = -1;
  selected_sidebar = 0;
  scroll_offset = 0.0f;
}

void PS4UI::Render() {
  ImGuiIO &io = ImGui::GetIO();
  time_accumulator += io.DeltaTime;

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  ImGui::Begin("##PS4UI", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse |
                   ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2 screen_size = io.DisplaySize;

  float wave = sinf(time_accumulator * 0.45f) * 0.08f + 0.92f;
  ImU32 bg_top = IM_COL32(5, 18, 38, 255);
  ImU32 bg_bottom = IM_COL32(18, 56, 100, 255);
  draw_list->AddRectFilledMultiColor(ImVec2(0, 0), screen_size, bg_top, bg_top,
                                     bg_bottom, bg_bottom);

  draw_list->AddCircleFilled(ImVec2(screen_size.x * 0.82f, screen_size.y * 0.18f),
                             200.0f, IM_COL32(50, 120, 220, 50), 48);

  RenderStatusBar(draw_list, screen_size);
  RenderAppsRow(draw_list, screen_size, 130.0f);
  RenderFunctionMenu(draw_list, screen_size, 400.0f);
  RenderSelectionInfo(draw_list, screen_size);

  if (SettingsUI::IsOpen()) {
    SettingsUI::Render();
  }

  ImGui::End();
  ImGui::PopStyleVar(2);
}

void PS4UI::RenderStatusBar(ImDrawList *draw_list, ImVec2 screen_size) {
  float bar_height = 54.0f;
  DrawRoundedPanel(draw_list, ImVec2(0, 0), ImVec2(screen_size.x, bar_height),
                   IM_COL32(6, 12, 22, 220), IM_COL32(90, 120, 180, 90), 0.0f);

  ImVec2 user_pos(24.0f, 12.0f);
  draw_list->AddCircleFilled(ImVec2(user_pos.x + 16, user_pos.y + 16), 16.0f,
                             IM_COL32(120, 180, 255, 255), 24);
  draw_list->AddText(ImVec2(user_pos.x + 44, user_pos.y + 8),
                     IM_COL32(255, 255, 255, 255), "LayraUser");

  char time_str[24];
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  strftime(time_str, sizeof(time_str), "%I:%M %p", timeinfo);
  draw_list->AddText(ImVec2(screen_size.x - 140.0f, 12.0f),
                     IM_COL32(255, 255, 255, 255), time_str);

  draw_list->AddCircleFilled(ImVec2(screen_size.x - 100.0f, 20.0f), 8.0f,
                             IM_COL32(90, 220, 120, 255), 12);
  draw_list->AddCircleFilled(ImVec2(screen_size.x - 78.0f, 20.0f), 8.0f,
                             IM_COL32(255, 220, 90, 255), 12);
  draw_list->AddCircleFilled(ImVec2(screen_size.x - 56.0f, 20.0f), 8.0f,
                             IM_COL32(255, 255, 255, 180), 12);
}

void PS4UI::RenderAppsRow(ImDrawList *draw_list, ImVec2 screen_size, float y_pos) {
  const float panel_x = 24.0f;
  const float panel_y = 78.0f;
  const float panel_w = screen_size.x - 48.0f;
  const float panel_h = 300.0f;

  DrawRoundedPanel(draw_list, ImVec2(panel_x, panel_y), ImVec2(panel_w, panel_h),
                   IM_COL32(8, 18, 35, 210), IM_COL32(90, 120, 180, 110), 24.0f);

  draw_list->AddText(ImVec2(panel_x + 24.0f, panel_y + 20.0f),
                     IM_COL32(255, 255, 255, 255), "PS4 Home");
  draw_list->AddText(ImVec2(panel_x + 24.0f, panel_y + 52.0f),
                     IM_COL32(150, 180, 220, 255), "Featured content and quick access");

  float tile_size = 146.0f;
  float tile_spacing = 24.0f;
  float start_x = panel_x + 24.0f;
  float start_y = panel_y + 90.0f;

  for (size_t i = 0; i < apps.size(); ++i) {
    float tile_x = start_x + i * (tile_size + tile_spacing);
    if (tile_x + tile_size > screen_size.x - 24.0f) {
      break;
    }

    bool is_selected = (i == selected_app && selected_menu == -1);
    float hover = is_selected ? 1.0f : 0.0f;
    float scale = 1.0f + hover * 0.06f;
    float current_size = tile_size * scale;

    ImVec2 tile_pos(tile_x, start_y);
    ImVec2 tile_end(tile_x + current_size, start_y + current_size);

    ImU32 base = is_selected ? IM_COL32(60, 120, 190, 255)
                             : IM_COL32(30, 40, 60, 230);
    DrawRoundedPanel(draw_list, tile_pos, ImVec2(current_size, current_size),
                     base, IM_COL32(255, 255, 255, 90), 16.0f);

    if (is_selected) {
      draw_list->AddCircleFilled(ImVec2(tile_x + current_size - 18.0f, start_y + 18.0f),
                                 7.0f, IM_COL32(255, 255, 255, 255), 12);
    }

    ImVec2 icon_pos(tile_x + current_size * 0.5f, start_y + current_size * 0.4f);
    draw_list->AddCircleFilled(icon_pos, current_size * 0.24f,
                               apps[i].icon.empty() ? IM_COL32(200, 200, 220, 220)
                                                    : IM_COL32(90, 200, 255, 220), 24);

    ImVec2 text_size = ImGui::CalcTextSize(apps[i].name.c_str());
    draw_list->AddText(ImVec2(tile_x + (current_size - text_size.x) * 0.5f,
                              start_y + current_size - 34.0f),
                       IM_COL32(255, 255, 255, 255), apps[i].name.c_str());
  }
}

void PS4UI::RenderFunctionMenu(ImDrawList *draw_list, ImVec2 screen_size, float y_pos) {
  const char *actions[] = {"Launch", "Information", "Favorite", "Settings"};
  float panel_x = screen_size.x - 280.0f;
  float panel_y = y_pos;
  float panel_w = 240.0f;
  float panel_h = 200.0f;

  DrawRoundedPanel(draw_list, ImVec2(panel_x, panel_y), ImVec2(panel_w, panel_h),
                   IM_COL32(10, 16, 28, 220), IM_COL32(90, 120, 180, 110), 18.0f);

  draw_list->AddText(ImVec2(panel_x + 18.0f, panel_y + 16.0f),
                     IM_COL32(255, 255, 255, 255), "Quick Actions");

  for (int i = 0; i < 4; ++i) {
    float item_y = panel_y + 48.0f + i * 34.0f;
    ImU32 bg = (selected_menu == i) ? IM_COL32(88, 130, 210, 255)
                                    : IM_COL32(30, 45, 70, 220);
    draw_list->AddRectFilled(ImVec2(panel_x + 12.0f, item_y),
                             ImVec2(panel_x + panel_w - 12.0f, item_y + 24.0f),
                             bg, 8.0f);
    draw_list->AddText(ImVec2(panel_x + 24.0f, item_y + 4.0f),
                       IM_COL32(255, 255, 255, 255), actions[i]);
  }
}

void PS4UI::RenderSelectionInfo(ImDrawList *draw_list, ImVec2 screen_size) {
  if (selected_app >= 0 && selected_app < apps.size()) {
    float info_y = screen_size.y - 70.0f;
    DrawRoundedPanel(draw_list, ImVec2(22.0f, info_y),
                     ImVec2(screen_size.x - 44.0f, 44.0f),
                     IM_COL32(0, 0, 0, 185), IM_COL32(90, 120, 180, 90), 12.0f);

    const char *app_name = apps[selected_app].name.c_str();
    draw_list->AddText(ImVec2(40.0f, info_y + 12.0f),
                       IM_COL32(255, 255, 255, 255), app_name);

    const char *hint = "← → Navigate   Enter Launch   Tab Settings";
    draw_list->AddText(ImVec2(screen_size.x - 300.0f, info_y + 12.0f),
                       IM_COL32(180, 180, 180, 255), hint);
  }
}

void PS4UI::HandleInput() {
  if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
    selected_app = std::max(0, selected_app - 1);
    selected_menu = -1;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
    selected_app = std::min((int)apps.size() - 1, selected_app + 1);
    selected_menu = -1;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
    if (selected_menu == -1) {
      selected_menu = 0;
    } else {
      selected_menu = std::min(3, selected_menu + 1);
    }
  }

  if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
    if (selected_menu > 0) {
      selected_menu--;
    } else if (selected_menu == 0) {
      selected_menu = -1;
    }
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
    selected_sidebar = (selected_sidebar + 1) % 5;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space)) {
    if (selected_menu >= 0) {
      if (selected_menu == 3) {
        SettingsUI::Open();
      }
      return;
    }

    if (selected_app >= 0 && selected_app < static_cast<int>(apps.size())) {
      printf("[PS4UI] Launching: %s\n", apps[selected_app].name.c_str());
      if (apps[selected_app].name == "Settings") {
        SettingsUI::Open();
      } else if (apps[selected_app].is_game && !apps[selected_app].exe_path.empty()) {
        if (s_launch_callback) {
          s_game_running = true;
          s_launch_callback(apps[selected_app].exe_path);
        }
      }
    }
  }
}

} // namespace Gui
