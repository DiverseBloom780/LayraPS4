// LayraPS4  PS4 OS Emulator (error-free build) main.cpp
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
    void audio_play_boot_sound() {
        // Load the boot sound file
        SDL_AudioSpec spec;
        Uint32 wav_length;
        Uint8* wav_buffer;
        SDL_LoadWAV("bootsound.wav", &spec, &wav_buffer, &wav_length);

        // Play the sound
        SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
        SDL_QueueAudio(device, wav_buffer, wav_length);
        SDL_PauseAudioDevice(device, 0);
    }

    void kernel_init(void* arg) {
        // Initialize kernel memory management
        kernel_memory_init();

        // Initialize kernel threads
        kernel_thread_init();
    }

    void kernel_shutdown(void* arg) {
        // Shutdown kernel memory management
        kernel_memory_shutdown();

        // Shutdown kernel threads
        kernel_thread_shutdown();

        // Release any resources used by the kernel
        // ...
    }

    void modules_init() {
        // Load the necessary modules
        module_load("module1");
        module_load("module2");

        // Initialize module functionality
        module_init("module1");
        module_init("module2");
    }

    void audio_init() {
        // Initialize audio subsystem
        audio_subsystem_init();

        // Set up audio devices
        audio_device_setup();
    }

    void pad_init() {
        // Initialize pad subsystem
        pad_subsystem_init();

        // Set up pad devices
        pad_device_setup();
    }

    void savedata_init() {
        // Initialize savedata subsystem
        savedata_subsystem_init();

        // Set up savedata devices
        savedata_device_setup();
    }

    void trophy_init() {
        // Initialize trophy subsystem
        trophy_subsystem_init();

        // Set up trophy devices
        trophy_device_setup();
    }

    // Add this function to the orbis namespace
    void kernel_memory_init() {
        // Initialize kernel memory management
        // ...
    }

    void kernel_memory_shutdown() {
        // Shutdown kernel memory management
        // ...
    }

    void kernel_thread_init() {
        // Initialize kernel threads
        // ...
    }

    void kernel_thread_shutdown() {
        // Shutdown kernel threads
        // ...
    }

    // Add this function to the orbis namespace
    void module_load(const std::string& name) {
        // Load the module with the given name
        // ...
    }

    void module_init(const std::string& name) {
        // Initialize the module with the given name
        // ...
    }

    // Add this function to the orbis namespace
    void audio_subsystem_init() {
        // Initialize the audio subsystem
        // ...
    }

    void audio_subsystem_shutdown() {
        // Shutdown the audio subsystem
        // ...
    }

    void audio_device_setup() {
        // Set up the audio devices
        // ...
    }

    void audio_device_shutdown() {
        // Shutdown the audio devices
        // ...
    }

    // Add this function to the orbis namespace
    void pad_subsystem_init() {
        // Initialize the pad subsystem
        // ...
    }

    void pad_subsystem_shutdown() {
        // Shutdown the pad subsystem
        // ...
    }

    void pad_device_setup() {
        // Set up the pad devices
        // ...
    }

    void pad_device_shutdown() {
        // Shutdown the pad devices
        // ...
    }

    // Add this function to the orbis namespace
    void savedata_subsystem_init() {
        // Initialize the savedata subsystem
        // ...
    }

    void savedata_subsystem_shutdown() {
        // Shutdown the savedata subsystem
        // ...
    }

    void savedata_device_setup() {
        // Set up the savedata devices
        // ...
    }

    void savedata_device_shutdown() {
        // Shutdown the savedata devices
        // ...
    }

    // Add this function to the orbis namespace
    void trophy_subsystem_init() {
        // Initialize the trophy subsystem
        // ...
    }

    void trophy_subsystem_shutdown() {
        // Shutdown the trophy subsystem
        // ...
    }

    void trophy_device_setup() {
        // Set up the trophy devices
        // ...
    }
}

#include "layra_pkg.h"
#include "layra_vulkan.h"

struct Theme {
    std::string name;
    ImVec4 backgroundColor;
    ImVec4 buttonColor;
    ImVec4 buttonHoverColor;
};

class ThemeManager {
public:
    ThemeManager() {}
    ~ThemeManager() {}

    void addTheme(const Theme& theme) {
        themes.push_back(theme);
    }

    void selectTheme(const std::string& themeName) {
        for (const auto& theme : themes) {
            if (theme.name == themeName) {
                currentTheme = theme;
                break;
            }
        }
    }

    const Theme& getCurrentTheme() const {
        return currentTheme;
    }

private:
    std::vector<Theme> themes;
    Theme currentTheme;
};

static VkDescriptorPool gDescriptorPool = VK_NULL_HANDLE;
ThemeManager themeManager;

void ImGui_RenderCallback(VkCommandBuffer cmd) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

