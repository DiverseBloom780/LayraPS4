// gui/settings_ui.cpp
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "settings_ui.h"
#include "../layra_pkg.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>

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
static std::string s_status_message;
static std::thread s_pkg_install_thread;
static std::mutex s_pkg_install_mutex;
static std::atomic<bool> s_pkg_install_running{false};
static std::atomic<float> s_pkg_install_progress{0.0f};
static std::atomic<int> s_pkg_install_current{0};
static std::atomic<int> s_pkg_install_total{0};
static std::string s_pkg_install_status;
static std::string s_pkg_install_file;

static std::string GetSettingsPath() { return "layra_settings.ini"; }

static void DrawSectionTitle(const char *title) {
  ImGui::Spacing();
  ImGui::TextColored(ImVec4(0.9f, 0.95f, 1.0f, 1.0f), "%s", title);
  ImGui::Separator();
  ImGui::Spacing();
}

std::string SettingsUI::BrowseForFolder(const char *title) {
#ifdef _WIN32
  std::string result;
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  bool needUninit = SUCCEEDED(hr);

  IFileDialog *pfd = nullptr;
  hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                        IID_IFileDialog, reinterpret_cast<void **>(&pfd));
  if (SUCCEEDED(hr)) {
    DWORD dwOptions;
    pfd->GetOptions(&dwOptions);
    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

    wchar_t wTitle[256];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wTitle, 256);
    pfd->SetTitle(wTitle);

    hr = pfd->Show(nullptr);
    if (SUCCEEDED(hr)) {
      IShellItem *psi = nullptr;
      hr = pfd->GetResult(&psi);
      if (SUCCEEDED(hr)) {
        PWSTR pszPath = nullptr;
        hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
        if (SUCCEEDED(hr)) {
          int len = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, nullptr, 0,
                                        nullptr, nullptr);
          result.resize(len - 1);
          WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, result.data(), len,
                              nullptr, nullptr);
          CoTaskMemFree(pszPath);
        }
        psi->Release();
      }
    }
    pfd->Release();
  }

  if (needUninit) {
    CoUninitialize();
  }
  return result;
#else
  return "";
#endif
}

static std::string BrowseForPkgFile(const char *title) {
#ifdef _WIN32
  std::string result;
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  bool needUninit = SUCCEEDED(hr);

  IFileOpenDialog *pfd = nullptr;
  hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                        IID_IFileOpenDialog, reinterpret_cast<void **>(&pfd));
  if (SUCCEEDED(hr)) {
    COMDLG_FILTERSPEC filters[] = {{L"PlayStation PKG files", L"*.pkg;*.PKG"},
                                   {L"All files", L"*.*"}};
    pfd->SetFileTypes(2, filters);

    wchar_t wTitle[256];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wTitle, 256);
    pfd->SetTitle(wTitle);

    hr = pfd->Show(nullptr);
    if (SUCCEEDED(hr)) {
      IShellItem *psi = nullptr;
      hr = pfd->GetResult(&psi);
      if (SUCCEEDED(hr)) {
        PWSTR pszPath = nullptr;
        hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
        if (SUCCEEDED(hr)) {
          int len = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, nullptr, 0,
                                        nullptr, nullptr);
          result.resize(len - 1);
          WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, result.data(), len,
                              nullptr, nullptr);
          CoTaskMemFree(pszPath);
        }
        psi->Release();
      }
    }
    pfd->Release();
  }

  if (needUninit) {
    CoUninitialize();
  }
  return result;
#else
  return "";
#endif
}

static void OnPkgInstallProgress(int current, int total, const char *filename,
                                 void *userdata) {
  (void)userdata;
  std::lock_guard<std::mutex> lock(s_pkg_install_mutex);
  s_pkg_install_current.store(current);
  s_pkg_install_total.store(total);
  if (total > 0) {
    s_pkg_install_progress.store(static_cast<float>(current) / static_cast<float>(total));
  }
  if (filename) {
    s_pkg_install_file = filename;
  }
  if (total > 0) {
    s_pkg_install_status = "Extracting package contents...";
  }
}

