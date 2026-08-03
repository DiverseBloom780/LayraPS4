// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// CRITICAL: imgui.h MUST be included FIRST!
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui.h"                      // ← FIRST!
#include "../lib/imgui/backends/imgui_impl_sdl3.h"   // ← Now this works
#include "../lib/imgui/backends/imgui_impl_vulkan.h" // ← Now this works

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <windows.h>


#include "emulator.h"
#include "gui/ps4_ui.h"
#include "layra_vulkan.h"
#include "core/libraries/gnmdriver/gnmdriver.h"

// Global instance
std::unique_ptr<Core::Emulator> g_emulator_instance;

// Static globals
static VkDescriptorPool gDescriptorPool = VK_NULL_HANDLE;
static FILE *g_log_file = nullptr;

static std::filesystem::path GetLogFilePath() {
  char module_path[MAX_PATH] = {};
  if (GetModuleFileNameA(NULL, module_path, MAX_PATH) > 0) {
    std::filesystem::path exe_path(module_path);
    return exe_path.remove_filename() / "LayraPS4.log";
  }
  return std::filesystem::current_path() / "LayraPS4.log";
}

static bool InitializeLogFile() {
  auto log_path = GetLogFilePath();
  if (!log_path.has_parent_path())
    return false;

  std::filesystem::create_directories(log_path.parent_path());
  // Open a dedicated log file instead of redirecting stdout/stderr. Redirecting
  // CRT std streams with freopen can trigger debug assertions in some
  // environments; opening a separate FILE* is safer for crash logging.
  FILE *log_fp = nullptr;
  errno_t ferr = fopen_s(&log_fp, log_path.string().c_str(), "w+");
  if (ferr != 0 || !log_fp) {
    // Try fallback to current working directory if module path is inaccessible.
    try {
      auto fallback = std::filesystem::current_path() / "LayraPS4.log";
      errno_t ferr2 = fopen_s(&log_fp, fallback.string().c_str(), "w+");
      if (ferr2 != 0 || !log_fp) {
        // Show an error message so users running the exe by double-click see the problem.
        char msg[1024];
        snprintf(msg, sizeof(msg), "Failed to open log file:\nPrimary: %s\nFallback: %s\n\nerrno: %d",
                 log_path.string().c_str(), fallback.string().c_str(), (int)ferr2);
        MessageBoxA(NULL, msg, "LayraPS4 - Log Error", MB_OK | MB_ICONERROR);
        return false;
      }
    } catch (...) {
      MessageBoxA(NULL, "Failed to open log file and fallback path.", "LayraPS4 - Log Error", MB_OK | MB_ICONERROR);
      return false;
    }
  }

  g_log_file = log_fp;
  setvbuf(g_log_file, NULL, _IONBF, 0);

  fprintf(g_log_file, "=== LayraPS4 log started ===\n");
  fflush(g_log_file);
  return true;
}

static void CloseLogFile() {
  if (g_log_file) {
    fflush(g_log_file);
    if (g_log_file != stdout && g_log_file != stderr) {
      fclose(g_log_file);
    }
    g_log_file = nullptr;
  }
}

static int FatalError(const char *message) {
  if (g_log_file) {
    fprintf(g_log_file, "FATAL: %s\n", message);
    fflush(g_log_file);
  }

  char formatted[1536];
  if (g_log_file) {
    snprintf(formatted, sizeof(formatted), "%s\n\nSee LayraPS4.log for details.", message);
  } else {
    snprintf(formatted, sizeof(formatted), "%s", message);
  }

  fprintf(stderr, "FATAL: %s\n", formatted);
  MessageBoxA(NULL, formatted, "LayraPS4 - Fatal Error", MB_OK | MB_ICONERROR);
  return -1;
}