// PS4 Boot Sequence
void RenderPS4BootSequence(ImGuiIO& io) {
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

// PS4 Dashboard
void RenderPS4Dashboard(ImGuiIO& io) {
    themeManager.addTheme(Theme{"Default", ImVec4(0.06f, 0.08f, 0.12f, 0.95f), ImVec4(0.16f, 0.29f, 0.48f, 0.40f), ImVec4(0.26f, 0.59f, 0.98f, 1.00f)});
    themeManager.addTheme(Theme{"Dark", ImVec4(0.02f, 0.02f, 0.02f, 0.95f), ImVec4(0.08f, 0.08f, 0.08f, 0.40f), ImVec4(0.12f, 0.12f, 0.12f, 1.00f)});
    themeManager.selectTheme("Default");

    const Theme& currentTheme = themeManager.getCurrentTheme();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, currentTheme.backgroundColor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("PS4 Dashboard", nullptr,
                 ImGuiWindowFlags_NoTitleBar
//ImGuiWindowFlags_NoResize
                 

ImGuiWindowFlags_NoMove
 ImGuiWindowFlags_NoCollapse);
    // Top row  function icons
    ImGui::Indent(60);
    const char* icons[] = {"Store", "Friends", "Settings", "Power"};
    for (int i = 0; i < 4; ++i) {
        if (ImGui::Button(icons[i], ImVec2(120, 40))) {}
        ImGui::SameLine(0, 20);
    }
    // Middle row  game tiles (hard-coded for now)
    float contentY = io.DisplaySize.y * 0.35f;
    ImGui::SetCursorPos(ImVec2(100, contentY));
    const char* games[] = {"Bloodborne", "Playroom", "The Last of Us Part II"};
    for (int i = 0; i < 3; ++i) {
        ImGui::BeginGroup();
        if (ImGui::Button(games[i], ImVec2(240, 240))) {}
        ImGui::Text("%s", games[i]);
        ImGui::EndGroup();
        ImGui::SameLine(0, 30);
    }

    // Theme selection menu
    if (ImGui::BeginMenu("Themes")) {
        for (const auto& theme : themeManager.themes) {
            if (ImGui::MenuItem(theme.name.c_str())) {
                themeManager.selectTheme(theme.name);
            }
        }
        ImGui::EndMenu();
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

/* ----------  Main  ---------- */
int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO
//SDL_INIT_TIMER	SDL_INIT_GAMEPAD
                  SDL_INIT_AUDIO) != 0) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Window* window = SDL_CreateWindow(
        "LayraPS4 - PS4 OS Emulator", 1920, 1080,
        SDL_WINDOW_VULKAN
//SDL_WINDOW_RESIZABLE
        SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) return -1;
    LayraVulkanContext vk{};
    if (!layra_vulkan_init(vk, window)) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags
// = ImGuiConfigFlags_NavEnableKeyboard
// ImGuiConfigFlags_NavEnableGamepad;
    // PS4 dark theme
    ImVec4* c = ImGui::GetStyle().Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.08f, 0.12f, 0.95f);
    c[ImGuiCol_Button] = ImVec4(0.16f, 0.29f, 0.48f, 0.40f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    // Descriptor pool
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000}
    };
    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000,
        .poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes)),
        .pPoolSizes = poolSizes
    };
    vkCreateDescriptorPool(vk.device, &poolInfo, nullptr, &gDescriptorPool);

    ImGui_ImplSDL3_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo initInfo{
        .Instance = vk.instance,
        .PhysicalDevice = vk.physicalDevice,
        .Device = vk.device,
        .QueueFamily = vk.graphicsQueueFamilyIndex,
        .Queue = vk.graphicsQueue,
        .DescriptorPool = gDescriptorPool,
        .MinImageCount = 2,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    };
    ImGui_ImplVulkan_Init(initInfo);

    // Initialize PS4 subsystems (stubs)
    orbis::audio_play_boot_sound();
    orbis::kernel_init(nullptr);
    orbis::modules_init();
    orbis::audio_init();
    orbis::pad_init();
    orbis::savedata_init();
    orbis::trophy_init();

    bool done = false;
    Uint64 bootStart = SDL_GetTicks();

    while (!done) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) done = true;
            if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED)
                layra_vulkan_recreate_swapchain(vk, window);
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (SDL_GetTicks() - bootStart < 3000) {
            RenderPS4BootSequence(io);
        } else {
            RenderPS4Dashboard(io);
        }

        ImGui::Render();
        layra_vulkan_render_frame(vk, ImGui_RenderCallback);
    }

    // Cleanup
    vkDeviceWaitIdle(vk.device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(vk.device, gDescriptorPool, nullptr);
    layra_vulkan_cleanup(vk);
    orbis::kernel_shutdown(nullptr); // Add this line to call the kernel_shutdown function
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
