global RHI_VK_State rhi_vk_state = {0};

fn void rhi_init(void) {
  rhi_vk_state.arena = arena_build();

  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .engineVersion = VK_MAKE_VERSION(1,0,0),
    .apiVersion = VK_API_VERSION_1_0,
  };
  VkInstanceCreateInfo create_instance_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
    .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
    .enabledExtensionCount = Arrsize(rhi_vk_needed_extensions),
    .ppEnabledExtensionNames = rhi_vk_needed_extensions,
#if DEBUG
    .enabledLayerCount = Arrsize(rhi_vk_layers),
    .ppEnabledLayerNames = rhi_vk_layers,
#else
    .enabledLayerCount = 0,
#endif
  };

  {
#if DEBUG
    Scratch scratch = ScratchBegin(0, 0);
    u32 retrieved_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&retrieved_layer_count, 0);
    VkLayerProperties *retrieved_layers = arena_push_many(scratch.arena,
                                                          VkLayerProperties,
                                                          retrieved_layer_count);
    vkEnumerateInstanceLayerProperties(&retrieved_layer_count, retrieved_layers);
    for (usize i = 0; i < Arrsize(rhi_vk_layers); ++i) {
      bool layer_found = false;
      for (usize j = 0; j < retrieved_layer_count; ++j) {
        if (cstr_eq(rhi_vk_layers[i], retrieved_layers[j].layerName)) {
          layer_found = true;
          break;
        }
      }
      Assert(layer_found);
    }
    ScratchEnd(scratch);
#endif
  }

  VkResult create_instance_result = vkCreateInstance(&create_instance_info,
                                                     0, &rhi_vk_state.instance);
  Assert(create_instance_result == VK_SUCCESS);
}

fn void rhi_deinit(void) {}

fn RHI_VK_Device rhi_vk_device_create(VkSurfaceKHR vk_surface) {
  RHI_VK_Device res = {0};

  local const char *device_extensions_required[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  Scratch scratch_physical_device = ScratchBegin(0, 0);
  u32 phydevices_len = 0;
  VkPhysicalDevice *phydevices = 0;
  rhi_vk_get_array(scratch_physical_device.arena,
                   vkEnumeratePhysicalDevices, VkPhysicalDevice,
                   phydevices, phydevices_len,
                   rhi_vk_state.instance, &phydevices_len, phydevices);
  Assert(phydevices && phydevices_len > 0);

  struct PhysicalDeviceRating {
    i32 rating;
    VkPhysicalDevice *device;
    struct PhysicalDeviceRating *next;
    struct PhysicalDeviceRating *prev;
  };
  struct PhysicalDeviceRating *phydevices_orderedlist_buffer =
    arena_push_many(scratch_physical_device.arena,
                    struct PhysicalDeviceRating,
                    phydevices_len);
  struct PhysicalDeviceRating *phydevices_orderedlist_first = 0;
  struct PhysicalDeviceRating *phydevices_orderedlist_last = 0;

  // TODO(lb): test this on a system that has multiple GPUs where one is
  //           clearly better then the other
  for (u32 i = 0; i < phydevices_len; ++i) {
    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(phydevices[i], &props);
    VkPhysicalDeviceFeatures features = {0};
    vkGetPhysicalDeviceFeatures(phydevices[i], &features);

#if DEBUG
    rhi_vk_device_properties_print(&props);
#endif

    struct PhysicalDeviceRating *new_node = &phydevices_orderedlist_buffer[i];
    new_node->device = &phydevices[i];
    new_node->rating += props.limits.maxImageDimension2D;
    new_node->rating += props.limits.maxComputeSharedMemorySize;
    switch (props.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
      new_node->rating += 1000;
    } break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
      new_node->rating += 500;
    } break;
    default: break;
    }

    bool new_node_inserted = false;
    for (struct PhysicalDeviceRating *curr = phydevices_orderedlist_first;
         curr; curr = curr->next) {
      if (curr->rating < new_node->rating) {
        new_node_inserted = true;
        new_node->prev = curr;
        new_node->next = curr->next;
        curr->next = new_node;
        if (new_node->next) {
          new_node->next->prev = new_node;
        } else {
          phydevices_orderedlist_last = new_node;
        }
        break;
      }
    }
    if (!new_node_inserted) {
      DLLPushBack(phydevices_orderedlist_first,
                  phydevices_orderedlist_last, new_node);
    }
    i += 0;
  }

  bool device_optimal_found = false;
  for (u32 i = 0; i < phydevices_len; ++i) {
    VkPhysicalDevice *device = phydevices_orderedlist_first->device;
    phydevices_orderedlist_first = phydevices_orderedlist_first->next;

    u32 device_extensions_detected_len = 0;
    VkExtensionProperties *device_extensions_detected = 0;
    rhi_vk_get_array(scratch_physical_device.arena,
                     vkEnumerateDeviceExtensionProperties,
                     VkExtensionProperties, device_extensions_detected,
                     device_extensions_detected_len, *device, 0,
                     &device_extensions_detected_len,
                     device_extensions_detected);
    Assert(device_extensions_detected && device_extensions_detected_len > 0);
    u32 device_extensions_found_count = 0;
    for (u32 i = 0; i < Arrsize(device_extensions_required); ++i) {
      for (u32 j = 0; j < device_extensions_detected_len; ++j) {
        if (cstr_eq(device_extensions_detected[j].extensionName,
                    device_extensions_required[i])) {
          device_extensions_found_count += 1;
          break;
        }
      }
    }
    if (device_extensions_found_count == Arrsize(device_extensions_required)) {
      memcopy(&res.physical, device, sizeof (*device));
      device_optimal_found = true;
      break;
    }
  }
  ScratchEnd(scratch_physical_device);
  Assert(device_optimal_found);

