global WayVk_State wayvk_state = {};
global PFN_vkCreateWaylandSurfaceKHR wl_vk_surface_create = 0;

#if DEBUG
const char *validation_layers[] = {
  "VK_LAYER_KHRONOS_validation",
};
#endif

fn void os_gfx_init(void) {
  wayvk_state.arena = ArenaBuild();

  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .engineVersion = VK_MAKE_VERSION(1,0,0),
    .apiVersion = VK_API_VERSION_1_0,
  };

  const char* extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
  };

  VkInstanceCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
    .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
    .enabledExtensionCount = Arrsize(extensions),
    .ppEnabledExtensionNames = extensions,
  };

#if DEBUG
  u32 layer_count = 0;
  vkEnumerateInstanceLayerProperties(&layer_count, 0);
  VkLayerProperties *layers = New(wayvk_state.arena, VkLayerProperties,
                                  layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, layers);

  for (usize i = 0; i < Arrsize(validation_layers); ++i) {
    bool layer_found = false;
    for (usize j = 0; j < layer_count; ++j) {
      if (cstr_eq(validation_layers[i], layers[j].layerName)) {
        layer_found = true;
        break;
      }
    }
    Assert(layer_found);
  }

  create_info.enabledLayerCount = Arrsize(validation_layers);
  create_info.ppEnabledLayerNames = validation_layers;
