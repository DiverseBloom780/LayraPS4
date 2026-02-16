// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// CRITICAL: imgui.h MUST be included FIRST!
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/backends/imgui_impl_sdl3.h"   // ← Now this works
#include "../lib/imgui/backends/imgui_impl_vulkan.h" // ← Now this works
#include "../lib/imgui/imgui.h"                      // ← FIRST!

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <cmath>
#include <cstdio>
#include <memory>

#include "emulator.h"
#include "gui/ps4_ui.h"
#include "layra_vulkan.h"

// Global instance
std::unique_ptr<Core::Emulator> g_emulator_instance;

// Static globals
static VkDescriptorPool gDescriptorPool = VK_NULL_HANDLE;

// Forward declarations
void ImGui_RenderCallback(VkCommandBuffer cmd);
void RenderPS4BootSequence(ImGuiIO &io);

// Implementation of render callback
void ImGui_RenderCallback(VkCommandBuffer cmd) {
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
  printf("========================================\n");
  printf("LayraPS4 - PlayStation 4 OS Emulator\n");
  printf("========================================\n\n");

  // SDL3: SDL_Init returns true on success, false on failure
  printf("[Main] Initializing SDL...\n");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    fprintf(stderr, "ERROR: SDL_Init failed: %s\n", SDL_GetError());
    return -1;
  }
  printf("[Main] SDL initialized\n");

  printf("[Main] Creating window...\n");
  SDL_Window *window =
      SDL_CreateWindow("LayraPS4", 1920, 1080,
                       SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY);

  if (!window) {
    fprintf(stderr, "ERROR: Failed to create window: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }
  printf("[Main] Window created\n");

  printf("[Main] Initializing Vulkan...\n");
  LayraVulkanContext vk{};
  if (!layra_vulkan_init(&vk, window)) {
    fprintf(stderr, "FATAL: Failed to initialize Vulkan\n");
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }
  printf("[Main] Vulkan initialized\n");

  // Initialize Emulator
  printf("\n[Main] Creating emulator instance...\n");
  g_emulator_instance.reset(new Core::Emulator());

  printf("[Main] Initializing emulator...\n");
  if (!g_emulator_instance->Initialize()) {
    fprintf(stderr, "FATAL: Failed to initialize emulator\n");
    layra_vulkan_cleanup(&vk);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }
  printf("[Main] Emulator initialized successfully\n\n");

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

  printf("[Main] Creating descriptor pool...\n");
  VkDescriptorPoolSize poolSizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000}};

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets = 1000;
  poolInfo.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes));
  poolInfo.pPoolSizes = poolSizes;

  if (vkCreateDescriptorPool(vk.device, &poolInfo, nullptr, &gDescriptorPool) !=
      VK_SUCCESS) {
    fprintf(stderr, "FATAL: Failed to create descriptor pool\n");
    ImGui::DestroyContext();
    g_emulator_instance.reset();
    layra_vulkan_cleanup(&vk);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }
  printf("[Main] Descriptor pool created\n");

  printf("[Main] Initializing ImGui backends...\n");
  ImGui_ImplSDL3_InitForVulkan(window);

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.ApiVersion = VK_API_VERSION_1_0;
  initInfo.Instance = vk.instance;
  initInfo.PhysicalDevice = vk.physicalDevice;
  initInfo.Device = vk.device;
  initInfo.QueueFamily = vk.graphicsQueueFamilyIndex;
  initInfo.Queue = vk.graphicsQueue;
  initInfo.DescriptorPool = gDescriptorPool;
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
    fprintf(stderr, "FATAL: Failed to initialize ImGui Vulkan backend\n");
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(vk.device, gDescriptorPool, nullptr);
    g_emulator_instance.reset();
    layra_vulkan_cleanup(&vk);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }
  printf("[Main] ImGui backends initialized\n");

  // Initialize PS4 UI
  printf("[Main] Initializing PS4 UI...\n");
  Gui::PS4UI::Initialize();
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
  vkDestroyDescriptorPool(vk.device, gDescriptorPool, nullptr);
  layra_vulkan_cleanup(&vk);

  printf("[Main] Shutting down emulator...\n");
  g_emulator_instance.reset();

  printf("[Main] Cleaning up SDL...\n");
  SDL_DestroyWindow(window);
  SDL_Quit();

  printf("\n[Main] Shutdown complete\n");
  printf("========================================\n");
  return 0;
}