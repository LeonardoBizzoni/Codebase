#ifndef RHI_VULKAN_CORE_H
#define RHI_VULKAN_CORE_H

#include <vulkan/vulkan.h>

typedef struct {
  VkInstance instance;
  VkPresentModeKHR   swapchain_present_mode;
  VkSurfaceFormatKHR swapchain_surface_format;
} RHI_Vulkan_State;

typedef struct {
  VkDevice virtual;
  VkPhysicalDevice physical;
  VkPhysicalDeviceMemoryProperties memory_properties;
  union {
    VkQueue values[3];
    struct {
      VkQueue graphics;
      VkQueue present;
      VkQueue transfer;
    };
  } queue;
  union {
    u32 values[3];
    struct {
      u32 present;
      u32 graphics;
      u32 transfer;
    };
  } queue_idx;
} RHI_Vulkan_Device;

typedef struct {
  VkCommandPool graphics;
  VkCommandPool transfer;
} RHI_Vulkan_CmdPool;

typedef struct {
  struct {
    VkCommandBuffer handles[2];
    i32 submitted_count;
  } graphics;
  struct {
    VkCommandBuffer handles[2];
    i32 submitted_count;
  } transfer;
} RHI_Vulkan_CmdBuffers;

typedef struct {
  OS_Handle hwindow;

  VkSurfaceKHR surface;
  RHI_Vulkan_Device device;

  RHI_Vulkan_CmdPool *pools;
  RHI_Vulkan_CmdBuffers *cmdbuffs;

  VkFence *images_in_flight;
  VkFence  fences_in_flight[2];
  VkSemaphore semaphores_image_available[2];
  VkSemaphore semaphores_render_finished[2];
  VkSemaphore *semaphores_image_finished;

  struct {
    VkImage *images;
    VkImageView *imageviews;
    VkSwapchainKHR handle;
    VkExtent2D extent;
    VkViewport viewport;
    VkRect2D scissor;
    u32 image_count;
    i32 image_width;
    i32 image_height;
  } swapchain;

  VkClearValue clear_value;

  u32 frame_current;
  u32 frame_current_image;
} RHI_Vulkan_Context;

typedef struct {
  union {
    VkPipelineShaderStageCreateInfo values[2];
    struct {
      VkPipelineShaderStageCreateInfo vertex;
      VkPipelineShaderStageCreateInfo pixel;
    };
  };
  i32 count;
} RHI_Vulkan_Shader;

typedef struct {
  VkBuffer handle;
  VkDeviceMemory memory;
} RHI_Vulkan_Buffer;

typedef struct {
  VkCommandBuffer handle;
  VkQueue queue;
  VkCommandPool pool;
} RHI_Vulkan_CommandBuffer;

typedef struct {
  VkSwapchainKHR swapchain;
  VkSurfaceFormatKHR surface_format;
  VkPresentModeKHR present_mode;
  VkExtent2D extent;
  VkViewport viewport;
  VkRect2D scissor;
  VkImage *images;
  VkImageView *image_views;
  i32 image_count;
} RHI_Vulkan_Swapchain;

typedef struct {
  VkPipeline handle;
  VkPipelineLayout layout;
} RHI_Vulkan_Pipeline;

// NOTE(lb): there isn't a format for matrices, they map to N vectors.
global const VkFormat rhi_vulkan_shadertype_map_format[] = {
  [RHI_ShaderDataType_Float]   = VK_FORMAT_R32_SFLOAT,
  [RHI_ShaderDataType_Int]     = VK_FORMAT_R32_SINT,
  [RHI_ShaderDataType_Bool]    = VK_FORMAT_R8_UINT,
  [RHI_ShaderDataType_Vec2F32] = VK_FORMAT_R32G32_SFLOAT,
  [RHI_ShaderDataType_Vec2I32] = VK_FORMAT_R32G32_SINT,
  [RHI_ShaderDataType_Vec3F32] = VK_FORMAT_R32G32B32_SFLOAT,
  [RHI_ShaderDataType_Vec3I32] = VK_FORMAT_R32G32B32_SINT,
  [RHI_ShaderDataType_Vec4F32] = VK_FORMAT_R32G32B32A32_SFLOAT,
  [RHI_ShaderDataType_Vec4I32] = VK_FORMAT_R32G32B32A32_SINT,
  [RHI_ShaderDataType_Mat3F32] = VK_FORMAT_UNDEFINED,
  [RHI_ShaderDataType_Mat4F32] = VK_FORMAT_UNDEFINED,
};

// Implemented per platform
internal VkSurfaceKHR rhi_vulkan_surface_create(OS_Handle os_window);
internal void rhi_vulkan_surface_destroy(VkSurfaceKHR surface);

// Platform independent
internal bool rhi_vulkan_device_init(RHI_Vulkan_Context *context);
internal void rhi_vulkan_device_print(RHI_Vulkan_Device *device);

internal void rhi_vulkan_swapchain_init(Arena *arena, RHI_Vulkan_Context *context, OS_Handle hwindow);
internal void rhi_vulkan_swapchain_create(Arena *arena, RHI_Vulkan_Context *context, i32 width, i32 height);
internal void rhi_vulkan_swapchain_destroy(RHI_Vulkan_Context *context);

#endif
