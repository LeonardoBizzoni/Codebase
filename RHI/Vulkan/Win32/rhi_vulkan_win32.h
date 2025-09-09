#ifndef RHI_VULKAN_WIN32_H
#define RHI_VULKAN_WIN32_H

#include <vulkan/vulkan_win32.h>

global const char *rhi_vk_needed_extensions[] = {
  VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
  "VK_KHR_surface",
  "VK_KHR_win32_surface",
};
local const char *rhi_vk_layers[] = {
#if DEBUG
  "VK_LAYER_KHRONOS_validation",
#endif
};

#endif