#if DEBUG
  VkPhysicalDeviceProperties props = {0};
  vkGetPhysicalDeviceProperties(res.physical, &props);
  dbg_print("Optimal ");
  rhi_vk_device_properties_print(&props);
#endif

  struct Vk_UniqueQueueFamily_Node {
    struct Vk_UniqueQueueFamily_Node *next;
    u32 value;
  };
  struct Vk_UniqueQueueFamily_List {
    struct Vk_UniqueQueueFamily_Node *first;
    struct Vk_UniqueQueueFamily_Node *last;
    u32 count;
  };

  Scratch scratch_queue_family = ScratchBegin(0, 0);
  u32 queue_family_count = 0;
  VkQueueFamilyProperties *queue_families = 0;
  rhi_vk_get_array(scratch_queue_family.arena,
                   vkGetPhysicalDeviceQueueFamilyProperties,
                   VkQueueFamilyProperties, queue_families, queue_family_count,
                   res.physical, &queue_family_count, queue_families);
  Assert(queue_families && queue_family_count > 0);
  memset(res.queue_idx.values, -1, 2 * sizeof(i32));
  struct Vk_UniqueQueueFamily_List queue_family_list = {0};
  for (u32 i = 0, queues_set = 0; i < queue_family_count; ++i, queues_set = 0) {
    if (res.queue_idx.graphics == U32_MAX &&
        queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      res.queue_idx.graphics = i;
      queues_set += 1;
    }
    if (res.queue_idx.transfer == U32_MAX &&
        queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      res.queue_idx.transfer = i;
      queues_set += 1;
    }
    VkBool32 support_present = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(res.physical, i, vk_surface,
                                         &support_present);
    if (res.queue_idx.present == U32_MAX && support_present) {
      res.queue_idx.present = i;
      queues_set += 1;
    }

    if (!queues_set) { continue; }
    bool unique = true;
    for (struct Vk_UniqueQueueFamily_Node *curr = queue_family_list.first;
         curr; curr = curr->next) {
      if (curr->value == i) {
        unique = false;
        break;
      }
    }
    if (!unique) { continue; }
    struct Vk_UniqueQueueFamily_Node *node =
      arena_push(scratch_queue_family.arena,
                 struct Vk_UniqueQueueFamily_Node);
    node->value = i;
    QueuePush(queue_family_list.first, queue_family_list.last, node);
    queue_family_list.count += 1;
  }

  f32 graphics_queue_idx_prio = 1.0f;
  VkDeviceQueueCreateInfo *create_devqueue_info =
    arena_push_many(scratch_queue_family.arena,
                    VkDeviceQueueCreateInfo,
                    queue_family_list.count);
  for (u32 i = 0; i < queue_family_list.count; ++i) {
    struct Vk_UniqueQueueFamily_Node *node = queue_family_list.first;
    QueuePop(queue_family_list.first);
    create_devqueue_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    create_devqueue_info[i].queueFamilyIndex = node->value;
    create_devqueue_info[i].queueCount = 1;
    create_devqueue_info[i].pQueuePriorities = &graphics_queue_idx_prio;
  }

  VkPhysicalDeviceFeatures devfeatures = {0};
  VkDeviceCreateInfo create_device_info = {0};
  create_device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_device_info.pQueueCreateInfos = create_devqueue_info;
  create_device_info.queueCreateInfoCount = queue_family_list.count;
  create_device_info.pEnabledFeatures = &devfeatures;
  create_device_info.enabledLayerCount = Arrsize(rhi_vk_layers);
  create_device_info.ppEnabledLayerNames = rhi_vk_layers;
  create_device_info.enabledExtensionCount =
    Arrsize(device_extensions_required);
  create_device_info.ppEnabledExtensionNames = device_extensions_required;
  VkResult create_device_result = vkCreateDevice(res.physical,
                                                 &create_device_info, 0,
                                                 &res.virtual);
  ScratchEnd(scratch_queue_family);
  Assert(create_device_result == VK_SUCCESS);

  vkGetPhysicalDeviceMemoryProperties(res.physical, &res.memory_properties);
  vkGetDeviceQueue(res.virtual, res.queue_idx.graphics, 0, &res.queue.graphics);
  vkGetDeviceQueue(res.virtual, res.queue_idx.present, 0, &res.queue.present);
  vkGetDeviceQueue(res.virtual, res.queue_idx.transfer, 0, &res.queue.transfer);
  return res;
}

