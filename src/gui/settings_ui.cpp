// gui/settings_ui.cpp
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "settings_ui.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#endif

namespace Gui {

static bool s_open = false;
static LayraSettings s_settings{};
static int s_activeTab = 0;

// ─── Settings file path ──────────────────────────────────────
static std::string GetSettingsPath() {
  // Store settings next to the executable
  return "layra_settings.ini";
}

// ─── Native Folder Picker (Windows) ──────────────────────────
std::string SettingsUI::BrowseForFolder(const char* title) {
#ifdef _WIN32
  std::string result;

  // Use modern IFileDialog (Vista+)
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  bool needUninit = SUCCEEDED(hr);

  IFileDialog* pfd = nullptr;
  hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                        IID_IFileDialog, reinterpret_cast<void**>(&pfd));
  if (SUCCEEDED(hr)) {
    // Set as folder picker
    DWORD dwOptions;
    pfd->GetOptions(&dwOptions);
    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

    // Set title
    wchar_t wTitle[256];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wTitle, 256);
    pfd->SetTitle(wTitle);

    // Show dialog
    hr = pfd->Show(nullptr);
    if (SUCCEEDED(hr)) {
      IShellItem* psi = nullptr;
      hr = pfd->GetResult(&psi);
      if (SUCCEEDED(hr)) {
        PWSTR pszPath = nullptr;
        hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
        if (SUCCEEDED(hr)) {
          // Convert wide string to UTF-8
          int len = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1,
                                        nullptr, 0, nullptr, nullptr);
          result.resize(len - 1);
          WideCharToMultiByte(CP_UTF8, 0, pszPath, -1,
                              result.data(), len, nullptr, nullptr);
          CoTaskMemFree(pszPath);
        }
        psi->Release();
      }
    }
    pfd->Release();
  }

  if (needUninit) CoUninitialize();
  return result;
#else
  // Linux/macOS: would use zenity or similar
  return "";
#endif
}

// ─── Public API ──────────────────────────────────────────────
void SettingsUI::Open() {
  s_open = true;
  s_activeTab = 0;
  printf("[Settings] Settings dialog opened\n");
}

void SettingsUI::Close() {
  s_open = false;
  printf("[Settings] Settings dialog closed\n");
}

bool SettingsUI::IsOpen() { return s_open; }

const LayraSettings& SettingsUI::GetSettings() { return s_settings; }
LayraSettings& SettingsUI::GetMutableSettings() { return s_settings; }

// ─── Main Render ─────────────────────────────────────────────
void SettingsUI::Render() {
  if (!s_open) return;

  ImGuiIO& io = ImGui::GetIO();
  ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
  ImVec2 dialogSize(780.0f, 560.0f);

  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(dialogSize, ImGuiCond_Appearing);

  // ── Dark themed window ──
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.12f, 0.97f));
  ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.06f, 0.06f, 0.10f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.10f, 0.15f, 0.30f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.12f, 0.12f, 0.18f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TabSelected, ImVec4(0.18f, 0.25f, 0.50f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.25f, 0.35f, 0.60f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.22f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.25f, 0.45f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.35f, 0.60f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.30f, 0.50f, 0.90f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 4.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));

  bool still_open = true;
  if (ImGui::Begin("Settings##LayraSettings", &still_open,
                    ImGuiWindowFlags_NoCollapse)) {

    // ── Tab Bar ──
    if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None)) {
      if (ImGui::BeginTabItem(" Graphics ")) {
        RenderGraphicsTab();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem(" Audio ")) {
        RenderAudioTab();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem(" System ")) {
        RenderSystemTab();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem(" Paths ")) {
        RenderPathsTab();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }

    // ── Bottom buttons ──
    ImGui::Separator();
    float buttonWidth = 120.0f;
    float spacing = 10.0f;
    float totalWidth = buttonWidth * 3 + spacing * 2;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

    if (ImGui::Button("Save", ImVec2(buttonWidth, 36))) {
      SaveSettings();
      printf("[Settings] Settings saved\n");
    }
    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Apply", ImVec2(buttonWidth, 36))) {
      SaveSettings();
      printf("[Settings] Settings applied\n");
    }
    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Close", ImVec2(buttonWidth, 36))) {
      s_open = false;
    }
  }
  ImGui::End();

  ImGui::PopStyleVar(4);
  ImGui::PopStyleColor(10);

  if (!still_open) {
    s_open = false;
  }
}

