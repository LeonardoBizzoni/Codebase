global RHI_Vulkan_State rhi_vulkan_state = {0};

fn RHI_Handle rhi_context_create(Arena *arena, OS_Handle window) {
  RHI_Vulkan_Context *context = arena_push(arena, RHI_Vulkan_Context);
  Assert(context);

  context->surface = rhi_vulkan_surface_create(window);
  rhi_vulkan_device_init(context);
  rhi_vulkan_swapchain_init(arena, context, window);

  RHI_Handle res = {{ (usize)context }};
  return res;
}

fn void rhi_context_destroy(RHI_Handle hcontext) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context*)hcontext.h[0];
  vkDestroySwapchainKHR(context->device.virtual, context->swapchain.handle, NULL);
  for (u32 i = 0; i < context->swapchain.image_count; ++i) {
    vkDestroyImageView(context->device.virtual, context->swapchain.imageviews[i], NULL);
  }
  vkDestroyDevice(context->device.virtual, NULL);
  vkDestroySurfaceKHR(rhi_vulkan_state.instance, context->surface, NULL);
}

fn RHI_Handle rhi_shader_from_file(Arena *arena, RHI_Handle hcontext,
                                   String8 vertex_shader_path,
                                   String8 pixel_shader_path) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context *)hcontext.h[0];
  RHI_Vulkan_Shader *shader = arena_push(arena, RHI_Vulkan_Shader);
  shader->count = 2;
  shader->vertex.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader->vertex.stage = VK_SHADER_STAGE_VERTEX_BIT;
  shader->vertex.pName = "main";
  shader->pixel.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader->pixel.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shader->pixel.pName = "main";

  OS_Handle vertex_file = os_fs_open(vertex_shader_path, OS_acfRead);
  OS_Handle pixel_file = os_fs_open(pixel_shader_path, OS_acfRead);
  {
    Scratch scratch = ScratchBegin(0, 0);
    String8 vertex_bytecode = os_fs_read(scratch.arena, vertex_file);
    String8 pixel_bytecode = os_fs_read(scratch.arena, pixel_file);

    VkShaderModuleCreateInfo create_shadermodule_info = {0};
    create_shadermodule_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_shadermodule_info.codeSize = (usize)vertex_bytecode.size;
    create_shadermodule_info.pCode = (const u32*)vertex_bytecode.str;
    VkResult create_shadermodule_result = vkCreateShaderModule(context->device.virtual, &create_shadermodule_info, 0, &shader->vertex.module);
    Assert(create_shadermodule_result == VK_SUCCESS);

    create_shadermodule_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_shadermodule_info.codeSize = (usize)pixel_bytecode.size;
    create_shadermodule_info.pCode = (const u32*)pixel_bytecode.str;
    create_shadermodule_result = vkCreateShaderModule(context->device.virtual, &create_shadermodule_info, 0, &shader->pixel.module);
    Assert(create_shadermodule_result == VK_SUCCESS);

    ScratchEnd(scratch);
  }
  os_fs_close(vertex_file);
  os_fs_close(pixel_file);

  RHI_Handle res = {{ (usize)shader }};
  return res;
}

