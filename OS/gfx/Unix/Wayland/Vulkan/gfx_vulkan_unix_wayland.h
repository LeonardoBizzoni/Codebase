#ifndef GFX_OPENGL_UNIX_WAYLAND_H
#define GFX_OPENGL_UNIX_WAYLAND_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

__attribute__((weak)) i32 os_vk_rate_device(VkPhysicalDeviceProperties *props,
                                            VkPhysicalDeviceFeatures *features,
                                            VkQueueFamilyProperties *queue_family,
                                            u32 family_count);

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

  VkInstance instance;
  VkPhysicalDevice *physical_device;
  u32 physical_device_queue_family_count;
  VkQueueFamilyProperties *physical_device_queue_family;

  VkDevice device;
  VkQueue graphics_queue;
} WayVk_State;

#endif
