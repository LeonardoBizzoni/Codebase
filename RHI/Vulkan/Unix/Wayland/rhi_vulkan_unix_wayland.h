#ifndef RHI_VULKAN_UNIX_WAYLAND_H
#define RHI_VULKAN_UNIX_WAYLAND_H

#include <vulkan/vulkan_wayland.h>

local const char *rhi_vk_needed_extensions[] = {
  VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
  "VK_KHR_surface",
  "VK_KHR_wayland_surface",
};
local const char *rhi_vk_layers[] = {
#if DEBUG
  "VK_LAYER_KHRONOS_validation",
#endif
};

#endif
