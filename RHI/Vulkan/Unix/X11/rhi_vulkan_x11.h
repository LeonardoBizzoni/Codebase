#ifndef VULKAN_UNIX_X11_H
#define VULKAN_UNIX_X11_H

#include <vulkan/vulkan_xlib.h>

global const char *rhi_vk_needed_extensions[] = {
  VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
  "VK_KHR_surface",
  "VK_KHR_xlib_surface",
};
local const char *rhi_vk_layers[] = {
#if DEBUG
  "VK_LAYER_KHRONOS_validation",
#endif
};

#endif
