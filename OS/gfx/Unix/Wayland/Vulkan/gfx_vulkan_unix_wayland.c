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

  u32 graphics_family_queue_idx = -1;
  for (u32 i = 0; i < wayvk_state.physical_device_queue_family_count; ++i) {
    VkBool32 present_support = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(*wayvk_state.physical_device, i,
                                         dummy_surface, &present_support);
    // NOTE(lb): allow using 2 different queue families?
    if (present_support &&
        (wayvk_state.physical_device_queue_family[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      graphics_family_queue_idx = i;
      break;
    }
  }
  Assert(graphics_family_queue_idx != (u32)-1);
  vkDestroySurfaceKHR(wayvk_state.instance, dummy_surface, 0);
  wl_surface_destroy(surface_create_info.surface);

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

  GFX_Handle res = {{(u64)gfx_window}};
  return res;
}

fn void os_gfx_context_window_deinit(GFX_Handle handle) {
  WayVk_Window *gfx_window = (WayVk_Window*)handle.h[0];
  vkDestroySurfaceKHR(wayvk_state.instance, gfx_window->vk_surface, 0);
}

fn void os_gfx_window_make_current(GFX_Handle handle) {
}

fn void os_gfx_window_commit(GFX_Handle handle) {
}

internal void os_gfx_window_resize(GFX_Handle context, u32 width, u32 height) {
}
