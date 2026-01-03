// LayraPS4 – PS4 OS Emulator (error-free build)
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

// Stubs (replace with real implementations later)
namespace orbis {
    void audio_init() {}
    void pad_init() {}
    void kernel_init(void*) {}
    void kernel_shutdown(void*) {}
    void modules_init() {}
    void savedata_init() {}
    void trophy_init() {}
    void play_boot_sound() {}
}

#include "layra_pkg.h"
#include "layra_vfs.h"
#include "layra_vulkan.h"

static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

void ImGui_RenderCallback(VkCommandBuffer cmd) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

/* ----------  PS4 Dashboard  ---------- */
void RenderDashboard(ImGuiIO& io) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.08f, 0.12f, 0.95f));

    ImGui::Begin("PS4 Dashboard", nullptr,
                 ImGuiWindowFlags_NoTitleBar 

ImGuiWindowFlags_NoResize
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    // Top row – content tiles
    float topY = io.DisplaySize.y * 0.15f;
    ImGui::SetCursorPosY(topY);
    ImGui::Indent(60);
    const char* tiles[] = { "What's New", "Library", "Store", "Capture Gallery" };
    for (int i = 0; i < IM_ARRAYSIZE(tiles); ++i) {
        ImGui::PushID(i);
        if (ImGui::Button(tiles[i], ImVec2(150, 150))) {}
        ImGui::PopID();
        if (i < IM_ARRAYSIZE(tiles) - 1) ImGui::SameLine();
    }

    // Bottom row – function icons
    float bottomY = io.DisplaySize.y * 0.75f;
    ImGui::SetCursorPosY(bottomY);
    ImGui::Indent(60);
    const char* icons[] = { "Friends", "Messages", "Notifications", "Profile", "Settings", "Power" };
    for (int i = 0; i < IM_ARRAYSIZE(icons); ++i) {
        ImGui::PushID(i + 100);
        if (ImGui::Button(icons[i], ImVec2(100, 100))) {}
        ImGui::PopID();
        if (i < IM_ARRAYSIZE(icons) - 1) ImGui::SameLine();
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

/* ----------  Boot Sequence  ---------- */
void RenderBootSequence(ImGuiIO& io) {
    static Uint64 start = SDL_GetTicks();
    Uint64 now = SDL_GetTicks();
    float alpha = (now - start) < 2000 ? 1.0f : 0.0f;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, alpha));

    ImGui::Begin("Boot", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    ImVec2 txt = ImGui::CalcTextSize("LayraPS4 - Booting Orbis OS...");
    ImGui::SetCursorPos((io.DisplaySize - txt) * 0.5f);
    ImGui::Text("LayraPS4 - Booting Orbis OS...");
    ImGui::End();
    ImGui::PopStyleColor();
}

/* ----------  Main  ---------- */
int main(int, char*[]) {
    if (SDL_Init(SDL_INIT_VIDEO 

SDL_INIT_TIMER	SDL_INIT_GAMEPAD
 SDL_INIT_AUDIO) != 0) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Window* window = SDL_CreateWindow(
        "LayraPS4 - PS4 OS Emulator", 1920, 1080,
        SDL_WINDOW_VULKAN 

SDL_WINDOW_RESIZABLE
 SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) return -1;
    LayraVulkanContext vk{};
    if (!layra_vulkan_init(&vk, window)) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags 

= ImGuiConfigFlags_NavEnableKeyboard
 ImGuiConfigFlags_NavEnableGamepad;
    // PS4 dark theme
    ImVec4* c = ImGui::GetStyle().Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.08f, 0.12f, 0.95f);
    c[ImGuiCol_Button] = ImVec4(0.16f, 0.29f, 0.48f, 0.40f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    // Descriptor pool
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000,
        .poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes),
        .pPoolSizes = pool_sizes
    };
    vkCreateDescriptorPool(vk.device, &pool_info, nullptr, &g_DescriptorPool);

    ImGui_ImplSDL3_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info{
        .Instance = vk.instance,
        .PhysicalDevice = vk.physicalDevice,
        .Device = vk.device,
        .QueueFamily = vk.graphicsQueueFamilyIndex,
        .Queue = vk.graphicsQueue,
        .DescriptorPool = g_DescriptorPool,
        .MinImageCount = 2,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    };
    ImGui_ImplVulkan_Init(&init_info);

    // Initialize PS4 subsystems (stubs for now)
    orbis::kernel_init(nullptr);
    orbis::modules_init();
    orbis::audio_init();
    orbis::pad_init();
    orbis::savedata_init();
    orbis::trophy_init();

    bool done = false;
    Uint64 boot_start = SDL_GetTicks();

    while (!done) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT) done = true;
            if (ev.type == SDL_EVENT_WINDOW_RESIZED)
                layra_vulkan_recreate_swapchain(&vk, window);
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (SDL_GetTicks() - boot_start < 3000) {
            RenderBootSequence(io);
        } else {
            RenderDashboard(io);
        }

        ImGui::Render();
        layra_vulkan_render_frame(&vk, ImGui_RenderCallback);
    }

    // Cleanup
    vkDeviceWaitIdle(vk.device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(vk.device, g_DescriptorPool, nullptr);
    layra_vulkan_cleanup(&vk);
    SDL_DestroyWindow(window);
    SDL_Quit();
    orbis::kernel_shutdown(nullptr);
    return 0;
}