fn RHI_VK_Swapchain
rhi_vk_swapchain_create(Arena *arena, RHI_VK_Device rhi_device,
                        VkSurfaceKHR surface, i32 width, i32 height) {
  RHI_VK_Swapchain res = {0};

  Scratch scratch = ScratchBegin(0, 0);
  u32 swapchain_formats_count = 0;
  VkSurfaceFormatKHR *swapchain_formats = 0;
  rhi_vk_get_array(scratch.arena, vkGetPhysicalDeviceSurfaceFormatsKHR,
                   VkSurfaceFormatKHR, swapchain_formats,
                   swapchain_formats_count, rhi_device.physical, surface,
                   &swapchain_formats_count, swapchain_formats);
  Assert(swapchain_formats && swapchain_formats_count > 0);

  u32 swapchain_present_modes_count = 0;
  VkPresentModeKHR *swapchain_present_modes = 0;
  rhi_vk_get_array(scratch.arena, vkGetPhysicalDeviceSurfacePresentModesKHR,
                   VkPresentModeKHR, swapchain_present_modes,
                   swapchain_present_modes_count, rhi_device.physical, surface,
                   &swapchain_present_modes_count, swapchain_present_modes);
  Assert(swapchain_present_modes && swapchain_present_modes_count > 0);
  ScratchEnd(scratch);

  bool supported_b8g8r8a8_srgb_nonlinear = false;
  for (u32 i = 0; i < swapchain_formats_count; ++i) {
    if (swapchain_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
        swapchain_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      res.surface_format = swapchain_formats[i];
      memcopy(&res.surface_format, &swapchain_formats[i],
              sizeof (*swapchain_formats));
      supported_b8g8r8a8_srgb_nonlinear = true;
      break;
    }
  }
  Assert(supported_b8g8r8a8_srgb_nonlinear);

  for (u32 i = 0; i < swapchain_present_modes_count; ++i) {
    if (swapchain_present_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
      res.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    } else if (swapchain_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      res.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }

  VkSurfaceCapabilitiesKHR swapchain_capabilities = {0};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rhi_device.physical, surface,
                                            &swapchain_capabilities);
  if (swapchain_capabilities.currentExtent.width == U32_MAX &&
      swapchain_capabilities.currentExtent.height == U32_MAX) {
    res.extent.width = Clamp((u32)width,
                             swapchain_capabilities.minImageExtent.width,
                             swapchain_capabilities.maxImageExtent.width);
    res.extent.height = Clamp((u32)height,
                              swapchain_capabilities.minImageExtent.height,
                              swapchain_capabilities.maxImageExtent.height);
  } else {
    res.extent = swapchain_capabilities.currentExtent;
  }

  res.image_count = (i32)swapchain_capabilities.minImageCount + 1;
  if (swapchain_capabilities.maxImageCount > 0) {
    res.image_count = ClampTop(res.image_count,
                               (i32)swapchain_capabilities.maxImageCount);
  }

  VkSwapchainCreateInfoKHR create_swapchain_info = {0};
  create_swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_swapchain_info.surface = surface;
  create_swapchain_info.minImageCount = (u32)res.image_count;
  create_swapchain_info.imageFormat = res.surface_format.format;
  create_swapchain_info.imageColorSpace = res.surface_format.colorSpace;
  create_swapchain_info.imageExtent = res.extent;
  create_swapchain_info.imageArrayLayers = 1;
  create_swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  create_swapchain_info.preTransform = swapchain_capabilities.currentTransform;
  create_swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_swapchain_info.presentMode = res.present_mode;
  create_swapchain_info.clipped = VK_TRUE;
  create_swapchain_info.oldSwapchain = VK_NULL_HANDLE;
  if (rhi_device.queue_idx.graphics == rhi_device.queue_idx.present) {
    create_swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  } else {
    create_swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_swapchain_info.queueFamilyIndexCount =
      Arrsize(rhi_device.queue_idx.values);
    create_swapchain_info.pQueueFamilyIndices =
      (u32*)rhi_device.queue_idx.values;
  }
  VkResult create_swapchain_result =
    vkCreateSwapchainKHR(rhi_device.virtual, &create_swapchain_info,
                         0, &res.swapchain);
  Assert(create_swapchain_result == VK_SUCCESS);

  vkGetSwapchainImagesKHR(rhi_device.virtual, res.swapchain,
                          (u32*)&res.image_count, 0);
  res.images = arena_push_many(arena, VkImage, res.image_count);
  res.image_views = arena_push_many(arena, VkImageView, res.image_count);
  vkGetSwapchainImagesKHR(rhi_device.virtual, res.swapchain,
                          (u32*)&res.image_count, res.images);
  for (i32 i = 0; i < res.image_count; ++i) {
    VkImageViewCreateInfo create_imageview_info = {0};
    create_imageview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_imageview_info.image = res.images[i];
    create_imageview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_imageview_info.format = res.surface_format.format;
    create_imageview_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_imageview_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_imageview_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_imageview_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_imageview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_imageview_info.subresourceRange.baseMipLevel = 0;
    create_imageview_info.subresourceRange.levelCount = 1;
    create_imageview_info.subresourceRange.baseArrayLayer = 0;
    create_imageview_info.subresourceRange.layerCount = 1;

    VkResult create_imageview_result =
      vkCreateImageView(rhi_device.virtual, &create_imageview_info, 0,
                        &res.image_views[i]);
    Assert(create_imageview_result == VK_SUCCESS);
  }

  res.viewport.x = 0.0f;
  res.viewport.y = (f32)res.extent.height;
  res.viewport.width = (f32)res.extent.width;
  res.viewport.height = -(f32)res.extent.height;
  res.viewport.minDepth = 0.0f;
  res.viewport.maxDepth = 1.0f;
  res.scissor.offset.x = 0;
  res.scissor.offset.y = 0;
  res.scissor.extent.width = res.extent.width;
  res.scissor.extent.height = res.extent.height;
  return res;
}