fn RHI_Handle rhi_buffer_alloc(Arena *arena, RHI_Handle hcontext,
                               i32 size, RHI_BufferType type) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context *)hcontext.h[0];
  RHI_Vulkan_Buffer *buffer = arena_push(arena, RHI_Vulkan_Buffer);
  Assert(buffer);

  VkBufferCreateInfo create_buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = (u32)size,
    .sharingMode = (context->device.queue_idx.transfer == context->device.queue_idx.graphics
                    ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT),
    .queueFamilyIndexCount = 2,
    .pQueueFamilyIndices = (u32[]) {
      context->device.queue_idx.transfer,
      context->device.queue_idx.graphics
    },
  };

  VkMemoryPropertyFlags memory_flags;
  switch (type) {
  case RHI_BufferType_Staging: {
    create_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  } break;
  case RHI_BufferType_Vertex: {
    create_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  } break;
  case RHI_BufferType_Index: {
    create_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  } break;
  }
  VkResult create_buffer_result = vkCreateBuffer(context->device.virtual, &create_buffer_info, NULL, &buffer->handle);
  Assert(create_buffer_result == VK_SUCCESS);

  VkMemoryRequirements memory_requirements = {0};
  vkGetBufferMemoryRequirements(context->device.virtual, buffer->handle, &memory_requirements);

  VkMemoryAllocateInfo mem_alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size,
    .memoryTypeIndex = U32_MAX,
  };
  for (u32 i = 0; i < context->device.memory_properties.memoryTypeCount; i++) {
    if ((memory_requirements.memoryTypeBits & (1 << i)) &&
        (context->device.memory_properties.memoryTypes[i].propertyFlags & memory_flags)) {
      mem_alloc_info.memoryTypeIndex = i;
      break;
    }
  }
  Assert(mem_alloc_info.memoryTypeIndex != U32_MAX);
  VkResult mem_alloc_result = vkAllocateMemory(context->device.virtual, &mem_alloc_info, NULL, &buffer->memory);
  Assert(mem_alloc_result == VK_SUCCESS);
  vkBindBufferMemory(context->device.virtual, buffer->handle, buffer->memory, 0);

  RHI_Handle res = {{ (usize)buffer }};
  return res;
}

fn void rhi_buffer_host_send(RHI_Handle hcontext, RHI_Handle hbuffer,
                             const void *data, i32 size) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context *)hcontext.h[0];
  RHI_Vulkan_Buffer *buffer = (RHI_Vulkan_Buffer *)hbuffer.h[0];

  void *buffer_data = 0;
  vkMapMemory(context->device.virtual, buffer->memory, 0, (u64)size, 0, &buffer_data);
  Assert(buffer_data);
  memcopy(buffer_data, data, size);
  vkUnmapMemory(context->device.virtual, buffer->memory);
}

fn void rhi_buffer_copy(RHI_Handle hcontext, RHI_Handle target_buffer,
                        RHI_Handle source_buffer, i32 size) {
  Unused(hcontext);
  Unused(target_buffer);
  Unused(source_buffer);
  Unused(size);
}

