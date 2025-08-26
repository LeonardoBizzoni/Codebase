#ifndef GFX_OPENGL_UNIX_WAYLAND_H
#define GFX_OPENGL_UNIX_WAYLAND_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

typedef struct WayVk_Window {
  Wl_Window *os_window;

  VkSurfaceKHR vk_surface;

  struct WayVk_Window *next;
  struct WayVk_Window *prev;
} WayVk_Window;

typedef struct {
  Arena *arena;
  WayVk_Window *freequeue_first;
  WayVk_Window *freequeue_last;

  u32 (*rate_device)(VkPhysicalDeviceProperties *,
                     VkPhysicalDeviceFeatures *,
                     VkQueueFamilyProperties *, u32 family_count);

  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
} WayVk_State;
global WayVk_State wayvk_state = {};

#endif