static void InstallPkgWorker(const std::string &pkg_path,
                             const std::filesystem::path &package_dir) {
  {
    std::lock_guard<std::mutex> lock(s_pkg_install_mutex);
    s_pkg_install_running.store(true);
    s_pkg_install_progress.store(0.0f);
    s_pkg_install_current.store(0);
    s_pkg_install_total.store(0);
    s_pkg_install_status = "Preparing extraction...";
    s_pkg_install_file.clear();
  }

  bool ok = layra_pkg_extract_to_directory(pkg_path.c_str(),
                                            package_dir.string().c_str(),
                                            OnPkgInstallProgress, nullptr);

  {
    std::lock_guard<std::mutex> lock(s_pkg_install_mutex);
    s_pkg_install_running.store(false);
    s_pkg_install_progress.store(ok ? 1.0f : 0.0f);
    s_pkg_install_status = ok ? "PKG installation complete." : "PKG installation failed.";
  }
}

void SettingsUI::Open() {
  s_open = true;
  s_activeTab = 0;
  s_status_message.clear();
  printf("[Settings] Settings dialog opened\n");
}

void SettingsUI::Close() {
  s_open = false;
  printf("[Settings] Settings dialog closed\n");
}

bool SettingsUI::IsOpen() { return s_open; }
const LayraSettings &SettingsUI::GetSettings() { return s_settings; }
LayraSettings &SettingsUI::GetMutableSettings() { return s_settings; }

void SettingsUI::Render() {
  if (!s_open) {
    return;
  }

  ImGuiIO &io = ImGui::GetIO();
  ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
  ImVec2 dialogSize(960.0f, 640.0f);

  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(dialogSize, ImGuiCond_Appearing);

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.06f, 0.11f, 0.97f));
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.16f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.16f, 0.24f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.24f, 0.44f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.32f, 0.56f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.38f, 0.66f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 18));

  bool still_open = true;
  if (ImGui::Begin("Settings##LayraSettings", &still_open,
                   ImGuiWindowFlags_NoCollapse)) {
    ImGui::TextColored(ImVec4(0.75f, 0.88f, 1.0f, 1.0f), "System Settings");
    ImGui::SameLine();
    ImGui::TextDisabled("PS4-style configuration");
    ImGui::Separator();

    ImGui::BeginChild("Sidebar", ImVec2(180.0f, 0.0f), true);
    const char *tabs[] = {"Overview", "Graphics", "Audio", "System", "Paths"};
    for (int i = 0; i < 5; ++i) {
      bool selected = (s_activeTab == i);
      ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImVec4(0.18f, 0.30f, 0.56f, 1.0f)
                                                     : ImVec4(0.10f, 0.13f, 0.20f, 1.0f));
      if (ImGui::Button(tabs[i], ImVec2(-1, 36))) {
        s_activeTab = i;
      }
      ImGui::PopStyleColor();
      ImGui::Spacing();
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("Content", ImVec2(0.0f, -56.0f), true);
    switch (s_activeTab) {
    case 0:
      RenderOverviewTab();
      break;
    case 1:
      RenderGraphicsTab();
      break;
    case 2:
      RenderAudioTab();
      break;
    case 3:
      RenderSystemTab();
      break;
    case 4:
      RenderPathsTab();
      break;
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 360.0f) * 0.5f);
    if (ImGui::Button("Save", ImVec2(100, 34))) {
      SaveSettings();
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply", ImVec2(100, 34))) {
      SaveSettings();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(100, 34))) {
      s_open = false;
    }
  }
  ImGui::End();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(6);

  if (!still_open) {
    s_open = false;
  }
}