fn RHI_Handle rhi_pipeline_create(Arena *arena, RHI_Handle hcontext, RHI_Handle hshader, RHI_BufferLayout layout) {
  i32 actual_layout_count = layout.count;
  VkVertexInputBindingDescription vertex_input_binding = {0};
  vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  for (i32 i = 0; i < layout.count; ++i) {
    vertex_input_binding.stride += (u32)(rhi_shadertype_map_element_count[layout.elements[i].type] *
                                         rhi_shadertype_map_size[layout.elements[i].type]);
    if (layout.elements[i].type == RHI_ShaderDataType_Mat3F32) {
      actual_layout_count += 2;
    } else if (layout.elements[i].type == RHI_ShaderDataType_Mat4F32) {
      actual_layout_count += 3;
    }
  }

  {
    Scratch scratch = ScratchBegin(&arena, 1);
    VkVertexInputAttributeDescription *vertex_input_attributes = arena_push_many(scratch.arena, VkVertexInputAttributeDescription, actual_layout_count);
    u32 offset = 0, location = 0;
    for (i32 i = 0; i < layout.count; ++i) {
      vertex_input_attributes[location].binding = 0;
      vertex_input_attributes[location].location = location;
      vertex_input_attributes[location].offset = offset;
      if (layout.elements[i].type == RHI_ShaderDataType_Mat3F32 ||
          layout.elements[i].type == RHI_ShaderDataType_Mat4F32) {
        vertex_input_attributes[location].format = rhi_vulkan_shadertype_map_format[RHI_ShaderDataType_Vec3F32];
        u32 repeat_for = (layout.elements[i].type == RHI_ShaderDataType_Mat3F32) ? 2 : 3;
        for (u32 j = 0, inner_offset = offset; j < repeat_for; ++j) {
          location += 1;
          inner_offset += (u32)(rhi_shadertype_map_element_count[RHI_ShaderDataType_Vec3F32] *
                                rhi_shadertype_map_size[RHI_ShaderDataType_Vec3F32]);
          vertex_input_attributes[location].binding = 0;
          vertex_input_attributes[location].location = location;
          vertex_input_attributes[location].offset = inner_offset;
          vertex_input_attributes[location].format = rhi_vulkan_shadertype_map_format[RHI_ShaderDataType_Vec3F32];
        }
      } else {
        vertex_input_attributes[location].format = rhi_vulkan_shadertype_map_format[layout.elements[i].type];
        location += 1;
      }

      offset += (u32)(rhi_shadertype_map_element_count[layout.elements[i].type] *
                      rhi_shadertype_map_size[layout.elements[i].type]);
    }

    VkPipelineDynamicStateCreateInfo create_pipeline_dynamic_states_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = (VkDynamicState[]) {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
      },
    };
    VkPipelineVertexInputStateCreateInfo create_vertex_input_state_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertex_input_binding,
      .vertexAttributeDescriptionCount = (u32)actual_layout_count,
      .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    Unused(create_pipeline_dynamic_states_info);
    Unused(create_vertex_input_state_info);
    Unused(hcontext);
    Unused(hshader);

    ScratchEnd(scratch);
  }

  RHI_Handle res = {0};
  return res;
}


fn void rhi_buffer_set_layout(RHI_Handle hcontext, RHI_Handle hbuffer, RHI_BufferLayout layout) {
  Unused(hcontext);
  Unused(hbuffer);
  Unused(layout);
}

internal void rhi_init(void) {
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
    .enabledLayerCount = Arrsize(rhi_vulkan_layers_required),
    .ppEnabledLayerNames = rhi_vulkan_layers_required,
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
    for (usize i = 0; i < Arrsize(rhi_vulkan_layers_required); ++i) {
      bool layer_found = false;
      for (usize j = 0; j < retrieved_layer_count; ++j) {
        if (cstr_eq(rhi_vulkan_layers_required[i], retrieved_layers[j].layerName)) {
          layer_found = true;
          break;
        }
      }
      Assert(layer_found);
    }
    ScratchEnd(scratch);
#endif
  }

  rhi_vulkan_state.swapchain_surface_format.format     = VK_FORMAT_B8G8R8A8_UNORM;
  rhi_vulkan_state.swapchain_surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  rhi_vulkan_state.swapchain_present_mode              = VK_PRESENT_MODE_FIFO_KHR;

  VkResult create_instance_result = vkCreateInstance(&create_instance_info, 0,
                                                     &rhi_vulkan_state.instance);
  Assert(create_instance_result == VK_SUCCESS);
}

internal void rhi_deinit(void) {
  vkDestroyInstance(rhi_vulkan_state.instance, NULL);
}