// ─── Graphics Tab ────────────────────────────────────────────
void SettingsUI::RenderGraphicsTab() {
  ImGui::Spacing();

  // GPU Backend
  const char* backends[] = { "Vulkan" };
  ImGui::Text("GPU Backend");
  ImGui::SameLine(200);
  ImGui::SetNextItemWidth(250);
  ImGui::Combo("##gpu_backend", &s_settings.gpu_backend, backends, 1);

  ImGui::Spacing();

  // Resolution Scale
  ImGui::Text("Resolution Scale");
  ImGui::SameLine(200);
  ImGui::SetNextItemWidth(250);
  ImGui::SliderInt("##res_scale", &s_settings.resolution_scale, 50, 300, "%d%%");

  ImGui::Spacing();

  // VSync
  ImGui::Text("VSync");
  ImGui::SameLine(200);
  ImGui::Checkbox("##vsync", &s_settings.vsync);

  ImGui::Spacing();

  // Anisotropic Filtering
  const char* aniso_levels[] = { "1x", "2x", "4x", "8x", "16x" };
  int aniso_idx = 0;
  switch (s_settings.anisotropic_filter) {
    case 1:  aniso_idx = 0; break;
    case 2:  aniso_idx = 1; break;
    case 4:  aniso_idx = 2; break;
    case 8:  aniso_idx = 3; break;
    case 16: aniso_idx = 4; break;
  }
  ImGui::Text("Anisotropic Filter");
  ImGui::SameLine(200);
  ImGui::SetNextItemWidth(250);
  if (ImGui::Combo("##aniso", &aniso_idx, aniso_levels, 5)) {
    int vals[] = {1, 2, 4, 8, 16};
    s_settings.anisotropic_filter = vals[aniso_idx];
  }

  ImGui::Spacing();

  // Fullscreen
  ImGui::Text("Fullscreen");
  ImGui::SameLine(200);
  ImGui::Checkbox("##fullscreen", &s_settings.fullscreen);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Info box
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.7f, 0.9f, 1.0f));
  ImGui::TextWrapped("Graphics settings control the Vulkan rendering pipeline. "
                     "Resolution scaling affects internal render targets. "
                     "Changes may require a restart.");
  ImGui::PopStyleColor();
}

// ─── Audio Tab ───────────────────────────────────────────────
void SettingsUI::RenderAudioTab() {
  ImGui::Spacing();

  // Audio Backend
  const char* audio_backends[] = { "SDL3 Audio", "XAudio2 (Windows)" };
  ImGui::Text("Audio Backend");
  ImGui::SameLine(200);
  ImGui::SetNextItemWidth(250);
  ImGui::Combo("##audio_backend", &s_settings.audio_backend, audio_backends, 2);

  ImGui::Spacing();

  // Master Volume
  ImGui::Text("Master Volume");
  ImGui::SameLine(200);
  ImGui::SetNextItemWidth(250);
  ImGui::SliderInt("##volume", &s_settings.volume, 0, 100, "%d%%");

  ImGui::Spacing();

  // Audio Stretching
  ImGui::Text("Time Stretching");
  ImGui::SameLine(200);
  ImGui::Checkbox("##audio_stretch", &s_settings.audio_stretching);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.7f, 0.9f, 1.0f));
  ImGui::TextWrapped("Audio stretching reduces crackling when the emulator "
                     "runs slower than full speed, but adds slight latency.");
  ImGui::PopStyleColor();
}