static LONG WINAPI VectoredExceptionHandler(EXCEPTION_POINTERS *exception_info) {
  if (exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) return EXCEPTION_CONTINUE_SEARCH;
  
  DWORD code = exception_info->ExceptionRecord->ExceptionCode;
  void *addr = exception_info->ExceptionRecord->ExceptionAddress;
  auto *context = exception_info->ContextRecord;

  auto print_crash = [&](FILE *out) {
    if (!out) return;
    fprintf(out, "\n[Crash] Vectored exception code 0x%08X at address %p\n", code, addr);
    if (context) {
      fprintf(out, "[Crash] Registers:\n");
      fprintf(out, "  RAX: 0x%016llX  RBX: 0x%016llX  RCX: 0x%016llX\n", context->Rax, context->Rbx, context->Rcx);
      fprintf(out, "  RDX: 0x%016llX  RSI: 0x%016llX  RDI: 0x%016llX\n", context->Rdx, context->Rsi, context->Rdi);
      fprintf(out, "  RBP: 0x%016llX  RSP: 0x%016llX  RIP: 0x%016llX\n", context->Rbp, context->Rsp, context->Rip);
      fprintf(out, "  R8 : 0x%016llX  R9 : 0x%016llX  R10: 0x%016llX\n", context->R8, context->R9, context->R10);
      fprintf(out, "  R11: 0x%016llX  R12: 0x%016llX  R13: 0x%016llX\n", context->R11, context->R12, context->R13);
      fprintf(out, "  R14: 0x%016llX  R15: 0x%016llX\n", context->R14, context->R15);
      
      // Print first 8 quadwords from the stack
      uint64_t *sp = reinterpret_cast<uint64_t *>(context->Rsp);
      if (sp) {
        fprintf(out, "[Crash] Stack (RSP):\n");
        for (int i = 0; i < 8; ++i) {
          __try {
            fprintf(out, "  +0x%02X: 0x%016llX\n", i * 8, sp[i]);
          } __except (EXCEPTION_EXECUTE_HANDLER) {
            fprintf(out, "  +0x%02X: [Invalid memory]\n", i * 8);
            break;
          }
        }
      }

      // Print instruction bytes at RIP
      uint8_t *ip = reinterpret_cast<uint8_t *>(context->Rip);
      if (ip) {
        fprintf(out, "[Crash] Instruction at RIP:\n  ");
        __try {
          for (int i = 0; i < 16; ++i) {
            fprintf(out, "%02X ", ip[i]);
          }
          fprintf(out, "\n");
        } __except (EXCEPTION_EXECUTE_HANDLER) {
          fprintf(out, " [Invalid memory]\n");
        }
      }
    }
    fflush(out);
  };

  print_crash(g_log_file);
  print_crash(stderr);

  return EXCEPTION_CONTINUE_SEARCH; // Let it crash
}

