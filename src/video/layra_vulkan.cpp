
#include "layra_vulkan.h"
#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <set>
#include <string>
#include <vector>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#endif

static std::string g_last_vulkan_error;

const char *layra_vulkan_get_last_error() {
  return g_last_vulkan_error.c_str();
}

static void set_vulkan_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[2048];
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  g_last_vulkan_error = buffer;
}

// Runtime-configurable validation layers toggle.
// Set environment variable LAYRA_DISABLE_VK_VALIDATION=1 (or true) to disable.
const bool enableValidationLayers = []() -> bool {
#ifdef NDEBUG
  bool defaultVal = false;
#else
  bool defaultVal = true;
#endif
  const char *env = std::getenv("LAYRA_DISABLE_VK_VALIDATION");
  if (env) {
    if (env[0] == '1' || strcmp(env, "true") == 0 || strcmp(env, "TRUE") == 0) {
      return false;
    }
    if (env[0] == '0' || strcmp(env, "false") == 0 || strcmp(env, "FALSE") == 0) {
      return defaultVal;
    }
  }
  return defaultVal;
}();

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);

  return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

bool checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;
    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }
  return true;
}

bool layra_vulkan_init(LayraVulkanContext *context, SDL_Window *window) {
  fprintf(stderr, "[Vulkan] layra_vulkan_init() start\n");
  // Initialize all function pointers to null
  memset(context, 0, sizeof(LayraVulkanContext));

  // 1. Create Vulkan Instance
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "LayraPS4";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  fprintf(stderr, "[Vulkan] Loading Vulkan library...\n");
  if (SDL_Vulkan_LoadLibrary(NULL) != 0) {
    const char *sdl_error = SDL_GetError();
    fprintf(stderr, "Failed to load Vulkan library: %s\n", sdl_error);
    set_vulkan_error("Failed to load Vulkan library: %s", sdl_error);
#ifdef _WIN32
    DWORD err = GetLastError();
    if (err != 0) {
      LPVOID msgBuf = NULL;
      FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msgBuf, 0, NULL);
      if (msgBuf) {
        fprintf(stderr, "GetLastError: %u (%s)\n", (unsigned)err, (char *)msgBuf);
        set_vulkan_error("%s\nGetLastError: %u (%s)", g_last_vulkan_error.c_str(), (unsigned)err, (char *)msgBuf);
        LocalFree(msgBuf);
      } else {
        fprintf(stderr, "GetLastError: %u\n", (unsigned)err);
        set_vulkan_error("%s\nGetLastError: %u", g_last_vulkan_error.c_str(), (unsigned)err);
      }
    }

    bool loadedVulkan = false;
    const char *vulkan_sdk = std::getenv("VULKAN_SDK");
    if (vulkan_sdk) {
      std::string loaderPath = std::string(vulkan_sdk) + "\\Bin\\vulkan-1.dll";
      fprintf(stderr, "[Vulkan] Trying explicit loader path: %s\n", loaderPath.c_str());
      if (SDL_Vulkan_LoadLibrary(loaderPath.c_str()) == 0) {
        fprintf(stderr, "[Vulkan] Successfully loaded Vulkan library from VULKAN_SDK path.\n");
        loadedVulkan = true;
      } else {
        fprintf(stderr, "[Vulkan] Explicit loader path failed: %s\n", SDL_GetError());
        set_vulkan_error("%s\nExplicit Vulkan loader path failed: %s", g_last_vulkan_error.c_str(), SDL_GetError());
      }
    }

    if (!loadedVulkan) {
      constexpr const char *systemLoaderPath = "C:\\Windows\\System32\\vulkan-1.dll";
      fprintf(stderr, "[Vulkan] Trying explicit system loader path: %s\n", systemLoaderPath);
      if (SDL_Vulkan_LoadLibrary(systemLoaderPath) == 0) {
        fprintf(stderr, "[Vulkan] Successfully loaded Vulkan library from system path.\n");
        loadedVulkan = true;
      } else {
        fprintf(stderr, "[Vulkan] System loader path failed: %s\n", SDL_GetError());
        set_vulkan_error("%s\nSystem Vulkan loader path failed: %s", g_last_vulkan_error.c_str(), SDL_GetError());
      }
    }

    if (!loadedVulkan) {
      fprintf(stderr, "[Vulkan] Trying dynamic loader name vulkan-1.dll\n");
      if (SDL_Vulkan_LoadLibrary("vulkan-1.dll") == 0) {
        fprintf(stderr, "[Vulkan] Successfully loaded Vulkan library via system search path.\n");
        loadedVulkan = true;
      } else {
        fprintf(stderr, "[Vulkan] System search path load failed: %s\n", SDL_GetError());
        set_vulkan_error("%s\nSystem search path load failed: %s", g_last_vulkan_error.c_str(), SDL_GetError());
      }
    }

    if (!loadedVulkan) {
      HMODULE h2 = LoadLibraryExA("vulkan-1.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
      if (h2) {
        fprintf(stderr, "LoadLibraryExA(vulkan-1.dll) succeeded.\n");
        FreeLibrary(h2);
      } else {
        DWORD le2 = GetLastError();
        LPVOID msgBuf3 = NULL;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, le2, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msgBuf3, 0, NULL);
        if (msgBuf3) {
          fprintf(stderr, "LoadLibraryExA(vulkan-1.dll) failed -> %u (%s)\n", (unsigned)le2, (char *)msgBuf3);
          set_vulkan_error("%s\nLoadLibraryExA(vulkan-1.dll) failed -> %u (%s)", g_last_vulkan_error.c_str(), (unsigned)le2, (char *)msgBuf3);
          LocalFree(msgBuf3);
        } else {
          fprintf(stderr, "LoadLibraryExA(vulkan-1.dll) failed -> %u\n", (unsigned)le2);
          set_vulkan_error("%s\nLoadLibraryExA(vulkan-1.dll) failed -> %u", g_last_vulkan_error.c_str(), (unsigned)le2);
        }
      }
    }

    if (!loadedVulkan) {
      return false;
    }
#else
    return false;
#endif
  }
  fprintf(stderr, "[Vulkan] Loaded Vulkan library.\n");

  Uint32 extensions_count = 0;
  fprintf(stderr, "[Vulkan] Querying SDL instance extensions...\n");
  char const *const *extensions_names =
      SDL_Vulkan_GetInstanceExtensions(&extensions_count);
  if (!extensions_names) {
    fprintf(stderr, "Failed to get instance extensions: %s\n",
            SDL_GetError());
    return false;
  }
  if (extensions_count == 0) {
    fprintf(stderr, "SDL returned zero Vulkan instance extensions!\n");
    return false;
  }

  std::vector<const char *> extensions;
  extensions.reserve(extensions_count + 1);
  for (Uint32 index = 0; index < extensions_count; index++) {
    extensions.push_back(extensions_names[index]);
  }

  auto addInstanceExtension = [&](const char *name) {
    for (const char *existing : extensions) {
      if (strcmp(existing, name) == 0) {
        return;
      }
    }
    extensions.push_back(name);
  };

  if (enableValidationLayers) {
    addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  fprintf(stderr, "[Vulkan] Requested instance extensions (%zu):\n",
          extensions.size());
  for (const char *ext : extensions) {
    fprintf(stderr, "  %s\n", ext);
  }

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (enableValidationLayers) {
    if (!checkValidationLayerSupport()) {
      fprintf(stderr, "Validation layers requested, but not available!\n");
      return false;
    }
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  fprintf(stderr, "[Vulkan] Creating Vulkan instance...\n");
  if (vkCreateInstance(&createInfo, nullptr, &context->instance) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to create Vulkan instance!\n");
    return false;
  }
  fprintf(stderr, "[Vulkan] Vulkan instance created successfully.\n");

  if (enableValidationLayers) {
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    if (CreateDebugUtilsMessengerEXT(context->instance, &debugCreateInfo,
                                     nullptr,
                                     &context->debugMessenger) != VK_SUCCESS) {
      fprintf(stderr, "Failed to set up debug messenger!\n");
      return false;
    }
  }

  // 3. Create Surface
  fprintf(stderr, "[Vulkan] Creating Vulkan surface...\n");
  if (!SDL_Vulkan_CreateSurface(window, context->instance, nullptr,
                                &context->surface)) {
    fprintf(stderr, "Failed to create Vulkan surface: %s\n",
            SDL_GetError());
    return false;
  }
  fprintf(stderr, "[Vulkan] Surface created successfully.\n");

  // 4. Pick Physical Device
  uint32_t physicalDeviceCount = 0;
  vkEnumeratePhysicalDevices(context->instance, &physicalDeviceCount, nullptr);
  if (physicalDeviceCount == 0) {
    fprintf(stderr, "Failed to find GPUs with Vulkan support!\n");
    return false;
  }
  std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
  vkEnumeratePhysicalDevices(context->instance, &physicalDeviceCount,
                             physicalDevices.data());

  for (const auto &device : physicalDevices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader) {
      context->physicalDevice = device;
      break;
    }
    if (context->physicalDevice == VK_NULL_HANDLE) {
      context->physicalDevice = device;
    }
  }

  if (context->physicalDevice == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to find a suitable GPU!\n");
    return false;
  }

  // 5. Find Queue Families
  context->graphicsQueueFamilyIndex = UINT32_MAX;
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice,
                                           &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      context->physicalDevice, &queueFamilyCount, queueFamilies.data());

  uint32_t queueFamilyIndex = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice,
                                           queueFamilyIndex,
                                           context->surface, &presentSupport);
      if (presentSupport) {
        context->graphicsQueueFamilyIndex = queueFamilyIndex;
        break;
      }
    }
    queueFamilyIndex++;
  }

  if (context->graphicsQueueFamilyIndex == UINT32_MAX) {
    fprintf(stderr, "Failed to find a suitable graphics queue family!\n");
    return false;
  }

  // 6. Create Logical Device
  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = context->graphicsQueueFamilyIndex;
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures = {};
  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers) {
    deviceCreateInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    deviceCreateInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(context->physicalDevice, &deviceCreateInfo, nullptr,
                     &context->device) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create logical device!\n");
    return false;
  }

  fprintf(stderr, "[Vulkan] Logical device created: %p\n",
          reinterpret_cast<void *>(context->device));
  vkGetDeviceQueue(context->device, context->graphicsQueueFamilyIndex, 0,
                   &context->graphicsQueue);
  fprintf(stderr, "[Vulkan] Graphics queue handle: %p\n",
          reinterpret_cast<void *>(context->graphicsQueue));

  // 7. Create Swapchain (minimal implementation for now)
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice,
                                            context->surface, &capabilities);

  uint32_t formatCount;
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice,
                                           context->surface, &formatCount,
                                           nullptr) != VK_SUCCESS) {
    fprintf(stderr, "Failed to query surface formats!\n");
    return false;
  }
  if (formatCount == 0) {
    fprintf(stderr, "No surface formats available for the Vulkan surface!\n");
    return false;
  }

  std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice,
                                           context->surface, &formatCount,
                                           surfaceFormats.data()) != VK_SUCCESS) {
    fprintf(stderr, "Failed to retrieve surface formats!\n");
    return false;
  }

  VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
  if (surfaceFormat.format == VK_FORMAT_UNDEFINED) {
    surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  } else {
    for (const auto &availableFormat : surfaceFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        surfaceFormat = availableFormat;
        break;
      }
    }
  }
  context->swapChainImageFormat = surfaceFormat.format;

  uint32_t presentModeCount;
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(
          context->physicalDevice, context->surface, &presentModeCount,
          nullptr) != VK_SUCCESS) {
    fprintf(stderr, "Failed to query present modes!\n");
    return false;
  }
  if (presentModeCount == 0) {
    fprintf(stderr, "No present modes available for the Vulkan surface!\n");
    return false;
  }

  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(context->physicalDevice,
                                                context->surface, &presentModeCount,
                                                presentModes.data()) != VK_SUCCESS) {
    fprintf(stderr, "Failed to retrieve present modes!\n");
    return false;
  }

  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (const auto &availablePresentMode : presentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = availablePresentMode;
      break;
    }
  }

  int width, height;
  SDL_GetWindowSizeInPixels(window, &width, &height);
  VkExtent2D extent = {static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height)};

  extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                            capabilities.maxImageExtent.width);
  extent.height = std::clamp(extent.height, capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height);
  context->swapChainExtent = extent;

  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }
  context->imageCount = imageCount;

  VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  if (!(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
    if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
      compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
      compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
      compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }
  }

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
  swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCreateInfo.surface = context->surface;
  swapchainCreateInfo.minImageCount = imageCount;
  swapchainCreateInfo.imageFormat = surfaceFormat.format;
  swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapchainCreateInfo.imageExtent = extent;
  swapchainCreateInfo.imageArrayLayers = 1;
  swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCreateInfo.preTransform = capabilities.currentTransform;
  swapchainCreateInfo.compositeAlpha = compositeAlpha;
  swapchainCreateInfo.presentMode = presentMode;
  swapchainCreateInfo.clipped = VK_TRUE;
  swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(context->device, &swapchainCreateInfo, nullptr,
                           &context->swapchain) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create swap chain!\n");
    return false;
  }

  vkGetSwapchainImagesKHR(context->device, context->swapchain, &imageCount,
                          nullptr);
  context->swapChainImages = (VkImage *)malloc(sizeof(VkImage) * imageCount);
  vkGetSwapchainImagesKHR(context->device, context->swapchain, &imageCount,
                          context->swapChainImages);

  context->swapChainImageViews =
      (VkImageView *)malloc(sizeof(VkImageView) * imageCount);
  for (uint32_t j = 0; j < imageCount; j++) {
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = context->swapChainImages[j];
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = context->swapChainImageFormat;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(context->device, &imageViewCreateInfo, nullptr,
                          &context->swapChainImageViews[j]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create image views!\n");
      return false;
    }
  }

  // 8. Create Render Pass
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = context->swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(context->device, &renderPassInfo, nullptr,
                         &context->renderPass) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create render pass!\n");
    return false;
  }
  fprintf(stderr, "[Vulkan] Render pass created: %p\n",
          reinterpret_cast<void *>(context->renderPass));

  // 9. Create Framebuffers
  context->swapChainFramebuffers =
      (VkFramebuffer *)malloc(sizeof(VkFramebuffer) * imageCount);
  for (uint32_t j = 0; j < imageCount; j++) {
    VkImageView attachments[] = {context->swapChainImageViews[j]};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = context->renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = context->swapChainExtent.width;
    framebufferInfo.height = context->swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(context->device, &framebufferInfo, nullptr,
                            &context->swapChainFramebuffers[j]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create framebuffer!\n");
      return false;
    }
  }

  // 10. Create Command Pool
  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = context->graphicsQueueFamilyIndex;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(context->device, &poolInfo, nullptr,
                          &context->commandPool) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create command pool!\n");
    return false;
  }

  // 11. Create Command Buffers
  context->commandBuffers =
      (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer) * imageCount);
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = context->commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = imageCount;

  if (vkAllocateCommandBuffers(context->device, &allocInfo,
                               context->commandBuffers) != VK_SUCCESS) {
    fprintf(stderr, "Failed to allocate command buffers!\n");
    return false;
  }

  // 12. Create Per-Frame Semaphores and Fences
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  context->currentFrame = 0;
  for (uint32_t i = 0; i < LAYRA_MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(context->device, &semaphoreInfo, nullptr,
                          &context->imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(context->device, &semaphoreInfo, nullptr,
                          &context->renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(context->device, &fenceInfo, nullptr,
                      &context->inFlightFences[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create per-frame sync objects!\n");
      return false;
    }
  }
  context->imageAvailableSemaphore = context->imageAvailableSemaphores[0];
  context->renderFinishedSemaphore = context->renderFinishedSemaphores[0];
  context->inFlightFence = context->inFlightFences[0];

  fprintf(stderr, "[Vulkan] Final init state: device=%p physicalDevice=%p queue=%p queueFamily=%u surface=%p swapchain=%p renderPass=%p imageCount=%u\n",
          reinterpret_cast<void *>(context->device),
          reinterpret_cast<void *>(context->physicalDevice),
          reinterpret_cast<void *>(context->graphicsQueue),
          context->graphicsQueueFamilyIndex,
          reinterpret_cast<void *>(context->surface),
          reinterpret_cast<void *>(context->swapchain),
          reinterpret_cast<void *>(context->renderPass),
          context->imageCount);
  fprintf(stdout, "Vulkan initialized successfully.\n");
  return true;
}

void layra_vulkan_cleanup_swapchain(LayraVulkanContext *context) {
  for (size_t i = 0; i < context->imageCount; i++) {
    vkDestroyFramebuffer(context->device, context->swapChainFramebuffers[i],
                         nullptr);
    vkDestroyImageView(context->device, context->swapChainImageViews[i],
                       nullptr);
  }
  free(context->swapChainFramebuffers);
  free(context->swapChainImageViews);
  free(context->swapChainImages);

  vkDestroySwapchainKHR(context->device, context->swapchain, nullptr);
  vkDestroyRenderPass(context->device, context->renderPass, nullptr);
}

bool layra_vulkan_recreate_swapchain(LayraVulkanContext *context,
                                     SDL_Window *window) {
  vkDeviceWaitIdle(context->device);

  layra_vulkan_cleanup_swapchain(context);

  // Recreate swapchain
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice,
                                            context->surface, &capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice,
                                       context->surface, &formatCount, nullptr);
  std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice,
                                       context->surface, &formatCount,
                                       surfaceFormats.data());

  VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
  if (surfaceFormat.format == VK_FORMAT_UNDEFINED) {
    surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  } else {
    for (const auto &availableFormat : surfaceFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        surfaceFormat = availableFormat;
        break;
      }
    }
  }
  context->swapChainImageFormat = surfaceFormat.format;

  uint32_t presentModeCount;
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(
          context->physicalDevice, context->surface, &presentModeCount,
          nullptr) != VK_SUCCESS) {
    fprintf(stderr, "Failed to query present modes!\n");
    return false;
  }
  if (presentModeCount == 0) {
    fprintf(stderr, "No present modes available for the Vulkan surface!\n");
    return false;
  }

  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(context->physicalDevice,
                                                context->surface, &presentModeCount,
                                                presentModes.data()) != VK_SUCCESS) {
    fprintf(stderr, "Failed to retrieve present modes!\n");
    return false;
  }

  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (const auto &availablePresentMode : presentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = availablePresentMode;
      break;
    }
  }

  int width, height;
  SDL_GetWindowSizeInPixels(window, &width, &height);
  VkExtent2D extent = {static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height)};

  extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                            capabilities.maxImageExtent.width);
  extent.height = std::clamp(extent.height, capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height);
  context->swapChainExtent = extent;

  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }
  uint32_t oldImageCount = context->imageCount;
  context->imageCount = imageCount;

  VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  if (!(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
    if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
      compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
      compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
      compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }
  }

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
  swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCreateInfo.surface = context->surface;
  swapchainCreateInfo.minImageCount = imageCount;
  swapchainCreateInfo.imageFormat = surfaceFormat.format;
  swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapchainCreateInfo.imageExtent = extent;
  swapchainCreateInfo.imageArrayLayers = 1;
  swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCreateInfo.preTransform = capabilities.currentTransform;
  swapchainCreateInfo.compositeAlpha = compositeAlpha;
  swapchainCreateInfo.presentMode = presentMode;
  swapchainCreateInfo.clipped = VK_TRUE;
  swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(context->device, &swapchainCreateInfo, nullptr,
                           &context->swapchain) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create swap chain!\n");
    return false;
  }

  vkGetSwapchainImagesKHR(context->device, context->swapchain, &imageCount,
                          nullptr);
  context->swapChainImages = (VkImage *)malloc(sizeof(VkImage) * imageCount);
  vkGetSwapchainImagesKHR(context->device, context->swapchain, &imageCount,
                          context->swapChainImages);

  context->swapChainImageViews =
      (VkImageView *)malloc(sizeof(VkImageView) * imageCount);
  for (uint32_t j = 0; j < imageCount; j++) {
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = context->swapChainImages[j];
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = context->swapChainImageFormat;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(context->device, &imageViewCreateInfo, nullptr,
                          &context->swapChainImageViews[j]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create image views!\n");
      return false;
    }
  }

  // 8. Create Render Pass (already created, just need to re-assign)
  // VkRenderPassCreateInfo renderPassInfo = {}; // No need to recreate, just
  // ensure it's valid

  // 9. Create Framebuffers
  context->swapChainFramebuffers =
      (VkFramebuffer *)malloc(sizeof(VkFramebuffer) * imageCount);
  for (uint32_t j = 0; j < imageCount; j++) {
    VkImageView attachments[] = {context->swapChainImageViews[j]};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = context->renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = context->swapChainExtent.width;
    framebufferInfo.height = context->swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(context->device, &framebufferInfo, nullptr,
                            &context->swapChainFramebuffers[j]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create framebuffer!\n");
      return false;
    }
  }

  // Reallocate command buffers if imageCount changed
  if (context->commandBuffers) {
    vkFreeCommandBuffers(context->device, context->commandPool,
                         oldImageCount, context->commandBuffers);
    free(context->commandBuffers);
  }
  context->commandBuffers =
      (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer) * imageCount);
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = context->commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = imageCount;

  if (vkAllocateCommandBuffers(context->device, &allocInfo,
                               context->commandBuffers) != VK_SUCCESS) {
    fprintf(stderr, "Failed to allocate command buffers!\n");
    return false;
  }

  return true;
}