// ─── System Tab ──────────────────────────────────────────────
void SettingsUI::RenderSystemTab() {
  ImGui::Spacing();

  // System Language
  const char* languages[] = {
    "Japanese", "English (US)", "French", "Spanish", "German",
    "Italian", "Dutch", "Portuguese (PT)", "Russian", "Korean",
    "Chinese (Traditional)", "Chinese (Simplified)", "Finnish",
    "Swedish", "Danish", "Norwegian", "Polish", "Portuguese (BR)",
    "English (UK)", "Turkish", "Arabic"
  };
  ImGui::Text("System Language");
  ImGui::SameLine(200);
  ImGui::SetNextItemWidth(250);
  ImGui::Combo("##sys_lang", &s_settings.system_language, languages, 21);

  ImGui::Spacing();

  // Username
  static char username_buf[64] = {};
  if (username_buf[0] == '\0') {
    strncpy(username_buf, s_settings.username.c_str(), sizeof(username_buf) - 1);
  }
  ImGui::Text("Username");
  ImGui::SameLine(200);
  ImGui::SetNextItemWidth(250);
  if (ImGui::InputText("##username", username_buf, sizeof(username_buf))) {
    s_settings.username = username_buf;
  }

  ImGui::Spacing();

  // Show Splash
  ImGui::Text("Show Boot Splash");
  ImGui::SameLine(200);
  ImGui::Checkbox("##splash", &s_settings.show_splash);

  ImGui::Spacing();

  // Show FPS
  ImGui::Text("Show FPS Counter");
  ImGui::SameLine(200);
  ImGui::Checkbox("##show_fps", &s_settings.show_fps);

  ImGui::Spacing();

  // Log to File
  ImGui::Text("Log to File");
  ImGui::SameLine(200);
  ImGui::Checkbox("##log_file", &s_settings.log_to_file);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.7f, 0.9f, 1.0f));
  ImGui::TextWrapped("System settings emulate the PS4's own system configuration. "
                     "Games use these values for language selection and user identity.");
  ImGui::PopStyleColor();
}

