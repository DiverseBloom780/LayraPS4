#pragma once

#include <cstdint>
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <stdbool.h>
#include <stdio.h>

// Forward declarations
typedef struct VkDebugUtilsMessengerEXT_T *VkDebugUtilsMessengerEXT;
typedef struct VkCommandBuffer_T *VkCommandBuffer;

#define LAYRA_MAX_FRAMES_IN_FLIGHT 3

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
  VkImage *swapChainImages;
  VkImageView *swapChainImageViews;
  VkRenderPass renderPass;
  VkFramebuffer *swapChainFramebuffers;
  VkCommandPool commandPool;
  VkCommandBuffer *commandBuffers;

  // Per-frame synchronization (one set per swapchain image)
  VkSemaphore imageAvailableSemaphores[LAYRA_MAX_FRAMES_IN_FLIGHT];
  VkSemaphore renderFinishedSemaphores[LAYRA_MAX_FRAMES_IN_FLIGHT];
  VkFence inFlightFences[LAYRA_MAX_FRAMES_IN_FLIGHT];
  uint32_t currentFrame;

  // Legacy single semaphore fields (kept for backward compat during init)
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;

} LayraVulkanContext;

// Initialize Vulkan context
bool layra_vulkan_init(LayraVulkanContext *context, SDL_Window *window);

// Get the last Vulkan initialization error message.
const char *layra_vulkan_get_last_error();

// Recreate swapchain and related resources (e.g., on window resize)
bool layra_vulkan_recreate_swapchain(LayraVulkanContext *context,
                                     SDL_Window *window);

// Render a frame
void layra_vulkan_render_frame(LayraVulkanContext *context,
                               void (*draw_callback)(VkCommandBuffer));

// Cleanup Vulkan context
void layra_vulkan_cleanup(LayraVulkanContext *context);
