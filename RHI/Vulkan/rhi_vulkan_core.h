#ifndef RHI_VULKAN_CORE_H
#define RHI_VULKAN_CORE_H

#include <vulkan/vulkan.h>

#define rhi_vk_get_array(Arena, Func, BufferType, BufferName, LengthVar, ...) \
do {                                                                          \
  Func(__VA_ARGS__);                                                          \
  BufferName = New(Arena, BufferType, LengthVar);                             \
  Func(__VA_ARGS__);                                                          \
} while (0)

typedef u8 RHI_ShaderType;
enum {
  RHI_ShaderType_Vertex,
  RHI_ShaderType_Fragment,
};

typedef struct {
  VkDevice virtual;
  VkPhysicalDevice physical;
  union {
    VkQueue values[2];
    struct {
      VkQueue graphics;
      VkQueue present;
    };
  } queue;
  union {
    i32 values[2];
    struct {
      i32 graphics;
      i32 present;
    };
  } queue_idx;
} RHI_VkDevice;

typedef struct {
  VkSwapchainKHR swapchain;
  VkSurfaceFormatKHR surface_format;
  VkPresentModeKHR present_mode;
  VkExtent2D extent;
  VkViewport viewport;
  VkRect2D scissor;

  VkImage *images;
  VkImageView *image_views;
  VkFramebuffer *framebuffers;
  u32 image_count;
} RHI_VkSwapchain;

typedef struct {
  VkShaderModule module;
  RHI_ShaderType type;
} RHI_VkShader;

typedef struct {
  Arena *arena;
  VkInstance instance;
} RHI_VkState;

fn VkSurfaceKHR rhi_vk_surface_create(OS_Handle os_window);
fn void rhi_vk_surface_destroy(VkSurfaceKHR surface);

fn RHI_VkDevice rhi_vk_device_create(VkSurfaceKHR vk_surface);
fn RHI_VkDevice rhi_vk_device_destroy(RHI_VkDevice device);

internal void rhi_vk_device_properties_print(VkPhysicalDeviceProperties *props);

#endif