internal void rhi_vulkan_device_init(RHI_Vulkan_Context *context) {
  local const char *device_extensions_required[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };
  Unused(device_extensions_required);

  {
    Scratch scratch = ScratchBegin(0, 0);
    u32 phydevices_count = 0;
    vkEnumeratePhysicalDevices(rhi_vulkan_state.instance, &phydevices_count, NULL);
    Assert(phydevices_count > 0);
    VkPhysicalDevice *phydevices = arena_push_many(scratch.arena, VkPhysicalDevice, phydevices_count);
    Assert(phydevices);
    vkEnumeratePhysicalDevices(rhi_vulkan_state.instance, &phydevices_count, phydevices);
    // TODO(lb): handle multiple GPUs
    context->device.physical = phydevices[0];
    ScratchEnd(scratch);
  }

#if DEBUG
  rhi_vulkan_device_print(&context->device);
#endif

  {
    Scratch scratch = ScratchBegin(0, 0);
    u32 device_extensions_detected_count = 0;
    vkEnumerateDeviceExtensionProperties(context->device.physical, NULL, &device_extensions_detected_count, NULL);
    Assert(device_extensions_detected_count > 0);
    VkExtensionProperties *device_extensions_found = arena_push_many(scratch.arena, VkExtensionProperties, device_extensions_detected_count);
    Assert(device_extensions_found);
    vkEnumerateDeviceExtensionProperties(context->device.physical, NULL, &device_extensions_detected_count, device_extensions_found);

    u32 device_extensions_found_count = 0;
    for (u32 i = 0; i < Arrsize(device_extensions_required); ++i) {
      for (u32 j = 0; j < device_extensions_detected_count; ++j) {
        if (cstr_eq(device_extensions_found[j].extensionName, device_extensions_required[i])) {
          device_extensions_found_count += 1;
          break;
        }
      }
    }
    AssertAlways(device_extensions_found_count == Arrsize(device_extensions_required));
    ScratchEnd(scratch);
  }

  {
    Scratch scratch = ScratchBegin(0, 0);
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->device.physical, &queue_family_count, NULL);
    Assert(queue_family_count > 0);
    VkQueueFamilyProperties *queue_families = arena_push_many(scratch.arena, VkQueueFamilyProperties, queue_family_count);
    Assert(queue_families);
    vkGetPhysicalDeviceQueueFamilyProperties(context->device.physical, &queue_family_count, queue_families);

    u32 queue_families_idx[3] = {0};
    u32 queue_families_candidate_count = 3;
    for (u32 i = 0; i < queue_family_count; ++i) {
      bool present = false, graphics = false, transfer = false;
      VkBool32 support_present = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(context->device.physical, i, context->surface, &support_present);
      if (support_present)                                      { present  = true; }
      if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { graphics = true; }
      if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) { transfer = true; }

      if (graphics && present && transfer) {
        queue_families_candidate_count = 1;
        context->device.queue_idx.present = context->device.queue_idx.graphics = context->device.queue_idx.transfer = queue_families_idx[0] = i;
        break;
      } else if (graphics && present) {
        queue_families_candidate_count = 2;
        context->device.queue_idx.present = context->device.queue_idx.graphics = queue_families_idx[0] = i;
        for (u32 j = 0; j < queue_family_count; ++j) {
          if (j == i) { continue; }
          vkGetPhysicalDeviceSurfaceSupportKHR(context->device.physical, i, context->surface, &support_present);
          if (support_present) {
            context->device.queue_idx.transfer = queue_families_idx[1] = j;
            break;
          }
        }
        break;
      }
    }
    if (queue_families_candidate_count == 3) {
      for (u32 i = 0; i < queue_family_count; ++i) {
        VkBool32 support_present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(context->device.physical, i, context->surface, &support_present);
        if (support_present) {
          context->device.queue_idx.present = queue_families_idx[0] = i;
        } else if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          context->device.queue_idx.graphics = queue_families_idx[1] = i;
        } else if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
          context->device.queue_idx.transfer = queue_families_idx[2] = i;
        }
      }
    }

    f32 queue_idx_prio = 1.0f;
    VkDeviceQueueCreateInfo *create_devqueue_info = arena_push_many(scratch.arena, VkDeviceQueueCreateInfo, queue_families_candidate_count);
    for (u32 i = 0; i < queue_families_candidate_count; ++i) {
      create_devqueue_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      create_devqueue_info[i].queueFamilyIndex = queue_families_idx[i];
      create_devqueue_info[i].queueCount = 1;
      create_devqueue_info[i].pQueuePriorities = &queue_idx_prio;
    }

    VkPhysicalDeviceFeatures devfeatures = {0};
    VkDeviceCreateInfo create_device_info = {0};
    create_device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_device_info.pQueueCreateInfos = create_devqueue_info;
    create_device_info.queueCreateInfoCount = queue_families_candidate_count;
    create_device_info.pEnabledFeatures = &devfeatures;
    create_device_info.enabledLayerCount = Arrsize(rhi_vulkan_layers_required);
    create_device_info.ppEnabledLayerNames = rhi_vulkan_layers_required;
    create_device_info.enabledExtensionCount = Arrsize(device_extensions_required);
    create_device_info.ppEnabledExtensionNames = device_extensions_required;
    VkResult create_device_result = vkCreateDevice(context->device.physical, &create_device_info, NULL, &context->device.virtual);
    Assert(create_device_result == VK_SUCCESS);

    ScratchEnd(scratch);
  }

  vkGetPhysicalDeviceMemoryProperties(context->device.physical, &context->device.memory_properties);
  vkGetDeviceQueue(context->device.virtual, context->device.queue_idx.graphics, 0, &context->device.queue.graphics);
  vkGetDeviceQueue(context->device.virtual, context->device.queue_idx.present,  0, &context->device.queue.present);
  vkGetDeviceQueue(context->device.virtual, context->device.queue_idx.transfer, 0, &context->device.queue.transfer);
}

