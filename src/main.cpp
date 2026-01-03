#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <cstring> // For memset and memcpy

#include <SDL.h>
// #include <SDL_image.h>
#include <SDL_vulkan.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"


#include "layra_pkg.h"
#include "layra_vfs.h"
#include "layra_vulkan.h"

// Placeholder for Vulkan specific ImGui descriptor pool
static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

// Forward declaration for ImGui rendering callback
void ImGui_RenderCallback(VkCommandBuffer commandBuffer);


// XMB-inspired menu rendering function
void RenderXMB() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.8f)); // Semi-transparent background

    ImGui::Begin("XMB Menu", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Horizontal Categories
    const char* categories[] = { "Games", "Media", "Settings", "Profile", "Power" };
    static int current_category = 0;

    ImGui::BeginChild("Categories", ImVec2(io.DisplaySize.x, 50), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(30, 0));
    
    for (int i = 0; i < IM_ARRAYSIZE(categories); i++) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::Selectable(categories[i], current_category == i, 0, ImVec2(0, 30))) {
            current_category = i;
        }
    }
    ImGui::PopStyleVar(2);
    ImGui::EndChild();

    // Vertical Sub-menus based on selected category
    ImGui::BeginChild("SubMenu", ImVec2(io.DisplaySize.x, io.DisplaySize.y - 50));
    switch (current_category) {
        case 0: // Games
            ImGui::Text("Games List:");
            if (ImGui::Selectable("Game 1")) { /* Launch Game 1 */ }
            if (ImGui::Selectable("Game 2")) { /* Launch Game 2 */ }
            break;
        case 1: // Media
            ImGui::Text("Media Options:");
            if (ImGui::Selectable("Screenshots")) { /* Open Screenshots */ }
            if (ImGui::Selectable("Video Captures")) { /* Open Video Captures */ }
            break;
        case 2: // Settings
            ImGui::Text("Emulator Settings:");
            if (ImGui::Selectable("Graphics")) { /* Open Graphics Settings */ }
            if (ImGui::Selectable("Audio")) { /* Open Audio Settings */ }
            break;
        case 3: // Profile
            ImGui::Text("Profile Management:");
            if (ImGui::Selectable("Select Profile")) { /* Open Profile Selector */ }
            if (ImGui::Selectable("Edit Profile")) { /* Open Profile Editor */ }
            break;
        case 4: // Power
            ImGui::Text("Power Options:");
            if (ImGui::Selectable("Shutdown")) { /* Shutdown Emulator */ }
            if (ImGui::Selectable("Restart")) { /* Restart Emulator */ }
            break;
    }
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void ImGui_RenderCallback(VkCommandBuffer commandBuffer) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

// Main code
int main(int argc, char* argv[])
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Initialize PNG loading (temporarily removed for build compatibility)
    // if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    //     printf("Error initializing SDL_image: %s\n", IMG_GetError());
    //     return -1;
    // }

    // Create window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("LayraPS4 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);

    LayraVulkanContext vulkan_context;
    memset(&vulkan_context, 0, sizeof(LayraVulkanContext)); // Initialize to zeros

    if (!layra_vulkan_init(&vulkan_context, window)) {
        fprintf(stderr, "Failed to initialize Vulkan!\n");
        SDL_DestroyWindow(window);
        // IMG_Quit();
        SDL_Quit();
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Create Descriptor Pool for ImGui
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000 * IM_ARRAYSIZE(pool_sizes),
        .poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes),
        .pPoolSizes = pool_sizes,
    };
    if (vkCreateDescriptorPool(vulkan_context.device, &pool_info, NULL, &g_DescriptorPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create ImGui descriptor pool!\n");
        layra_vulkan_cleanup(&vulkan_context);
        SDL_DestroyWindow(window);
        // IMG_Quit();
        SDL_Quit();
        return -1;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = { };
    init_info.Instance = vulkan_context.instance;
    init_info.PhysicalDevice = vulkan_context.physicalDevice;
    init_info.Device = vulkan_context.device;
    init_info.QueueFamily = vulkan_context.graphicsQueueFamilyIndex;
    init_info.Queue = vulkan_context.graphicsQueue;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 3;
    init_info.PipelineInfoMain.RenderPass = vulkan_context.renderPass;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&init_info);

    // Upload Fonts (handled internally by ImGui_ImplVulkan_Init, manual command buffer submission is not needed here)
    // The following code block is removed to avoid conflicts and ensure correct behavior.
    // Original code:
    //    VkCommandPool font_command_pool;
    //    VkCommandPoolCreateInfo cmd_pool_info = {};
    //    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //    cmd_pool_info.queueFamilyIndex = vulkan_context.graphicsQueueFamilyIndex;
    //    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //    vkCreateCommandPool(vulkan_context.device, &cmd_pool_info, NULL, &font_command_pool);
    //
    //    VkCommandBuffer font_command_buffer;
    //    VkCommandBufferAllocateInfo cmd_buf_alloc_info = {};
    //    cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //    cmd_buf_alloc_info.commandPool = font_command_pool;
    //    cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    //    cmd_buf_alloc_info.commandBufferCount = 1;
    //    vkAllocateCommandBuffers(vulkan_context.device, &cmd_buf_alloc_info, &font_command_buffer);
    //
    //    VkCommandBufferBeginInfo begin_info = {};
    //    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    //    vkBeginCommandBuffer(font_command_buffer, &begin_info);
    //    ImGui_ImplVulkan_CreateFontsTexture(font_command_buffer);
    //
    //    VkSubmitInfo end_info = {};
    //    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //    end_info.commandBufferCount = 1;
    //    end_info.pCommandBuffers = &font_command_buffer;
    //    vkEndCommandBuffer(font_command_buffer);
    //    vkQueueSubmit(vulkan_context.graphicsQueue, 1, &end_info, VK_NULL_HANDLE);
    //    vkQueueWaitIdle(vulkan_context.graphicsQueue);
    //
    //
    //    vkFreeCommandBuffers(vulkan_context.device, font_command_pool, 1, &font_command_buffer);
    //    vkDestroyCommandPool(vulkan_context.device, font_command_pool, NULL);

    // Main loop
    bool done = false;
    bool show_boot_screen = true;
    Uint32 boot_screen_start_time = SDL_GetTicks();

    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                // Handle window resize
                layra_vulkan_recreate_swapchain(&vulkan_context, window);
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (show_boot_screen) {
            // Display boot screen for a few seconds
            if (SDL_GetTicks() - boot_screen_start_time > 3000) { // 3 seconds
                show_boot_screen = false;
            }

            // Placeholder for boot screen rendering with Vulkan
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("Boot Screen", &show_boot_screen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            ImGui::Text("LayraPS4 - Booting..."); // Placeholder text
            ImGui::End();
            ImGui::PopStyleVar();

        } else {
            RenderXMB();
        }

        // Rendering
        ImGui::Render();
        layra_vulkan_render_frame(&vulkan_context, ImGui_RenderCallback);
    }

    // Cleanup
    vkDeviceWaitIdle(vulkan_context.device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (g_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(vulkan_context.device, g_DescriptorPool, NULL);
    }

    layra_vulkan_cleanup(&vulkan_context);
    SDL_DestroyWindow(window);
    // IMG_Quit();
    SDL_Quit();

    return 0;
}

