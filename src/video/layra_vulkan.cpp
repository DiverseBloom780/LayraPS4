// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "layravulkan.h"
#include <vector>
#include <string>
#include <set>
#include <algorithm>

#ifdef NDEBUG
const bool enableValidationLayers = false; 
#else
const bool enableValidationLayers = true; 
#endif

const std::vector<const char> validationLayers = {
    "VKLAYERKHRONOSvalidation"
};

const std::vector<const char> deviceExtensions = {
    VKKHRSWAPCHAINEXTENSIONNAME
};

static VKAPIATTR VkBool32 VKAPICALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT pCallbackData,
    void pUserData) {

    fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);

    return VKFALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
 VkInstance instance,
 const VkDebugUtilsMessengerCreateInfoEXT pCreateInfo,
 const VkAllocationCallbacks pAllocator,
 VkDebugUtilsMessengerEXT pDebugMessenger) {
 auto func = (PFNvkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"); 
 if (func != nullptr) {
 return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
 } 
 else {
 return VKERROREXTENSIONNOTPRESENT;
 } 
}

void DestroyDebugUtilsMessengerEXT(
 VkInstance instance,
 VkDebugUtilsMessengerEXT debugMessenger,
 const VkAllocationCallbacks pAllocator) {
 auto func = (PFNvkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
 if (func != nullptr ) {
 func(instance, debugMessenger, pAllocator); 
 }
}

bool checkValidationLayerSupport() {
 uint32t layerCount; 
 vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

 std::vector<VkLayerProperties> availableLayers(layerCount); 
 vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

 for (const char layerName : validationLayers) {
 bool layerFound = false;
 for (const auto& layerProperties : availableLayers) {
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

bool layravulkaninit(LayraVulkanContext context, SDLWindow window) {
    // Initialize all function pointers to null
    memset(context, 0, sizeof(LayraVulkanContext));

 // 1. Create Vulkan Instance
 VkApplicationInfo appInfo = {}; 
 appInfo.sType = VKSTRUCTURETYPEAPPLICATIONINFO;
 appInfo.pApplicationName = "LayraPS4";
 appInfo.applicationVersion = VK MAKEVERSION(1, 0, 0);
 appInfo.pEngineName = "No Engine";
 appInfo.engineVersion = VKMAKEVERSION(1, 0, 0);
 appInfo.apiVersion = VKAPIVERSION10;

 unsigned int extensionscount; 
 if (! SDLVulkanGetInstanceExtensions(window, &extensionscount, nullptr)) {
 fprintf(stderr, "Failed to get instance extensions count.\n");
 return false; 
 }
 std::vector<const char> extensions(extensionscount); 
 if (! SDLVulkanGetInstanceExtensions(window, &extensionscount, extensions.data())) {
 fprintf(stderr, "Failed to get instance extensions.\n");
 return false; 
 }

    if (enableValidationLayers) {
        extensions.pushback(VKEXTDEBUGUTILSEXTENSIONNAME);
    }

 VkInstanceCreateInfo createInfo = {}; 
 createInfo.sType = VKSTRUCTURETYPEINSTANCECREATEINFO; 
 createInfo.pApplicationInfo = &appInfo; 
 createInfo.enabledExtensionCount = staticcast<uint32t>(extensions.size()); 
 createInfo.ppEnabledExtensionNames = extensions.data();

 if (enableValidationLayers) {
 if (!checkValidationLayerSupport()) {
 fprintf(stderr, "Validation layers requested, but not available!\n");
 return false; 
 }
 createInfo.enabledLayerCount = staticcast<uint32t>(validationLayers.size()); 
 createInfo.ppEnabledLayerNames = validationLayers.data();

 VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {}; 
 debugCreateInfo.sType = VKSTRUCTURETYPEDEBUGUTILSMESSENGERCREATEINFOEXT; 
 debugCreateInfo.messageSeverity = VKDEBUGUTILSMESSAGESEVERITYVERBOSEBITEXT 

VKDEBUGUTILSMESSAGESEVERITYWARNINGBITEXT
 VKDEBUGUTILSMESSAGESEVERITYERRORBITEXT;
 debugCreateInfo.messageType = VKDEBUGUTILSMESSAGETYPE GENERALBITEXT | VKDEBUGUTILSMESSAGETYPEVALIDATION
