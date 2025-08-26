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
  const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation",
  };

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

  u32 physical_device_count = 0;
  vkEnumeratePhysicalDevices(wayvk_state.instance, &physical_device_count, 0);
  VkPhysicalDevice *physical_devices = New(wayvk_state.arena, VkPhysicalDevice,
                                           physical_device_count);
  vkEnumeratePhysicalDevices(wayvk_state.instance, &physical_device_count,
                             physical_devices);

  VkPhysicalDevice *best_physical_device = &physical_devices[0];
  for (u32 i = 0, best_score = 0;
       wayvk_state.rate_device && i < physical_device_count;
       ++i) {
    Scratch scratch = ScratchBegin(0, 0);
    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(physical_devices[i], &props);
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
                                             &queue_family_count, 0);
    VkQueueFamilyProperties *queue_families = New(wayvk_state.arena,
                                                  VkQueueFamilyProperties,
                                                  queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
                                             &queue_family_count,
                                             queue_families);

    u32 score = wayvk_state.rate_device(&props, &features, queue_families,
                                        queue_family_count);
    ScratchEnd(scratch);
    if (score > best_score) {
      score = best_score;
      best_physical_device = &physical_devices[i];
    }
  }
}

fn void os_gfx_deinit(void) {
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

  PFN_vkCreateWaylandSurfaceKHR fpCreateWaylandSurface =
    (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(wayvk_state.instance,
                                                         "vkCreateWaylandSurfaceKHR");
  Assert(fpCreateWaylandSurface);
  Assert(fpCreateWaylandSurface(wayvk_state.instance, &surface_create_info, 0,
                                &gfx_window->vk_surface) == VK_SUCCESS);

  GFX_Handle res = {{(u64)gfx_window}};
  return res;
}

fn void os_gfx_context_window_deinit(GFX_Handle handle) {
}

fn void os_gfx_window_make_current(GFX_Handle handle) {
}

fn void os_gfx_window_commit(GFX_Handle handle) {
}

internal void os_gfx_window_resize(GFX_Handle context, u32 width, u32 height) {
}