#endif

  VkResult create_instance_result = vkCreateInstance(&create_info, 0,
                                                     &wayvk_state.instance);
  Assert(create_instance_result == VK_SUCCESS);

  wl_vk_surface_create =
    (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(wayvk_state.instance,
                                                         "vkCreateWaylandSurfaceKHR");
  Assert(wl_vk_surface_create);

  u32 physical_device_count = 0;
  vkEnumeratePhysicalDevices(wayvk_state.instance, &physical_device_count, 0);
  VkPhysicalDevice *physical_devices = New(wayvk_state.arena, VkPhysicalDevice,
                                           physical_device_count);
  vkEnumeratePhysicalDevices(wayvk_state.instance, &physical_device_count,
                             physical_devices);

  wayvk_state.physical_device = &physical_devices[0];
  if (os_vk_rate_device) {
    Scratch scratch = ScratchBegin(0, 0);
    VkQueueFamilyProperties *tmp = 0;
    i32 best_score = -1;
    for (u32 i = 0; i < physical_device_count; ++i) {
      VkPhysicalDeviceProperties props = {};
      vkGetPhysicalDeviceProperties(physical_devices[i], &props);
      VkPhysicalDeviceFeatures features = {};
      vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

      u32 device_queue_family_count = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
                                               &device_queue_family_count, 0);
      VkQueueFamilyProperties *device_queue_families = New(scratch.arena,
                                                           VkQueueFamilyProperties,
                                                           device_queue_family_count);
      vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
                                               &device_queue_family_count,
                                               device_queue_families);

      i32 score = os_vk_rate_device(&props, &features, device_queue_families,
                                    device_queue_family_count);
      if (score > best_score) {
        best_score = score;
        tmp = device_queue_families;
        wayvk_state.physical_device = &physical_devices[i];
        wayvk_state.physical_device_queue_family_count = device_queue_family_count;
      }
    }
    Assert(tmp);
    wayvk_state.physical_device_queue_family = New(wayvk_state.arena,
                                                   VkQueueFamilyProperties,
                                                   wayvk_state.physical_device_queue_family_count);
    memcopy(wayvk_state.physical_device_queue_family, tmp,
            sizeof (*tmp) * wayvk_state.physical_device_queue_family_count);
    ScratchEnd(scratch);
  } else {
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[0],
                                             &wayvk_state.physical_device_queue_family_count, 0);
    wayvk_state.physical_device_queue_family = New(wayvk_state.arena,
                                                   VkQueueFamilyProperties,
                                                   wayvk_state.physical_device_queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[0],
                                             &wayvk_state.physical_device_queue_family_count,
                                             wayvk_state.physical_device_queue_family);
  }

  VkSurfaceKHR dummy_surface = {};
  VkWaylandSurfaceCreateInfoKHR surface_create_info = {
    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
    .display = waystate.wl_display,
    .surface = wl_compositor_create_surface(waystate.wl_compositor),
  };
  VkResult surface_create_result = wl_vk_surface_create(wayvk_state.instance,
                                                        &surface_create_info, 0,
                                                        &dummy_surface);
  Assert(surface_create_result == VK_SUCCESS);

  const char *device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, };
  u32 device_extension_count = 0;
  {
    Scratch scratch = ScratchBegin(0, 0);
    vkEnumerateDeviceExtensionProperties(*wayvk_state.physical_device, 0,
                                         &device_extension_count, 0);
    VkExtensionProperties *device_extensions_available = New(scratch.arena,
                                                             VkExtensionProperties,
                                                             device_extension_count);
    vkEnumerateDeviceExtensionProperties(*wayvk_state.physical_device, 0,
                                         &device_extension_count, device_extensions_available);
    for (usize i = 0; i < Arrsize(device_extensions); ++i) {
      bool extension_found = false;
      for (usize j = 0; j < device_extension_count; ++j) {
        if (cstr_eq(device_extensions[i],
                    device_extensions_available[j].extensionName)) {
          extension_found = true;
          break;
        }
      }
      Assert(extension_found);
    }
    ScratchEnd(scratch);
  }

  u32 graphics_family_queue_idx = -1;
  for (u32 i = 0; i < wayvk_state.physical_device_queue_family_count; ++i) {
    VkBool32 present_support = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(*wayvk_state.physical_device, i,
                                         dummy_surface, &present_support);

    if (present_support &&
        (wayvk_state.physical_device_queue_family[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      graphics_family_queue_idx = i;
      break;
    }
  }
  Assert(graphics_family_queue_idx != (u32)-1);

  f32 queue_priority = 1.f;
  VkDeviceQueueCreateInfo queue_create_info = {};
  queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueFamilyIndex = graphics_family_queue_idx;
  queue_create_info.queueCount = 1;
  queue_create_info.pQueuePriorities = &queue_priority;

  VkPhysicalDeviceFeatures device_used_features = {};

  VkDeviceCreateInfo device_create_info = {};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pQueueCreateInfos = &queue_create_info;
  device_create_info.queueCreateInfoCount = 1;
  device_create_info.pEnabledFeatures = &device_used_features;
  device_create_info.enabledExtensionCount = Arrsize(device_extensions);
  device_create_info.ppEnabledExtensionNames = device_extensions;
#if DEBUG
  device_create_info.enabledLayerCount = Arrsize(validation_layers);
  device_create_info.ppEnabledLayerNames = validation_layers;
#else
  device_create_info.enabledLayerCount = 0;
#endif

  VkResult create_device_result = vkCreateDevice(*wayvk_state.physical_device,
                                                 &device_create_info, 0,
                                                 &wayvk_state.device);
  Assert(create_device_result == VK_SUCCESS);

  vkGetDeviceQueue(wayvk_state.device, graphics_family_queue_idx, 0,
                   &wayvk_state.graphics_queue);

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*wayvk_state.physical_device,
                                            dummy_surface,
                                            &wayvk_state.swapchain.surface_capabilities);

  {
    Scratch scratch = ScratchBegin(0, 0);
    u32 swapchain_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*wayvk_state.physical_device,
                                         dummy_surface,
                                         &swapchain_format_count, 0);
    Assert(swapchain_format_count);
    VkSurfaceFormatKHR *swapchain_formats = New(scratch.arena,
                                                VkSurfaceFormatKHR,
                                                swapchain_format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(*wayvk_state.physical_device,
                                         dummy_surface,
                                         &swapchain_format_count,
                                         swapchain_formats);
    for (u32 i = 0; i < swapchain_format_count; ++i) {
      if (swapchain_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
          swapchain_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        memcopy(&wayvk_state.swapchain.surface_format,
                &swapchain_formats[i], sizeof (*swapchain_formats));
      }
    }
    if (!wayvk_state.swapchain.surface_format.format) {
      memcopy(&wayvk_state.swapchain.surface_format,
              &swapchain_formats[0], sizeof (*swapchain_formats));
    }

    u32 swapchain_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(*wayvk_state.physical_device,
                                              dummy_surface,
                                              &swapchain_mode_count, 0);
    Assert(swapchain_mode_count);
    VkPresentModeKHR *swapchain_modes = New(wayvk_state.arena, VkPresentModeKHR,
                                            swapchain_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(*wayvk_state.physical_device,
                                              dummy_surface,
                                              &swapchain_mode_count,
                                              swapchain_modes);
    for (u32 i = 0; i < swapchain_format_count; ++i) {
      if (swapchain_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        wayvk_state.swapchain.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      }
    }
    if (!wayvk_state.swapchain.present_mode) {
      wayvk_state.swapchain.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }
    ScratchEnd(scratch);
  }

  vkDestroySurfaceKHR(wayvk_state.instance, dummy_surface, 0);
  wl_surface_destroy(surface_create_info.surface);
}

fn void os_gfx_deinit(void) {
  vkDestroyDevice(wayvk_state.device, 0);
  vkDestroyInstance(wayvk_state.instance, 0);
}

fn GFX_Handle os_gfx_context_window_init(OS_Handle window) {
  Wl_Window *os_window = (Wl_Window*)window.h[0];
  WayVk_Window *gfx_window = wayvk_state.freequeue_first;
  if (gfx_window) {
    memzero(gfx_window, sizeof *gfx_window);
    QueuePop(wayvk_state.freequeue_first);
  } else {
    gfx_window = New(wayvk_state.arena, WayVk_Window);
  }
  gfx_window->os_window = os_window;

  VkWaylandSurfaceCreateInfoKHR surface_create_info = {
    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
    .display = waystate.wl_display,
    .surface = os_window->wl_surface,
  };

  VkResult surface_create_result = wl_vk_surface_create(wayvk_state.instance,
                                                        &surface_create_info, 0,
                                                        &gfx_window->vk_surface);
  Assert(surface_create_result == VK_SUCCESS);

  u32 min_image_count = wayvk_state.swapchain.surface_capabilities.minImageCount + 1;
  if (wayvk_state.swapchain.surface_capabilities.maxImageCount > 0) {
    min_image_count = ClampTop(min_image_count,
                               wayvk_state.swapchain.surface_capabilities.maxImageCount);
  }

  if (wayvk_state.swapchain.surface_capabilities.currentExtent.width != U32_MAX) {
    gfx_window->vk_extent.width = wayvk_state.swapchain.surface_capabilities.currentExtent.width;
    gfx_window->vk_extent.height = wayvk_state.swapchain.surface_capabilities.currentExtent.height;
  } else {
    gfx_window->vk_extent.width = Clamp(os_window->width,
                                        wayvk_state.swapchain.surface_capabilities.minImageExtent.width,
                                        wayvk_state.swapchain.surface_capabilities.maxImageExtent.width);
    gfx_window->vk_extent.height = Clamp(os_window->height,
                                         wayvk_state.swapchain.surface_capabilities.minImageExtent.height,
                                         wayvk_state.swapchain.surface_capabilities.maxImageExtent.height);
  }

  VkSwapchainCreateInfoKHR swapchain_create_info = {};
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.surface = gfx_window->vk_surface;
  swapchain_create_info.minImageCount = min_image_count;
  swapchain_create_info.imageFormat = wayvk_state.swapchain.surface_format.format;
  swapchain_create_info.imageColorSpace = wayvk_state.swapchain.surface_format.colorSpace;
  swapchain_create_info.imageExtent = gfx_window->vk_extent;
  swapchain_create_info.imageArrayLayers = 1;
  swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchain_create_info.preTransform = wayvk_state.swapchain.surface_capabilities.currentTransform;
  swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_create_info.presentMode = wayvk_state.swapchain.present_mode;
  swapchain_create_info.clipped = VK_TRUE;
  swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
  VkResult swapchain_create_result = vkCreateSwapchainKHR(wayvk_state.device,
                                                          &swapchain_create_info, 0,
                                                          &gfx_window->vk_swapchain);
  Assert(swapchain_create_result == VK_SUCCESS);

  vkGetSwapchainImagesKHR(wayvk_state.device, gfx_window->vk_swapchain,
                          &gfx_window->vk_image_count, 0);
  Assert(gfx_window->vk_image_count);
  gfx_window->vk_images = New(wayvk_state.arena, VkImage,
                              gfx_window->vk_image_count);
  gfx_window->vk_imageviews = New(wayvk_state.arena, VkImageView,
                                  gfx_window->vk_image_count);
  vkGetSwapchainImagesKHR(wayvk_state.device, gfx_window->vk_swapchain,
                          &gfx_window->vk_image_count, gfx_window->vk_images);
  for (u32 i = 0; i < gfx_window->vk_image_count; ++i) {
    VkImageViewCreateInfo imageview_create_info = {};
    imageview_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageview_create_info.image = gfx_window->vk_images[i];
    imageview_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageview_create_info.format = wayvk_state.swapchain.surface_format.format;
    imageview_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageview_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageview_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageview_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageview_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageview_create_info.subresourceRange.baseMipLevel = 0;
    imageview_create_info.subresourceRange.levelCount = 1;
    imageview_create_info.subresourceRange.baseArrayLayer = 0;
    imageview_create_info.subresourceRange.layerCount = 1;

    VkResult create_imageview_result = vkCreateImageView(wayvk_state.device,
                                                         &imageview_create_info, 0,
                                                         &gfx_window->vk_imageviews[i]);
    Assert(create_imageview_result == VK_SUCCESS);
  }

  GFX_Handle res = {{(u64)gfx_window}};
  return res;
}

fn void os_gfx_context_window_deinit(GFX_Handle handle) {
  WayVk_Window *gfx_window = (WayVk_Window*)handle.h[0];
  for (u32 i = 0; i < gfx_window->vk_image_count; ++i) {
    vkDestroyImageView(wayvk_state.device, gfx_window->vk_imageviews[i], 0);
  }
  vkDestroySwapchainKHR(wayvk_state.device, gfx_window->vk_swapchain, 0);
  vkDestroySurfaceKHR(wayvk_state.instance, gfx_window->vk_surface, 0);
}

fn void os_gfx_window_make_current(GFX_Handle handle) {
}

fn void os_gfx_window_commit(GFX_Handle handle) {
}

internal void os_gfx_window_resize(GFX_Handle context, u32 width, u32 height) {
}