// ─── Paths Tab ───────────────────────────────────────────────
void SettingsUI::RenderPathsTab() {
  ImGui::Spacing();

  // Games Directory
  ImGui::Text("Games Directory");
  ImGui::Spacing();

  ImGui::SetNextItemWidth(480);
  char games_buf[512] = {};
  strncpy(games_buf, s_settings.games_directory.c_str(), sizeof(games_buf) - 1);
  ImGui::InputText("##games_path", games_buf, sizeof(games_buf),
                    ImGuiInputTextFlags_ReadOnly);
  ImGui::SameLine();
  if (ImGui::Button("Browse##games", ImVec2(90, 0))) {
    std::string path = BrowseForFolder("Select Games Directory");
    if (!path.empty()) {
      s_settings.games_directory = path;
      printf("[Settings] Games directory set to: %s\n", path.c_str());
    }
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // Firmware Directory
  ImGui::Text("Firmware Modules Directory");
  ImGui::Spacing();

  ImGui::SetNextItemWidth(480);
  char fw_buf[512] = {};
  strncpy(fw_buf, s_settings.firmware_directory.c_str(), sizeof(fw_buf) - 1);
  ImGui::InputText("##fw_path", fw_buf, sizeof(fw_buf),
                    ImGuiInputTextFlags_ReadOnly);
  ImGui::SameLine();
  if (ImGui::Button("Browse##firmware", ImVec2(90, 0))) {
    std::string path = BrowseForFolder("Select Firmware Modules Directory");
    if (!path.empty()) {
      s_settings.firmware_directory = path;
      printf("[Settings] Firmware directory set to: %s\n", path.c_str());
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Directory status indicators
  auto StatusIcon = [](const std::string& path, const char* label) {
    bool valid = !path.empty() && std::filesystem::exists(path) &&
                 std::filesystem::is_directory(path);
    ImVec4 color = valid ? ImVec4(0.2f, 0.8f, 0.3f, 1.0f)
                         : ImVec4(0.8f, 0.3f, 0.2f, 1.0f);
    const char* icon = valid ? "[OK]" : "[NOT SET]";

    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%s %s", icon, label);
    ImGui::PopStyleColor();
  };

  StatusIcon(s_settings.games_directory, "Games Directory");
  StatusIcon(s_settings.firmware_directory, "Firmware Directory");

  ImGui::Spacing();
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.7f, 0.9f, 1.0f));
  ImGui::TextWrapped("Point the Games Directory to a folder containing extracted "
                     "PS4 game folders (each with eboot.bin). The Firmware Directory "
                     "should contain PS4 system modules (.sprx files).");
  ImGui::PopStyleColor();
}

// ─── Save / Load ─────────────────────────────────────────────
void SettingsUI::SaveSettings() {
  std::string path = GetSettingsPath();
  std::ofstream f(path);
  if (!f.is_open()) {
    fprintf(stderr, "[Settings] Failed to save to: %s\n", path.c_str());
    return;
  }

  f << "[paths]\n";
  f << "games_directory=" << s_settings.games_directory << "\n";
  f << "firmware_directory=" << s_settings.firmware_directory << "\n";
  f << "\n[graphics]\n";
  f << "gpu_backend=" << s_settings.gpu_backend << "\n";
  f << "resolution_scale=" << s_settings.resolution_scale << "\n";
  f << "vsync=" << (s_settings.vsync ? 1 : 0) << "\n";
  f << "anisotropic_filter=" << s_settings.anisotropic_filter << "\n";
  f << "fullscreen=" << (s_settings.fullscreen ? 1 : 0) << "\n";
  f << "\n[audio]\n";
  f << "audio_backend=" << s_settings.audio_backend << "\n";
  f << "volume=" << s_settings.volume << "\n";
  f << "audio_stretching=" << (s_settings.audio_stretching ? 1 : 0) << "\n";
  f << "\n[system]\n";
  f << "system_language=" << s_settings.system_language << "\n";
  f << "show_splash=" << (s_settings.show_splash ? 1 : 0) << "\n";
  f << "show_fps=" << (s_settings.show_fps ? 1 : 0) << "\n";
  f << "log_to_file=" << (s_settings.log_to_file ? 1 : 0) << "\n";
  f << "username=" << s_settings.username << "\n";

  f.close();
  printf("[Settings] Saved to: %s\n", path.c_str());
}

void SettingsUI::LoadSettings() {
  std::string path = GetSettingsPath();
  std::ifstream f(path);
  if (!f.is_open()) {
    printf("[Settings] No settings file found, using defaults\n");
    return;
  }

  std::string line;
  while (std::getline(f, line)) {
    // Skip section headers and empty lines
    if (line.empty() || line[0] == '[' || line[0] == '#') continue;

    auto eq = line.find('=');
    if (eq == std::string::npos) continue;

    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);

    // Paths
    if (key == "games_directory")    s_settings.games_directory = val;
    if (key == "firmware_directory") s_settings.firmware_directory = val;
    // Graphics
    if (key == "gpu_backend")        s_settings.gpu_backend = std::stoi(val);
    if (key == "resolution_scale")   s_settings.resolution_scale = std::stoi(val);
    if (key == "vsync")              s_settings.vsync = (val == "1");
    if (key == "anisotropic_filter") s_settings.anisotropic_filter = std::stoi(val);
    if (key == "fullscreen")         s_settings.fullscreen = (val == "1");
    // Audio
    if (key == "audio_backend")      s_settings.audio_backend = std::stoi(val);
    if (key == "volume")             s_settings.volume = std::stoi(val);
    if (key == "audio_stretching")   s_settings.audio_stretching = (val == "1");
    // System
    if (key == "system_language")    s_settings.system_language = std::stoi(val);
    if (key == "show_splash")        s_settings.show_splash = (val == "1");
    if (key == "show_fps")           s_settings.show_fps = (val == "1");
    if (key == "log_to_file")        s_settings.log_to_file = (val == "1");
    if (key == "username")           s_settings.username = val;
  }

  f.close();
  printf("[Settings] Loaded from: %s\n", path.c_str());
  printf("[Settings]   Games: %s\n", s_settings.games_directory.c_str());
  printf("[Settings]   Firmware: %s\n", s_settings.firmware_directory.c_str());
}

} // namespace Gui
