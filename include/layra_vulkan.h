#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <stdio.h>
#include <stdbool.h>

// Forward declaration for debug messenger
typedef struct VkDebugUtilsMessengerEXT_T* VkDebugUtilsMessengerEXT;

typedef struct {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamilyIndex;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    uint32_t imageCount;
    VkImage* swapChainImages;
    VkImageView* swapChainImageViews;
    VkRenderPass renderPass;
    VkFramebuffer* swapChainFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    // Vulkan function pointers for dynamic loading


} LayraVulkanContext;

// Initialize Vulkan context
bool layra_vulkan_init(LayraVulkanContext* context, SDL_Window* window);

// Recreate swapchain and related resources (e.g., on window resize)
bool layra_vulkan_recreate_swapchain(LayraVulkanContext* context, SDL_Window* window);

// Render a frame
void layra_vulkan_render_frame(LayraVulkanContext* context, void (*draw_callback)(VkCommandBuffer));

// Cleanup Vulkan context
void layra_vulkan_cleanup(LayraVulkanContext* context);

