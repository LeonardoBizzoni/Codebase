#ifndef RHI_VULKAN_CORE_H
#define RHI_VULKAN_CORE_H

#include <vulkan/vulkan.h>

#define rhi_vk_get_array(Arena, Func, BufferType, BufferName, LengthVar, ...) \
do {                                                                          \
  Func(__VA_ARGS__);                                                          \
  BufferName = New(Arena, BufferType, LengthVar);                             \
  Func(__VA_ARGS__);                                                          \
} while (0)

#define rhi_vk_create(Func, ...)        \
do {                                    \
  VkResult _result = Func(__VA_ARGS__); \
  Assert(_result == VK_SUCCESS);        \
} while (0)

#define rhi_vk_renderpass_create(Device, Attachments, Subpasses, Dependencies) \
  _rhi_vk_renderpass_create((Device),                                          \
                            (Attachments), Arrsize(Attachments),               \
                            (Subpasses), Arrsize(Subpasses),                   \
                            (Dependencies), Arrsize(Dependencies))

typedef struct {
  VkShaderModule module;
  RHI_ShaderType type;
} RHI_VK_Shader;

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
    i32 values[3];
    struct {
      i32 graphics;
      i32 present;
      i32 transfer;
    };
  } queue_idx;
} RHI_VK_Device;

typedef struct {
  VkSwapchainKHR swapchain;
  VkSurfaceFormatKHR surface_format;
  VkPresentModeKHR present_mode;
  VkExtent2D extent;
  VkViewport viewport;
  VkRect2D scissor;

  VkImage *images;
  VkImageView *image_views;
  u32 image_count;
} RHI_VK_Swapchain;

typedef struct {
  VkBuffer vkbuffer;
  VkMemoryRequirements memory_requirements;
  u32 memorypool_position;
} RHI_VK_Buffer;

typedef struct {
  u32 offset;
  VkBufferUsageFlags usage;
  RHI_VK_Buffer buffer;
  VkDeviceMemory memory;
} RHI_VK_MemoryPool;

typedef struct {
  Arena *arena;
  VkInstance instance;
} RHI_VK_State;

fn VkSurfaceKHR rhi_vk_surface_create(OS_Handle os_window);
fn void rhi_vk_surface_destroy(VkSurfaceKHR surface);

fn RHI_VK_Device rhi_vk_device_create(VkSurfaceKHR vk_surface);
fn RHI_VK_Device rhi_vk_device_destroy(RHI_VK_Device device);

fn RHI_VK_Swapchain rhi_vk_swapchain_create(Arena *arena,
                                            RHI_VK_Device rhi_device,
                                            VkSurfaceKHR surface,
                                            u32 width, u32 height);
fn VkFramebuffer* rhi_vk_framebuffers_create(Arena *arena,
                                             RHI_VK_Device *rhi_device,
                                             RHI_VK_Swapchain *rhi_swapchain,
                                             VkRenderPass renderpass);

fn RHI_VK_Shader rhi_vk_shader_from_file(RHI_ShaderType type,
                                         RHI_VK_Device rhi_device,
                                         String8 shader_source_path);
fn RHI_VK_Shader rhi_vk_shader_from_bytes(RHI_ShaderType type,
                                          RHI_VK_Device rhi_device,
                                          isize size, u8 *bytes);

internal void rhi_vk_device_properties_print(VkPhysicalDeviceProperties *props);

#endif