fn void
rhi_vk_swapchain_destroy(RHI_VK_Device *rhi_device,
                         RHI_VK_Swapchain *rhi_swapchain) {
  vkDeviceWaitIdle(rhi_device->virtual);
  for (i32 i = 0; i < rhi_swapchain->image_count; ++i) {
    vkDestroyImageView(rhi_device->virtual, rhi_swapchain->image_views[i], 0);
  }
  vkDestroySwapchainKHR(rhi_device->virtual, rhi_swapchain->swapchain, 0);
}

fn VkFramebuffer*
rhi_vk_framebuffers_create(Arena *arena, RHI_VK_Device *rhi_device,
                           RHI_VK_Swapchain *rhi_swapchain,
                           VkRenderPass renderpass) {
  VkFramebuffer *framebuffers = arena_push_many(arena,
                                                VkFramebuffer,
                                                rhi_swapchain->image_count);
  for (i32 i = 0; i < rhi_swapchain->image_count; ++i) {
    VkFramebufferCreateInfo create_framebuffer_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = renderpass,
      .attachmentCount = 1,
      .pAttachments = &rhi_swapchain->image_views[i],
      .width = rhi_swapchain->extent.width,
      .height = rhi_swapchain->extent.height,
      .layers = 1,
    };
    VkResult create_framebuffer_result =
      vkCreateFramebuffer(rhi_device->virtual, &create_framebuffer_info,
                          0, &framebuffers[i]);
    Assert(create_framebuffer_result == VK_SUCCESS);
  }
  return framebuffers;
}