void layra_vulkan_render_frame(LayraVulkanContext *context,
                               void (*draw_callback)(VkCommandBuffer)) {
  uint32_t frame = context->currentFrame;

  vkWaitForFences(context->device, 1, &context->inFlightFences[frame],
                  VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      context->device, context->swapchain, UINT64_MAX,
      context->imageAvailableSemaphores[frame], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return;
  } else if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to acquire swap chain image!\n");
    return;
  }

  vkResetFences(context->device, 1, &context->inFlightFences[frame]);

  VkCommandBuffer commandBuffer = context->commandBuffers[imageIndex];
  vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = 0,
      .pInheritanceInfo = NULL,
  };

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    fprintf(stderr, "Failed to begin recording command buffer!\n");
    return;
  }

  VkRenderPassBeginInfo renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = context->renderPass,
      .framebuffer = context->swapChainFramebuffers[imageIndex],
      .renderArea =
          {
              .offset = {0, 0},
              .extent = context->swapChainExtent,
          },
  };

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  if (draw_callback) {
    draw_callback(commandBuffer);
  }

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    fprintf(stderr, "Failed to record command buffer!\n");
    return;
  }

  VkPipelineStageFlags colorAttachmentOutputStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &context->imageAvailableSemaphores[frame],
      .pWaitDstStageMask = &colorAttachmentOutputStage,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &context->renderFinishedSemaphores[frame],
  };

  if (vkQueueSubmit(context->graphicsQueue, 1, &submitInfo,
                    context->inFlightFences[frame]) != VK_SUCCESS) {
    fprintf(stderr, "Failed to submit draw command buffer!\n");
    return;
  }

  VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &context->renderFinishedSemaphores[frame],
      .swapchainCount = 1,
      .pSwapchains = &context->swapchain,
      .pImageIndices = &imageIndex,
      .pResults = NULL,
  };

  result = vkQueuePresentKHR(context->graphicsQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return;
  } else if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to present swap chain image!\n");
    return;
  }

  context->currentFrame = (frame + 1) % LAYRA_MAX_FRAMES_IN_FLIGHT;
}

void layra_vulkan_cleanup(LayraVulkanContext *context) {
  layra_vulkan_cleanup_swapchain(context);

  for (uint32_t i = 0; i < LAYRA_MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(context->device, context->renderFinishedSemaphores[i],
                       nullptr);
    vkDestroySemaphore(context->device, context->imageAvailableSemaphores[i],
                       nullptr);
    vkDestroyFence(context->device, context->inFlightFences[i], nullptr);
  }

  vkFreeCommandBuffers(context->device, context->commandPool,
                       context->imageCount, context->commandBuffers);
  vkDestroyCommandPool(context->device, context->commandPool, nullptr);
  free(context->commandBuffers);

  if (context->device != VK_NULL_HANDLE) {
    vkDestroyDevice(context->device, nullptr);
  }
  if (context->debugMessenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(context->instance, context->debugMessenger,
                                  nullptr);
  }
  if (context->surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(context->instance, context->surface, nullptr);
  }
  if (context->instance != VK_NULL_HANDLE) {
    vkDestroyInstance(context->instance, nullptr);
  }
  fprintf(stdout, "Vulkan cleaned up.\n");
}