static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS *exception_info) {
  if (g_log_file) {
    fprintf(g_log_file,
            "[Crash] Unhandled exception code 0x%08X at address %p\n",
            exception_info->ExceptionRecord->ExceptionCode,
            exception_info->ExceptionRecord->ExceptionAddress);
    fflush(g_log_file);
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

static void TerminateHandler() {
  if (g_log_file) {
    fprintf(g_log_file, "[Crash] std::terminate called\n");
    fflush(g_log_file);
  }
  std::abort();
}

// Forward declarations
void ImGui_RenderCallback(VkCommandBuffer cmd);
void RenderPS4BootSequence(ImGuiIO &io);

// Implementation of render callback
void ImGui_RenderCallback(VkCommandBuffer cmd) {
  // Render emulator output first (behind ImGui overlay)
  if (g_emulator_instance && g_emulator_instance->IsRunning()) {
    Core::Libraries::GnmDriver::RenderEmulatorFrame(cmd);
  }
  // Then render ImGui UI on top
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

// PS4 Boot Sequence Rendering
void RenderPS4BootSequence(ImGuiIO &io) {
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::Begin("##BootSequence", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

  // Center the boot logo/text
  ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);

  // Draw PlayStation logo text (simplified)
  const char *bootText = "PlayStation 4";
  ImVec2 textSize = ImGui::CalcTextSize(bootText);
  ImGui::SetCursorPos(
      ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f));

  // Pulse effect based on time
  float time = (float)SDL_GetTicks() / 1000.0f;
  float alpha = 0.5f + 0.5f * std::sin(time * 3.0f);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
  ImGui::Text("%s", bootText);
  ImGui::PopStyleColor();

  // Loading indicator
  ImGui::SetCursorPos(ImVec2(center.x - 50, center.y + 50));
  ImGui::Text("Loading...");

  ImGui::End();
}

int main(int argc, char **argv) {
  if (InitializeLogFile()) {
    printf("[Main] Logging to %s\n", GetLogFilePath().string().c_str());
  }
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  if (g_log_file) {
    setvbuf(g_log_file, NULL, _IONBF, 0);
  }
  AddVectoredExceptionHandler(1, VectoredExceptionHandler);
  std::set_terminate(TerminateHandler);

  printf("========================================\n");
  printf("LayraPS4 - PlayStation 4 OS Emulator\n");
  printf("========================================\n\n");

  // Set App Metadata - New in SDL3
  SDL_SetAppMetadata("LayraPS4", "0.0.1", "com.layra.emulator");
  SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Layra Project");

  // SDL3: SDL_Init returns true on success, false on failure
  printf("[Main] Initializing SDL...\n");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    char msg[1024];
    snprintf(msg, sizeof(msg), "ERROR: SDL_Init failed: %s", SDL_GetError());
    return FatalError(msg);
  }
  printf("[Main] SDL initialized\n");

  printf("[Main] Creating window...\n");
  SDL_Window *window =
      SDL_CreateWindow("LayraPS4", 1920, 1080,
                       SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY);

  if (!window) {
    char msg[1024];
    snprintf(msg, sizeof(msg), "ERROR: Failed to create window: %s", SDL_GetError());
    SDL_Quit();
    return FatalError(msg);
  }
  printf("[Main] Window created\n");

  printf("[Main] Initializing Vulkan...\n");
  LayraVulkanContext vk{};
  if (!layra_vulkan_init(&vk, window)) {
    const char *error = layra_vulkan_get_last_error();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return FatalError(error ? error : "Failed to initialize Vulkan");
  }
  printf("[Main] Vulkan initialized\n");

  // Initialize Emulator
  printf("\n[Main] Creating emulator instance...\n");
  g_emulator_instance.reset(new Core::Emulator());

  printf("[Main] Initializing emulator...\n");
  if (!g_emulator_instance->Initialize()) {
    layra_vulkan_cleanup(&vk);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return FatalError("Failed to initialize emulator");
  }
  printf("[Main] Emulator initialized successfully\n");

  // Validate Vulkan state BEFORE attempting to use the handles
  printf("[Main] Validating Vulkan state...\n");
  bool invalidVulkanState = false;
  if (vk.device == VK_NULL_HANDLE) {
    fprintf(stderr, "[Main] Vulkan device handle is NULL\n");
    invalidVulkanState = true;
  }
  if (vk.graphicsQueue == VK_NULL_HANDLE) {
    fprintf(stderr, "[Main] Vulkan graphics queue handle is NULL\n");
    invalidVulkanState = true;
  }
  if (vk.graphicsQueueFamilyIndex == UINT32_MAX) {
    fprintf(stderr, "[Main] Vulkan graphics queue family index is invalid (UINT32_MAX)\n");
    invalidVulkanState = true;
  }
  if (vk.renderPass == VK_NULL_HANDLE) {
    fprintf(stderr, "[Main] Vulkan render pass handle is NULL\n");
    invalidVulkanState = true;
  }
  if (invalidVulkanState) {
    char msg[512];
    snprintf(msg, sizeof(msg), "Invalid Vulkan state detected: device=%p queue=%p queueFamily=%u renderPass=%p",
             reinterpret_cast<void *>(vk.device),
             reinterpret_cast<void *>(vk.graphicsQueue),
             vk.graphicsQueueFamilyIndex,
             reinterpret_cast<void *>(vk.renderPass));
    layra_vulkan_cleanup(&vk);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return FatalError(msg);
  }

  printf("[Main] Connecting Vulkan context to Emulator...\n");
  int w, h;
  SDL_GetWindowSizeInPixels(window, &w, &h);
  g_emulator_instance->SetVulkanContext(vk.device, vk.physicalDevice, vk.graphicsQueue, vk.graphicsQueueFamilyIndex, vk.renderPass, w, h);
  printf("[Main] Vulkan context connected\n\n");

  if (argc > 1) {
    std::string path = argv[1];
    printf("[Main] Loading executable: %s\n", path.c_str());
    if (g_emulator_instance->LoadExecutable(path)) {
      printf("[Main] Executable loaded successfully\n");
    } else {
      fprintf(stderr, "[Main] Failed to load executable: %s\n", path.c_str());
    }
  }

  // Initialize ImGui
  printf("[Main] Initializing ImGui...\n");
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
  printf("[Main] ImGui context created\n");

  printf("[Main] Initializing ImGui backends...\n");
  ImGui_ImplSDL3_InitForVulkan(window);

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.ApiVersion = VK_API_VERSION_1_0;
  initInfo.Instance = vk.instance;
  initInfo.PhysicalDevice = vk.physicalDevice;
  initInfo.Device = vk.device;
  initInfo.QueueFamily = vk.graphicsQueueFamilyIndex;
  initInfo.Queue = vk.graphicsQueue;
  initInfo.DescriptorPool = VK_NULL_HANDLE;
  initInfo.DescriptorPoolSize = 1000;
  initInfo.MinImageCount = 2;
  initInfo.ImageCount = 3;
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.UseDynamicRendering = false;
  initInfo.Allocator = nullptr;
  initInfo.CheckVkResultFn = nullptr;
  initInfo.MinAllocationSize = 1024 * 1024;
  // Pipeline info (RenderPass, MSAASamples) moved to PipelineInfoMain in
  // current ImGui
  initInfo.PipelineInfoMain.RenderPass = vk.renderPass;
  initInfo.PipelineInfoMain.Subpass = 0;
  initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  if (!ImGui_ImplVulkan_Init(&initInfo)) {
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (gDescriptorPool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(vk.device, gDescriptorPool, nullptr);
    }
    g_emulator_instance.reset();
    layra_vulkan_cleanup(&vk);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return FatalError("Failed to initialize ImGui Vulkan backend");
  }
  printf("[Main] ImGui backends initialized\n");

  // Initialize PS4 UI
  printf("[Main] Initializing PS4 UI...\n");
  Gui::PS4UI::Initialize();

  Gui::PS4UI::SetLaunchCallback([&](const std::string &exe_path) {
    printf("[Main] Loading PS4 executable: %s\n", exe_path.c_str());

    if (!g_emulator_instance) {
      fprintf(stderr, "[Main] ERROR: Emulator not initialized!\n");
      return;
    }

    // Load the PS4 ELF binary into our emulator
    if (g_emulator_instance->LoadExecutable(exe_path)) {
      printf("[Main] Executable loaded successfully, starting execution\n");
      g_emulator_instance->Run();
    } else {
      fprintf(stderr, "[Main] Failed to load executable: %s\n",
              exe_path.c_str());
    }
  });

  printf("[Main] PS4 UI initialized\n\n");

  printf("[Main] Entering main loop...\n\n");
  printf("========================================\n\n");

  bool done = false;
  Uint64 bootStart = SDL_GetTicks();
  bool boot_complete = false;

  while (!done) {
    // Process events
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      ImGui_ImplSDL3_ProcessEvent(&ev);
      if (ev.type == SDL_EVENT_QUIT) {
        printf("\n[Main] Quit event received\n");
        done = true;
      }
      if (ev.type == SDL_EVENT_WINDOW_RESIZED) {
          // Orbis OS expects fixed resolutions (1920x1080).
          // We scale the UI, but the internal emulator resolution remains fixed.
          if (vk.device != VK_NULL_HANDLE) {
              layra_vulkan_recreate_swapchain(&vk, window);
          }
      }
      if (ev.type == SDL_EVENT_KEY_DOWN) {
        if (ev.key.key == SDLK_ESCAPE) {
          printf("\n[Main] ESC pressed, exiting...\n");
          done = true;
        }
      }
    }

    // Start ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Show boot sequence for 3 seconds, then PS4 UI
    if (SDL_GetTicks() - bootStart < 3000) {
      RenderPS4BootSequence(io);
    } else {
      if (!boot_complete) {
        printf("[Main] Boot complete, showing PS4 UI\n");
        boot_complete = true;
      }

      // Handle PS4 UI input
      Gui::PS4UI::HandleInput();

      // Render PS4 UI
      Gui::PS4UI::Render();
    }

    // Step emulator if running
    if (g_emulator_instance && g_emulator_instance->IsRunning()) {
      g_emulator_instance->Step();
    }

    // Render ImGui
    ImGui::Render();

    // Render frame
    layra_vulkan_render_frame(&vk, ImGui_RenderCallback);

    // Cap framerate
    SDL_Delay(16); // ~60 FPS
  }

  // Cleanup
  printf("\n========================================\n");
  printf("[Main] Shutting down...\n");
  vkDeviceWaitIdle(vk.device);

  printf("[Main] Cleaning up ImGui...\n");
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  printf("[Main] Cleaning up Vulkan...\n");
  if (gDescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(vk.device, gDescriptorPool, nullptr);
  }
  layra_vulkan_cleanup(&vk);

  printf("[Main] Shutting down emulator...\n");
  g_emulator_instance.reset();

  printf("[Main] Cleaning up SDL...\n");
  SDL_DestroyWindow(window);
  SDL_Quit();

  printf("\n[Main] Shutdown complete\n");
  printf("========================================\n");
  CloseLogFile();
  return 0;
}