void SettingsUI::RenderOverviewTab() {
  DrawSectionTitle("System Summary");
  ImGui::BeginChild("OverviewCards", ImVec2(0, 180), true);
  ImGui::Columns(2, "OverviewCols", false);
  ImGui::Text("User Profile");
  ImGui::NextColumn();
  ImGui::Text("%s", s_settings.username.c_str());
  ImGui::NextColumn();
  ImGui::Text("Games Folder");
  ImGui::NextColumn();
  ImGui::TextWrapped("%s", s_settings.games_directory.empty() ? "Not set" : s_settings.games_directory.c_str());
  ImGui::Columns(1);
  ImGui::EndChild();

  DrawSectionTitle("Quick Actions");
  if (ImGui::Button("Scan Games Directory", ImVec2(220, 0))) {
    printf("[Settings] Re-scan requested\n");
  }
  ImGui::SameLine();
  if (ImGui::Button("Open Firmware Folder", ImVec2(220, 0))) {
    std::string path = BrowseForFolder("Select Firmware Folder");
    if (!path.empty()) {
      s_settings.firmware_directory = path;
    }
  }
  ImGui::Spacing();
  if (ImGui::Button("Install PKG", ImVec2(220, 0)) && !s_pkg_install_running.load()) {
    std::string pkg_path = BrowseForPkgFile("Select PKG to install");
    if (!pkg_path.empty()) {
      std::filesystem::path install_root = s_settings.games_directory.empty()
                                               ? std::filesystem::current_path() / "games"
                                               : std::filesystem::path(s_settings.games_directory);
      std::filesystem::path package_dir = install_root / std::filesystem::path(pkg_path).stem().string();
      try {
        std::filesystem::create_directories(package_dir);
        std::filesystem::copy_file(pkg_path, package_dir / std::filesystem::path(pkg_path).filename().string(),
                                   std::filesystem::copy_options::overwrite_existing);
      } catch (const std::filesystem::filesystem_error &e) {
        s_status_message = std::string("PKG install failed: ") + e.what();
        printf("[Settings] PKG install failed: %s\n", e.what());
      }

      if (!s_status_message.empty() && s_status_message.find("PKG install failed") != std::string::npos) {
        return;
      }

      s_status_message.clear();
      if (s_pkg_install_thread.joinable()) {
        s_pkg_install_thread.join();
      }
      s_pkg_install_thread = std::thread(InstallPkgWorker, pkg_path, package_dir);
    }
  }

  if (s_pkg_install_running.load()) {
    ImGui::Spacing();
    ImGui::TextWrapped("%s", [&]() {
      std::lock_guard<std::mutex> lock(s_pkg_install_mutex);
      return s_pkg_install_status;
    }().c_str());

    float progress = s_pkg_install_progress.load();
    int current = s_pkg_install_current.load();
    int total = s_pkg_install_total.load();
    std::string file_name;
    {
      std::lock_guard<std::mutex> lock(s_pkg_install_mutex);
      file_name = s_pkg_install_file;
    }

    ImGui::ProgressBar(progress, ImVec2(-1, 0));
    if (total > 0) {
      ImGui::Text("%d / %d files", current, total);
    }
    if (!file_name.empty()) {
      ImGui::TextDisabled("%s", file_name.c_str());
    }
  }

  if (!s_status_message.empty()) {
    ImGui::Spacing();
    ImGui::TextWrapped("%s", s_status_message.c_str());
  }
}

void SettingsUI::RenderGraphicsTab() {
  DrawSectionTitle("Rendering");
  const char *backends[] = {"Vulkan"};
  ImGui::Text("GPU Backend");
  ImGui::SameLine(220);
  ImGui::SetNextItemWidth(240);
  ImGui::Combo("##gpu_backend", &s_settings.gpu_backend, backends, 1);

  ImGui::Text("Resolution Scale");
  ImGui::SameLine(220);
  ImGui::SetNextItemWidth(240);
  ImGui::SliderInt("##res_scale", &s_settings.resolution_scale, 50, 300, "%d%%");

  ImGui::Text("VSync");
  ImGui::SameLine(220);
  ImGui::Checkbox("##vsync", &s_settings.vsync);

  const char *aniso_levels[] = {"1x", "2x", "4x", "8x", "16x"};
  int aniso_idx = 0;
  switch (s_settings.anisotropic_filter) {
  case 1: aniso_idx = 0; break;
  case 2: aniso_idx = 1; break;
  case 4: aniso_idx = 2; break;
  case 8: aniso_idx = 3; break;
  case 16: aniso_idx = 4; break;
  }
  ImGui::Text("Anisotropic Filter");
  ImGui::SameLine(220);
  ImGui::SetNextItemWidth(240);
  if (ImGui::Combo("##aniso", &aniso_idx, aniso_levels, 5)) {
    int vals[] = {1, 2, 4, 8, 16};
    s_settings.anisotropic_filter = vals[aniso_idx];
  }

  ImGui::Text("Fullscreen");
  ImGui::SameLine(220);
  ImGui::Checkbox("##fullscreen", &s_settings.fullscreen);
}

void SettingsUI::RenderAudioTab() {
  DrawSectionTitle("Audio");
  const char *audio_backends[] = {"SDL3 Audio", "XAudio2 (Windows)"};
  ImGui::Text("Audio Backend");
  ImGui::SameLine(220);
  ImGui::SetNextItemWidth(240);
  ImGui::Combo("##audio_backend", &s_settings.audio_backend, audio_backends, 2);

  ImGui::Text("Master Volume");
  ImGui::SameLine(220);
  ImGui::SetNextItemWidth(240);
  ImGui::SliderInt("##volume", &s_settings.volume, 0, 100, "%d%%");

  ImGui::Text("Time Stretching");
  ImGui::SameLine(220);
  ImGui::Checkbox("##audio_stretch", &s_settings.audio_stretching);
}