fn void
rhi_vk_framebuffers_destroy(RHI_VK_Device *rhi_device,
                            RHI_VK_Swapchain *rhi_swapchain,
                            VkFramebuffer *framebuffers) {
  vkDeviceWaitIdle(rhi_device->virtual);
  for (i32 i = 0; i < rhi_swapchain->image_count; ++i) {
    vkDestroyFramebuffer(rhi_device->virtual, framebuffers[i], 0);
  }
}

fn VkDescriptorSet*
rhi_vk_descriptorset_create(Arena *arena, RHI_VK_Device device,
                            VkDescriptorPool descriptor_pool,
                            i32 descriptor_count,
                            VkDescriptorSetLayout descriptor_layout) {
  VkDescriptorSet *res = arena_push_many(arena, VkDescriptorSet,
                                         descriptor_count);
  {
    Scratch scratch = ScratchBegin(0, 0);
    VkDescriptorSetLayout *layouts = arena_push_many(scratch.arena,
                                                     VkDescriptorSetLayout,
                                                     descriptor_count);
    for (i32 i = 0; i < descriptor_count; ++i) {
      layouts[i] = descriptor_layout;
    }

    VkDescriptorSetAllocateInfo alloc_description_set_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptor_pool,
      .descriptorSetCount = (u32)descriptor_count,
      .pSetLayouts = layouts,
    };
    rhi_vk_create(vkAllocateDescriptorSets, device.virtual,
                  &alloc_description_set_info, res);
    ScratchEnd(scratch);
  }
  return res;
}

internal VkDescriptorSetLayout
_rhi_vk_descriptorset_layout_create(RHI_VK_Device device,
                                    VkDescriptorSetLayoutBinding *bindings,
                                    u32 bindings_count) {
  VkDescriptorSetLayout res = {0};
  VkDescriptorSetLayoutCreateInfo create_layout_description_set_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = bindings_count,
    .pBindings = bindings,
  };
  rhi_vk_create(vkCreateDescriptorSetLayout, device.virtual,
                &create_layout_description_set_info, 0,
                &res);
  return res;
}

