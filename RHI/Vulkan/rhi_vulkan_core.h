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

#define rhi_vk_fence_wait(Device, ...) \
  _rhi_vk_fence_wait((Device), ((VkFence[]) {__VA_ARGS__}),\
                     Arrsize(((VkFence[]) {__VA_ARGS__})))

#define rhi_vk_fence_wait_at_least_ms(Device, Milliseconds, ...) \
  _rhi_vk_fence_wait_at_least_ms((Device), (Milliseconds),       \
                                 ((VkFence[]) {__VA_ARGS__}),    \
                                 Arrsize(((VkFence[]) {__VA_ARGS__})))

#define rhi_vk_fence_reset(Device, ...) \
  _rhi_vk_fence_reset((Device), ((VkFence[]) {__VA_ARGS__}),\
                      Arrsize(((VkFence[]) {__VA_ARGS__})))

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

fn RHI_VK_Swapchain
rhi_vk_swapchain_create(Arena *arena, RHI_VK_Device rhi_device,
                        VkSurfaceKHR surface, u32 width, u32 height);
fn void
rhi_vk_swapchain_destroy(RHI_VK_Device *rhi_device,
                         RHI_VK_Swapchain *rhi_swapchain);

fn VkFramebuffer*
rhi_vk_framebuffers_create(Arena *arena, RHI_VK_Device *rhi_device,
                           RHI_VK_Swapchain *rhi_swapchain,
                           VkRenderPass renderpass);
fn void
rhi_vk_framebuffers_destroy(RHI_VK_Device *rhi_device,
                            RHI_VK_Swapchain *rhi_swapchain,
                            VkFramebuffer *framebuffers);

fn VkSemaphore*
rhi_vk_semaphore_create(Arena *arena, RHI_VK_Device device, i32 count,
                        VkSemaphoreCreateFlags flags);

fn VkFence*
rhi_vk_fence_create(Arena *arena, RHI_VK_Device device, i32 count,
                    VkFenceCreateFlags flags);

fn RHI_VK_Buffer
rhi_vk_buffer_create(RHI_VK_Device device, u32 size,
                     VkBufferUsageFlags usage);
fn void
rhi_vk_buffer_destroy(RHI_VK_Device device, RHI_VK_Buffer buffer);
fn void
rhi_vk_buffer_copy(RHI_VK_Device device, RHI_VK_Buffer dest, RHI_VK_Buffer src,
                   VkCommandPool cmdpool, VkBufferCopy *buffer_copy_regions,
                   u32 buffer_copy_region_count);

fn RHI_VK_MemoryPool
rhi_vk_memorypool_build(RHI_VK_Device device, u32 size,
                        VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags wanted_properties);
fn RHI_VK_Buffer
rhi_vk_memorypool_push(RHI_VK_Device device, RHI_VK_MemoryPool *pool,
                       u32 size);
fn void*
rhi_vk_memorypool_map(RHI_VK_Device device, RHI_VK_MemoryPool *pool,
                      RHI_VK_Buffer buffer);
fn void
rhi_vk_memorypool_destroy(RHI_VK_Device device, RHI_VK_MemoryPool pool);

fn RHI_VK_Shader
rhi_vk_shader_from_file(RHI_ShaderType type, RHI_VK_Device rhi_device,
                        String8 shader_source_path);
fn RHI_VK_Shader
rhi_vk_shader_from_bytes(RHI_ShaderType type, RHI_VK_Device rhi_device,
                         isize size, u8 *bytes);


internal VkDeviceMemory
rhi_vk_alloc(RHI_VK_Device device, VkMemoryRequirements memory_requirements,
             VkMemoryPropertyFlags wanted_properties);

internal VkRenderPass
_rhi_vk_renderpass_create(RHI_VK_Device device,
                          VkAttachmentDescription *attachments,
                          u32 attachment_count,
                          VkSubpassDescription *subpasses, u32 subpass_count,
                          VkSubpassDependency *dependencies,
                          u32 dependency_count);

internal void
_rhi_vk_fence_wait(RHI_VK_Device device, VkFence *fences, u32 count);
internal void
_rhi_vk_fence_wait_at_least_ms(RHI_VK_Device device, u64 milliseconds,
                               VkFence *fences, u32 count);
internal void
_rhi_vk_fence_reset(RHI_VK_Device device, VkFence *fences, u32 count);

internal void
rhi_vk_device_properties_print(VkPhysicalDeviceProperties *props);

#endif