void SettingsUI::RenderSystemTab() {
  DrawSectionTitle("System");
  const char *languages[] = {"Japanese", "English (US)", "French", "Spanish", "German",
                              "Italian", "Dutch", "Portuguese (PT)", "Russian", "Korean",
                              "Chinese (Traditional)", "Chinese (Simplified)", "Finnish",
                              "Swedish", "Danish", "Norwegian", "Polish", "Portuguese (BR)",
                              "English (UK)", "Turkish", "Arabic"};
  ImGui::Text("System Language");
  ImGui::SameLine(220);
  ImGui::SetNextItemWidth(240);
  ImGui::Combo("##sys_lang", &s_settings.system_language, languages, 21);

  static char username_buf[64] = {};
  if (username_buf[0] == '\0') {
    strncpy(username_buf, s_settings.username.c_str(), sizeof(username_buf) - 1);
  }
  ImGui::Text("Username");
  ImGui::SameLine(220);
  ImGui::SetNextItemWidth(240);
  if (ImGui::InputText("##username", username_buf, sizeof(username_buf))) {
    s_settings.username = username_buf;
  }

  ImGui::Text("Show Boot Splash");
  ImGui::SameLine(220);
  ImGui::Checkbox("##splash", &s_settings.show_splash);

  ImGui::Text("Show FPS Counter");
  ImGui::SameLine(220);
  ImGui::Checkbox("##show_fps", &s_settings.show_fps);

  ImGui::Text("Log to File");
  ImGui::SameLine(220);
  ImGui::Checkbox("##log_file", &s_settings.log_to_file);
}

void SettingsUI::RenderPathsTab() {
  DrawSectionTitle("Directory Paths");
  ImGui::Text("Games Directory");
  ImGui::InputText("##games_path", (char *)"", 0, ImGuiInputTextFlags_ReadOnly);
  ImGui::SameLine();
  ImGui::TextWrapped("%s", s_settings.games_directory.empty() ? "Not configured" : s_settings.games_directory.c_str());
  if (ImGui::Button("Browse Games", ImVec2(180, 0))) {
    std::string path = BrowseForFolder("Select Games Directory");
    if (!path.empty()) {
      s_settings.games_directory = path;
    }
  }

  ImGui::Spacing();
  ImGui::Text("Firmware Modules Directory");
  ImGui::TextWrapped("%s", s_settings.firmware_directory.empty() ? "Not configured" : s_settings.firmware_directory.c_str());
  if (ImGui::Button("Browse Firmware", ImVec2(180, 0))) {
    std::string path = BrowseForFolder("Select Firmware Modules Directory");
    if (!path.empty()) {
      s_settings.firmware_directory = path;
    }
  }
}

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
    if (line.empty() || line[0] == '[' || line[0] == '#') {
      continue;
    }

    auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);

    if (key == "games_directory") s_settings.games_directory = val;
    if (key == "firmware_directory") s_settings.firmware_directory = val;
    if (key == "gpu_backend") s_settings.gpu_backend = std::stoi(val);
    if (key == "resolution_scale") s_settings.resolution_scale = std::stoi(val);
    if (key == "vsync") s_settings.vsync = (val == "1");
    if (key == "anisotropic_filter") s_settings.anisotropic_filter = std::stoi(val);
    if (key == "fullscreen") s_settings.fullscreen = (val == "1");
    if (key == "audio_backend") s_settings.audio_backend = std::stoi(val);
    if (key == "volume") s_settings.volume = std::stoi(val);
    if (key == "audio_stretching") s_settings.audio_stretching = (val == "1");
    if (key == "system_language") s_settings.system_language = std::stoi(val);
    if (key == "show_splash") s_settings.show_splash = (val == "1");
    if (key == "show_fps") s_settings.show_fps = (val == "1");
    if (key == "log_to_file") s_settings.log_to_file = (val == "1");
    if (key == "username") s_settings.username = val;
  }

  f.close();
  printf("[Settings] Loaded from: %s\n", path.c_str());
}

} // namespace Gui