fn VkSemaphore*
rhi_vk_semaphore_create(Arena *arena, RHI_VK_Device device, i32 count,
                        VkSemaphoreCreateFlags flags) {
  Assert(count > 0);
  VkSemaphoreCreateInfo create_semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .flags = flags,
  };
  VkSemaphore *res = arena_push_many(arena, VkSemaphore, (u32)count);
  for (i32 i = 0; i < count; ++i) {
    rhi_vk_create(vkCreateSemaphore, device.virtual,
                  &create_semaphore_info, 0, res + i);
  }
  return res;
}

fn VkFence*
rhi_vk_fence_create(Arena *arena, RHI_VK_Device device, i32 count,
                    VkFenceCreateFlags flags) {
  Assert(count > 0);
  VkFenceCreateInfo create_fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = flags,
  };
  VkFence *res = arena_push_many(arena, VkFence, count);
  for (i32 i = 0; i < count; ++i) {
    rhi_vk_create(vkCreateFence, device.virtual,
                  &create_fence_info, 0, res + i);
  }
  return res;
}

fn RHI_VK_Buffer
rhi_vk_buffer_create(RHI_VK_Device device, u32 size,
                     VkBufferUsageFlags usage) {
  RHI_VK_Buffer res = {0};
  res.memorypool_position = U32_MAX;
  VkBufferCreateInfo create_buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
    .sharingMode = (device.queue_idx.transfer == device.queue_idx.graphics
                    ? VK_SHARING_MODE_EXCLUSIVE
                    : VK_SHARING_MODE_CONCURRENT),
    .queueFamilyIndexCount = 2,
    .pQueueFamilyIndices = (u32[]) { device.queue_idx.transfer,
                                     device.queue_idx.graphics },
  };
  rhi_vk_create(vkCreateBuffer, device.virtual, &create_buffer_info, 0,
                &res.vkbuffer);
  vkGetBufferMemoryRequirements(device.virtual, res.vkbuffer,
                                &res.memory_requirements);
  return res;
}

fn void
rhi_vk_buffer_copy(RHI_VK_Device device, RHI_VK_Buffer dest, RHI_VK_Buffer src,
                   VkCommandPool cmdpool, VkBufferCopy *buffer_copy_regions,
                   u32 buffer_copy_region_count) {
  Assert(dest.memory_requirements.size >= src.memory_requirements.size);
  VkCommandBufferAllocateInfo alloc_cmdbuffer_info = {0};
  alloc_cmdbuffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_cmdbuffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_cmdbuffer_info.commandPool = cmdpool;
  alloc_cmdbuffer_info.commandBufferCount = 1;

  VkCommandBuffer cmdbuff = {0};
  vkAllocateCommandBuffers(device.virtual, &alloc_cmdbuffer_info, &cmdbuff);

  VkCommandBufferBeginInfo cmdbuff_begin = {0};
  cmdbuff_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdbuff_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmdbuff, &cmdbuff_begin);

  if (buffer_copy_regions) {
    vkCmdCopyBuffer(cmdbuff, src.vkbuffer, dest.vkbuffer,
                    buffer_copy_region_count, buffer_copy_regions);
  } else {
    VkBufferCopy buffer_copy_region = {0};
    buffer_copy_region.size = src.memory_requirements.size;
    buffer_copy_region.srcOffset = 0;
    buffer_copy_region.dstOffset = 0;
    vkCmdCopyBuffer(cmdbuff, src.vkbuffer, dest.vkbuffer,
                    1, &buffer_copy_region);
  }
  vkEndCommandBuffer(cmdbuff);

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmdbuff;
  vkQueueSubmit(device.queue.transfer, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(device.queue.transfer);
  vkFreeCommandBuffers(device.virtual, cmdpool, 1, &cmdbuff);
}

fn void
rhi_vk_buffer_destroy(RHI_VK_Device device, RHI_VK_Buffer buffer) {
  vkDestroyBuffer(device.virtual, buffer.vkbuffer, 0);
}