internal void rhi_vulkan_device_print(RHI_Vulkan_Device *device) {
  VkPhysicalDeviceProperties props = {0};
  vkGetPhysicalDeviceProperties(device->physical, &props);
  VkPhysicalDeviceFeatures features = {0};
  vkGetPhysicalDeviceFeatures(device->physical, &features);

  char *device_type_string = 0;
  switch (props.deviceType) {
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
  u32 vk_major = VK_VERSION_MAJOR(props.apiVersion);
  u32 vk_minor = VK_VERSION_MINOR(props.apiVersion);
  u32 vk_patch = VK_VERSION_PATCH(props.apiVersion);
  u32 driver_major = VK_VERSION_MAJOR(props.driverVersion);
  u32 driver_minor = VK_VERSION_MINOR(props.driverVersion);
  u32 driver_patch = VK_VERSION_PATCH(props.driverVersion);
  dbg_println("Device Name:     \t%s", props.deviceName);
  dbg_println("  Type:          \t%s", device_type_string);
  dbg_println("  Vendor ID:     \t%d", props.vendorID);
  dbg_println("  Device ID:     \t%d", props.deviceID);
  dbg_println("  API Version:   \tv%d.%d.%d", vk_major, vk_minor, vk_patch);
  dbg_println("  Driver Version:\tv%d.%d.%d\n", driver_major, driver_minor,
              driver_patch);
}

internal void rhi_vulkan_swapchain_init(Arena *arena, RHI_Vulkan_Context *context, OS_Handle hwindow) {
  {
    Scratch scratch = ScratchBegin(0, 0);

    u32 swapchain_formats_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->device.physical, context->surface, &swapchain_formats_count, 0);
    Assert(swapchain_formats_count > 0);
    VkSurfaceFormatKHR *swapchain_formats = arena_push_many(scratch.arena, VkSurfaceFormatKHR, swapchain_formats_count);
    Assert(swapchain_formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->device.physical, context->surface, &swapchain_formats_count, swapchain_formats);

    for (u32 i = 0; i < swapchain_formats_count; ++i) {
      if (swapchain_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
          swapchain_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        goto found_swapchain_format;
      }
    }
    Assert(false);
  found_swapchain_format:;

    u32 swapchain_present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->device.physical, context->surface, &swapchain_present_modes_count, 0);
    Assert(swapchain_present_modes_count > 0);
    VkPresentModeKHR *swapchain_present_modes = arena_push_many(scratch.arena, VkPresentModeKHR, swapchain_present_modes_count);
    Assert(swapchain_present_modes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(context->device.physical, context->surface, &swapchain_present_modes_count, swapchain_present_modes);

    for (u32 i = 0; i < swapchain_present_modes_count; ++i) {
      if (swapchain_present_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
        goto found_present_mode;
      }
    }
    Assert(false);
  found_present_mode:;

    ScratchEnd(scratch);
  }

  VkSurfaceCapabilitiesKHR swapchain_capabilities = {0};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->device.physical, context->surface, &swapchain_capabilities);
  if (swapchain_capabilities.currentExtent.width == U32_MAX) {
    i32 width = 0, height = 0;
    os_window_get_size(hwindow, &width, &height);
    Assert(width > 0 && height > 0);
    context->swapchain.extent.width = Clamp((u32)width, swapchain_capabilities.minImageExtent.width, swapchain_capabilities.maxImageExtent.width);
    context->swapchain.extent.height = Clamp((u32)height, swapchain_capabilities.minImageExtent.height, swapchain_capabilities.maxImageExtent.height);
  } else {
    context->swapchain.extent = swapchain_capabilities.currentExtent;
  }

  context->swapchain.viewport.x = 0.0f;
  context->swapchain.viewport.y = (f32)context->swapchain.extent.height;
  context->swapchain.viewport.width = (f32)context->swapchain.extent.width;
  context->swapchain.viewport.height = -(f32)context->swapchain.extent.height;
  context->swapchain.viewport.minDepth = 0.0f;
  context->swapchain.viewport.maxDepth = 1.0f;
  context->swapchain.scissor.offset.x = 0;
  context->swapchain.scissor.offset.y = 0;
  context->swapchain.scissor.extent.width = context->swapchain.extent.width;
  context->swapchain.scissor.extent.height = context->swapchain.extent.height;

  u32 min_images_count = swapchain_capabilities.minImageCount + 1;
  if (swapchain_capabilities.maxImageCount > 0) {
    min_images_count = ClampTop(min_images_count, swapchain_capabilities.maxImageCount);
  }

  VkSwapchainCreateInfoKHR create_swapchain_info = {0};
  create_swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_swapchain_info.surface = context->surface;
  create_swapchain_info.minImageCount = min_images_count;
  create_swapchain_info.imageFormat = rhi_vulkan_state.swapchain_surface_format.format;
  create_swapchain_info.imageColorSpace = rhi_vulkan_state.swapchain_surface_format.colorSpace;
  create_swapchain_info.imageExtent = context->swapchain.extent;
  create_swapchain_info.imageArrayLayers = 1;
  create_swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  create_swapchain_info.preTransform = swapchain_capabilities.currentTransform;
  create_swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_swapchain_info.presentMode = rhi_vulkan_state.swapchain_present_mode;
  create_swapchain_info.clipped = VK_TRUE;
  create_swapchain_info.oldSwapchain = VK_NULL_HANDLE;
  if (context->device.queue_idx.graphics == context->device.queue_idx.present) {
    create_swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  } else {
    create_swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_swapchain_info.queueFamilyIndexCount = Arrsize(context->device.queue_idx.values);
    create_swapchain_info.pQueueFamilyIndices = context->device.queue_idx.values;
  }
  VkResult create_swapchain_result = vkCreateSwapchainKHR(context->device.virtual, &create_swapchain_info, NULL, &context->swapchain.handle);
  Assert(create_swapchain_result == VK_SUCCESS);

  vkGetSwapchainImagesKHR(context->device.virtual, context->swapchain.handle, &context->swapchain.image_count, NULL);
  context->swapchain.images = arena_push_many(arena, VkImage, context->swapchain.image_count);
  Assert(context->swapchain.images);
  vkGetSwapchainImagesKHR(context->device.virtual, context->swapchain.handle, &context->swapchain.image_count, context->swapchain.images);

  context->swapchain.imageviews = arena_push_many(arena, VkImageView, context->swapchain.image_count);
  Assert(context->swapchain.imageviews);
  for (u32 i = 0; i < context->swapchain.image_count; ++i) {
    VkImageViewCreateInfo create_imageview_info = {0};
    create_imageview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_imageview_info.image = context->swapchain.images[i];
    create_imageview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_imageview_info.format = rhi_vulkan_state.swapchain_surface_format.format;
    create_imageview_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_imageview_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_imageview_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_imageview_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_imageview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_imageview_info.subresourceRange.baseMipLevel = 0;
    create_imageview_info.subresourceRange.levelCount = 1;
    create_imageview_info.subresourceRange.baseArrayLayer = 0;
    create_imageview_info.subresourceRange.layerCount = 1;
    VkResult create_imageview_result = vkCreateImageView(context->device.virtual, &create_imageview_info, NULL, &context->swapchain.imageviews[i]);
    Assert(create_imageview_result == VK_SUCCESS);
  }
}

