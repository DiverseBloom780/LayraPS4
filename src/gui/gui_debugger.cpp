#include "gui_debugger.h"
#include "core/kernel/kernel_manager.h"
#include "core/kernel/module_manager.h"
#include "core/memory/memory_manager.h"
#include "emulator.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

extern std::unique_ptr<Core::Emulator> g_emulator_instance;

namespace Gui {

GuiDebugger &GuiDebugger::GetInstance() {
  static GuiDebugger instance;
  return instance;
}

void GuiDebugger::AddLog(const std::string &message, const ImVec4 &color) {
  std::lock_guard<std::mutex> lock(logMutex);
  logs.push_back({nextLogId++, message, color});
  if (logs.size() > 1000) {
    logs.erase(logs.begin());
  }
}

void GuiDebugger::Render() {
  if (!visible)
    return;

  if (ImGui::Begin("LayraPS4 Debug Suite", &visible)) {
    if (ImGui::BeginTabBar("DebuggerTabs")) {
      if (ImGui::BeginTabItem("Kernel Console")) {
        RenderKernelConsole();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Thread Inspector")) {
        RenderThreadInspector();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Memory Viewer")) {
        RenderMemoryViewer();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Module Inspector")) {
        RenderModuleInspector();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }
  ImGui::End();
}

void GuiDebugger::RenderKernelConsole() {
  if (ImGui::Button("Clear")) {
    std::lock_guard<std::mutex> lock(logMutex);
    logs.clear();
  }
  ImGui::SameLine();
  ImGui::Checkbox("Auto-scroll", &scrollToBottom);

  ImGui::Separator();

  const float footer_height_to_reserve =
      ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve),
                    false, ImGuiWindowFlags_HorizontalScrollbar);

  {
    std::lock_guard<std::mutex> lock(logMutex);
    for (const auto &log : logs) {
      ImGui::PushStyleColor(ImGuiCol_Text, log.color);
      ImGui::TextUnformatted(log.message.c_str());
      ImGui::PopStyleColor();
    }
  }

  if (scrollToBottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    ImGui::SetScrollHereY(1.0f);

  ImGui::EndChild();
}

void GuiDebugger::RenderThreadInspector() {
  ImGui::Text("Emulated Threads:");
  ImGui::Separator();

  if (!g_emulator_instance) {
    ImGui::Text("Emulator not initialized.");
    return;
  }

  auto kernel = g_emulator_instance->GetKernelManager();
  if (!kernel) {
    ImGui::Text("Kernel Manager not available.");
    return;
  }

  auto threads = kernel->GetThreadList();
  if (threads.empty()) {
    ImGui::Text("No active threads.");
    return;
  }

  if (ImGui::BeginTable("ThreadTable", 5,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
    ImGui::TableSetupColumn("Handle");
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Entry");
    ImGui::TableSetupColumn("State");
    ImGui::TableSetupColumn("Status");
    ImGui::TableHeadersRow();

    for (const auto &thread : threads) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("0x%X", thread.handle);
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%s", thread.name.c_str());
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("0x%llX", thread.entry);
      ImGui::TableSetColumnIndex(3);
      if (thread.running) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Running");
      } else if (thread.exited) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Exited");
      } else {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Ready");
      }
      ImGui::TableSetColumnIndex(4);
      ImGui::Text("-");
    }
    ImGui::EndTable();
  }
}

void GuiDebugger::RenderMemoryViewer() {
  static char addr_buf[32] = "0x400000";
  ImGui::InputText("Address", addr_buf, sizeof(addr_buf));
  ImGui::Separator();

  if (!g_emulator_instance) {
    ImGui::Text("Emulator not initialized.");
    return;
  }

  auto mm = g_emulator_instance->GetMemoryManager();
  if (!mm) {
    ImGui::Text("Memory Manager not available.");
    return;
  }

  uint64_t vaddr = 0;
  try {
    vaddr = std::stoull(addr_buf, nullptr, 16);
  } catch (...) {
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid Address Format");
    return;
  }

  const int bytes_to_show = 256;
  const int bytes_per_row = 16;
  uint8_t buffer[bytes_to_show];

  // Attempt to read memory. MemoryManager::Read should handle safety.
  mm->Read(vaddr, buffer, bytes_to_show);

  if (ImGui::BeginChild("MemScrollingRegion")) {
    for (int i = 0; i < bytes_to_show; i += bytes_per_row) {
      ImGui::Text("%016llX: ", vaddr + i);
      ImGui::SameLine();

      // Hex part
      std::stringstream hex_ss;
      for (int j = 0; j < bytes_per_row; ++j) {
        hex_ss << std::hex << std::setw(2) << std::setfill('0')
               << (int)buffer[i + j] << " ";
      }
      ImGui::Text("%s", hex_ss.str().c_str());
      ImGui::SameLine();

      // ASCII part
      std::string ascii = "| ";
      for (int j = 0; j < bytes_per_row; ++j) {
        char c = (char)buffer[i + j];
        ascii += (c >= 32 && c <= 126) ? c : '.';
      }
      ascii += " |";
      ImGui::Text("%s", ascii.c_str());
    }
  }
  ImGui::EndChild();
}

void GuiDebugger::RenderModuleInspector() {
  if (!g_emulator_instance)
    return;

  auto mm = g_emulator_instance->GetModuleManager();
  if (!mm)
    return;

  auto modules = mm->GetLoadedModules();
  if (modules.empty()) {
    ImGui::Text("No modules loaded.");
    return;
  }

  if (ImGui::BeginTable("ModuleTable", 4,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_Resizable)) {
    ImGui::TableSetupColumn("Handle");
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Base Address");
    ImGui::TableSetupColumn("Exports");
    ImGui::TableHeadersRow();

    for (const auto &mod : modules) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("0x%X", mod.handle);
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%s", mod.name.c_str());
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("0x%llX", mod.baseAddress);
      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%zu", mod.exports.size());

      if (ImGui::IsItemHovered() && !mod.exports.empty()) {
        ImGui::BeginTooltip();
        ImGui::Text("Exported Symbols:");
        for (const auto &[name, sym] : mod.exports) {
          ImGui::Text("  %s @ 0x%llX", name.c_str(), sym.address);
        }
        ImGui::EndTooltip();
      }
    }
    ImGui::EndTable();
  }
}

} // namespace Gui