fn RHI_VK_MemoryPool
rhi_vk_memorypool_build(RHI_VK_Device device, u32 size,
                        VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags wanted_properties) {
  RHI_VK_MemoryPool res = {0};
  res.usage = usage;
  res.buffer = rhi_vk_buffer_create(device, size, usage);
  res.memory = rhi_vk_alloc(device, res.buffer.memory_requirements,
                            wanted_properties);
  vkBindBufferMemory(device.virtual, res.buffer.vkbuffer, res.memory, 0);
  return res;
}

fn RHI_VK_Buffer
rhi_vk_memorypool_push(RHI_VK_Device device, RHI_VK_MemoryPool *pool,
                       u32 size) {
  size = (u32)align_forward(size,
                            (isize)pool->buffer.memory_requirements.alignment);
  RHI_VK_Buffer res = rhi_vk_buffer_create(device, size, pool->usage);
  vkBindBufferMemory(device.virtual, res.vkbuffer, pool->memory, pool->offset);
  res.memorypool_position = pool->offset;
  pool->offset += res.memory_requirements.size;
  return res;
}

fn void*
rhi_vk_memorypool_map(RHI_VK_Device device, RHI_VK_MemoryPool *pool,
                      RHI_VK_Buffer buffer) {
  Assert(buffer.memorypool_position != (u32)-1);
  void *res = 0;
  vkMapMemory(device.virtual, pool->memory, buffer.memorypool_position,
              buffer.memory_requirements.size, 0, &res);
  return res;
}

fn void
rhi_vk_memorypool_unmap(RHI_VK_Device device, RHI_VK_MemoryPool *pool) {
  vkUnmapMemory(device.virtual, pool->memory);
}

fn void
rhi_vk_memorypool_destroy(RHI_VK_Device device, RHI_VK_MemoryPool pool) {
  vkDestroyBuffer(device.virtual, pool.buffer.vkbuffer, 0);
  vkFreeMemory(device.virtual, pool.memory, 0);
}

fn RHI_VK_Shader
rhi_vk_shader_from_file(RHI_ShaderType type, RHI_VK_Device rhi_device,
                        String8 shader_source_path) {
  Scratch scratch = ScratchBegin(0, 0);
  OS_Handle file_handle = os_fs_open(shader_source_path, OS_acfRead);
  String8 bytecode = os_fs_read(scratch.arena, file_handle);
  os_fs_close(file_handle);
  RHI_VK_Shader res = rhi_vk_shader_from_bytes(type, rhi_device,
                                               bytecode.size, bytecode.str);
  ScratchEnd(scratch);
  return res;
}

fn RHI_VK_Shader
rhi_vk_shader_from_bytes(RHI_ShaderType type, RHI_VK_Device rhi_device,
                         isize size, u8 *bytes) {
  RHI_VK_Shader res = {0};
  res.type = type;

  VkShaderModuleCreateInfo create_shadermodule_info = {0};
  create_shadermodule_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_shadermodule_info.codeSize = (usize)size;
  create_shadermodule_info.pCode = (const u32*)bytes;
  VkResult create_shadermodule_result =
    vkCreateShaderModule(rhi_device.virtual, &create_shadermodule_info,
                         0, &res.module);
  Assert(create_shadermodule_result == VK_SUCCESS);
  return res;
}


internal VkDeviceMemory
rhi_vk_alloc(RHI_VK_Device device, VkMemoryRequirements memory_requirements,
             VkMemoryPropertyFlags wanted_properties) {
  VkDeviceMemory res = {0};
  VkMemoryAllocateInfo mem_alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size,
    .memoryTypeIndex = U32_MAX,
  };
  for (u32 i = 0; i < device.memory_properties.memoryTypeCount; i++) {
    if ((memory_requirements.memoryTypeBits & (1 << i)) &&
        (device.memory_properties.memoryTypes[i].propertyFlags &
         wanted_properties)) {
      mem_alloc_info.memoryTypeIndex = i;
      break;
    }
  }

  Assert(mem_alloc_info.memoryTypeIndex != (u32)-1);
  rhi_vk_create(vkAllocateMemory, device.virtual, &mem_alloc_info, 0, &res);
  return res;
}

