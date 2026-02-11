// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "imgui.h"
#include <mutex>
#include <string>
#include <vector>

namespace Gui {

struct LogEntry {
  int id;
  std::string message;
  ImVec4 color;
};

class GuiDebugger {
public:
  static GuiDebugger &GetInstance();

  void AddLog(const std::string &message,
              const ImVec4 &color = ImVec4(1, 1, 1, 1));
  void Render();

  bool IsVisible() const { return visible; }
  void SetVisible(bool v) { visible = v; }

private:
  GuiDebugger() = default;
  ~GuiDebugger() = default;

  std::vector<LogEntry> logs;
  std::mutex logMutex;
  int nextLogId = 0;
  bool visible = true;
  bool scrollToBottom = true;

  void RenderKernelConsole();
  void RenderMemoryViewer();
  void RenderThreadInspector();
  void RenderModuleInspector();
};

} // namespace Gui