#if 0
internal void
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

internal RHI_VK_Buffer
rhi_vk_memorypool_push(RHI_VK_Device device, RHI_VK_MemoryPool *pool, u32 size) {
  size = (u32)align_forward(size, (isize)pool->buffer.memory_requirements.alignment);
  RHI_VK_Buffer res = rhi_vk_buffer_create(device, size, pool->usage);
  vkBindBufferMemory(device.virtual, res.vkbuffer, pool->memory, pool->offset);
  res.memorypool_position = pool->offset;
  pool->offset += res.memory_requirements.size;
  return res;
}

internal RHI_VK_MemoryPool
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

internal RHI_VK_Buffer
rhi_vk_buffer_create(RHI_VK_Device device, u32 size, VkBufferUsageFlags usage) {
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

internal RHI_VK_Swapchain
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

internal void
rhi_vk_swapchain_destroy(RHI_VK_Device *rhi_device,
                         RHI_VK_Swapchain *rhi_swapchain) {
  vkDeviceWaitIdle(rhi_device->virtual);
  for (i32 i = 0; i < rhi_swapchain->image_count; ++i) {
    vkDestroyImageView(rhi_device->virtual, rhi_swapchain->image_views[i], 0);
  }
  vkDestroySwapchainKHR(rhi_device->virtual, rhi_swapchain->swapchain, 0);
}

internal VkFramebuffer*
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

internal void
rhi_vk_framebuffers_destroy(RHI_VK_Device *rhi_device,
                            RHI_VK_Swapchain *rhi_swapchain,
                            VkFramebuffer *framebuffers) {
  vkDeviceWaitIdle(rhi_device->virtual);
  for (i32 i = 0; i < rhi_swapchain->image_count; ++i) {
    vkDestroyFramebuffer(rhi_device->virtual, framebuffers[i], 0);
  }
}

internal VkDescriptorSet*
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

internal VkSemaphore*
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

internal VkFence*
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

internal void
rhi_vk_buffer_destroy(RHI_VK_Device device, RHI_VK_Buffer buffer) {
  vkDestroyBuffer(device.virtual, buffer.vkbuffer, 0);
}

internal void*
rhi_vk_memorypool_map(RHI_VK_Device device, RHI_VK_MemoryPool *pool,
                      RHI_VK_Buffer buffer) {
  Assert(buffer.memorypool_position != (u32)-1);
  void *res = 0;
  vkMapMemory(device.virtual, pool->memory, buffer.memorypool_position,
              buffer.memory_requirements.size, 0, &res);
  return res;
}

internal void
rhi_vk_memorypool_unmap(RHI_VK_Device device, RHI_VK_MemoryPool *pool) {
  vkUnmapMemory(device.virtual, pool->memory);
}

internal void
rhi_vk_memorypool_destroy(RHI_VK_Device device, RHI_VK_MemoryPool pool) {
  vkDestroyBuffer(device.virtual, pool.buffer.vkbuffer, 0);
  vkFreeMemory(device.virtual, pool.memory, 0);
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
#endif