internal VkRenderPass
_rhi_vk_renderpass_create(RHI_VK_Device device,
                          VkAttachmentDescription *attachments,
                          u32 attachment_count,
                          VkSubpassDescription *subpasses, u32 subpass_count,
                          VkSubpassDependency *dependencies,
                          u32 dependency_count) {
  VkRenderPass res = {0};
  VkRenderPassCreateInfo create_renderpass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = attachment_count,
    .pAttachments = attachments,
    .subpassCount = subpass_count,
    .pSubpasses = subpasses,
    .dependencyCount = dependency_count,
    .pDependencies = dependencies,
  };
  rhi_vk_create(vkCreateRenderPass, device.virtual,
                &create_renderpass_info, 0, &res);
  return res;
}

internal VkDescriptorPool
_rhi_vk_descriptorpool_create(RHI_VK_Device device,
                              VkDescriptorPoolSize *counts, u32 counts_size) {
  VkDescriptorPool res = {0};
  u32 max_set = counts[0].descriptorCount;
  for (u32 i = 1; i < counts_size; ++i) {
    if (counts[i].descriptorCount > max_set) {
      max_set = counts[i].descriptorCount;
    }
  }
  VkDescriptorPoolCreateInfo create_descriptionpool_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .poolSizeCount = counts_size,
    .pPoolSizes = counts,
    .maxSets = max_set,
  };
  rhi_vk_create(vkCreateDescriptorPool, device.virtual,
                &create_descriptionpool_info, 0, &res);
  return res;
}

internal void
_rhi_vk_fence_wait(RHI_VK_Device device, VkFence *fences, u32 count) {
  vkWaitForFences(device.virtual, count, fences, VK_TRUE, UINT64_MAX);
}

internal void
_rhi_vk_fence_wait_at_least_ms(RHI_VK_Device device, u64 milliseconds,
                               VkFence *fences, u32 count) {
  vkWaitForFences(device.virtual, count, fences, VK_TRUE,
                  milliseconds * Million(1));
}

internal void
_rhi_vk_fence_reset(RHI_VK_Device device, VkFence *fences, u32 count) {
  vkResetFences(device.virtual, count, fences);
}

internal void
rhi_vk_device_properties_print(VkPhysicalDeviceProperties *props) {
  char *device_type_string = 0;
  switch (props->deviceType) {
  case VK_PHYSICAL_DEVICE_TYPE_OTHER: {
    device_type_string = "Other";
  } break;
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
    device_type_string = "Integrated GPU";
  } break;
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
    device_type_string = "Discrete GPU";
  } break;
  case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: {
    device_type_string = "Virtual GPU";
  } break;
  case VK_PHYSICAL_DEVICE_TYPE_CPU: {
    device_type_string = "CPU";
  } break;
  default: Assert(false && "invalid vulkan device type");
  }
  u32 vk_major = VK_VERSION_MAJOR(props->apiVersion);
  u32 vk_minor = VK_VERSION_MINOR(props->apiVersion);
  u32 vk_patch = VK_VERSION_PATCH(props->apiVersion);
  u32 driver_major = VK_VERSION_MAJOR(props->driverVersion);
  u32 driver_minor = VK_VERSION_MINOR(props->driverVersion);
  u32 driver_patch = VK_VERSION_PATCH(props->driverVersion);
  dbg_println("Device Name:     \t%s", props->deviceName);
  dbg_println("  Type:          \t%s", device_type_string);
  dbg_println("  Vendor ID:     \t%d", props->vendorID);
  dbg_println("  Device ID:     \t%d", props->deviceID);
  dbg_println("  API Version:   \tv%d.%d.%d", vk_major, vk_minor, vk_patch);
  dbg_println("  Driver Version:\tv%d.%d.%d\n", driver_major, driver_minor,
              driver_patch);
}
