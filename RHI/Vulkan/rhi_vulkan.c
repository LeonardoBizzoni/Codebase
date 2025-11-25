global RHI_Vulkan_State rhi_vulkan_state = {0};

fn RHI_Handle rhi_context_create(Arena *arena, OS_Handle window) {
  RHI_Vulkan_Context *context = arena_push(arena, RHI_Vulkan_Context);
  context->hwindow = window;
  context->surface = rhi_vulkan_surface_create(window);
  if (rhi_vulkan_device_init(context)) {
    rhi_vulkan_swapchain_init(arena, context, window);

    VkCommandPoolCreateInfo create_graphics_pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = context->device.queue_idx.graphics,
    };
    VkCommandPoolCreateInfo create_transfer_pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = context->device.queue_idx.transfer,
    };

    // NOTE(lb): allocates 3 command pools for each CPU core to support
    //           multi-threaded command submission
    context->pools = arena_push_many(arena, RHI_Vulkan_CmdPool, os_sysinfo()->core_count);
    for (i32 i = 0; i < os_sysinfo()->core_count; ++i) {
      vkCreateCommandPool(context->device.virtual, &create_graphics_pool_info, NULL, &context->pools[i].graphics);
      vkCreateCommandPool(context->device.virtual, &create_transfer_pool_info, NULL, &context->pools[i].transfer);
    }

    // NOTE(lb): same as above but for implicit command buffers
    context->cmdbuffs = arena_push_many(arena, RHI_Vulkan_CmdBuffers, os_sysinfo()->core_count);
    VkCommandBufferAllocateInfo alloc_cmdbuff_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };
    for (i32 i = 0; i < os_sysinfo()->core_count; ++i) {
      alloc_cmdbuff_info.commandPool = context->pools[i].graphics;
      VkResult alloc_cmdbuff_result = vkAllocateCommandBuffers(context->device.virtual, &alloc_cmdbuff_info, context->cmdbuffs[i].graphics.handles);
      Assert(alloc_cmdbuff_result == VK_SUCCESS);

      alloc_cmdbuff_info.commandPool = context->pools[i].transfer;
      alloc_cmdbuff_result = vkAllocateCommandBuffers(context->device.virtual, &alloc_cmdbuff_info, context->cmdbuffs[i].transfer.handles);
      Assert(alloc_cmdbuff_result == VK_SUCCESS);
    }

    context->images_in_flight = arena_push_many(arena, VkFence, context->swapchain.image_count);
    VkFenceCreateInfo create_fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VkSemaphoreCreateInfo create_semaphore_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      VkResult create_fence_result = vkCreateFence(context->device.virtual, &create_fence_info, NULL, &context->fences_in_flight[i]);
      Assert(create_fence_result == VK_SUCCESS);
      VkResult create_semaphore_result = vkCreateSemaphore(context->device.virtual, &create_semaphore_info, NULL, &context->semaphores_image_available[i]);
      Assert(create_semaphore_result == VK_SUCCESS);
      create_semaphore_result = vkCreateSemaphore(context->device.virtual, &create_semaphore_info, NULL, &context->semaphores_render_finished[i]);
      Assert(create_semaphore_result == VK_SUCCESS);
    }

    context->semaphores_image_finished = arena_push_many(arena, VkSemaphore, context->swapchain.image_count);
    for (u32 i = 0; i < context->swapchain.image_count; ++i) {
      VkResult create_semaphore_result = vkCreateSemaphore(context->device.virtual, &create_semaphore_info, NULL, &context->semaphores_image_finished[i]);
      Assert(create_semaphore_result == VK_SUCCESS);
    }

    VkDescriptorPoolSize descriptor_pool_size = {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = MAX_FRAMES_IN_FLIGHT,
    };
    VkDescriptorPoolCreateInfo create_descriptor_pool_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = 1,
      .pPoolSizes = &descriptor_pool_size,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
    };
    VkResult create_descritor_pool_result = vkCreateDescriptorPool(context->device.virtual, &create_descriptor_pool_info, NULL, &context->pool_descriptor);
    Assert(create_descritor_pool_result == VK_SUCCESS);
  } else {
    Err("Dynamic rendering isn't supported");
    context = 0;
  }

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

  OS_Handle vertex_file = os_fs_open(vertex_shader_path, OS_AccessFlag_Read);
  OS_Handle pixel_file = os_fs_open(pixel_shader_path, OS_AccessFlag_Read);
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

fn RHI_Handle rhi_pipeline_create(Arena *arena, RHI_Handle hcontext, RHI_Handle hshader,
                                  RHI_BufferLayoutElement layout[], i64 layout_elements_count,
                                  i32 descriptors[], i32 descriptors_count) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context *)hcontext.h[0];
  RHI_Vulkan_Shader *shader = (RHI_Vulkan_Shader *)hshader.h[0];
  RHI_Vulkan_Pipeline *pipeline = arena_push(arena, RHI_Vulkan_Pipeline);

  VkPipelineLayoutCreateInfo create_pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
  };

  {
    Scratch scratch = ScratchBegin(&arena, 1);

    VkDescriptorSetLayout descriptor_set_layout = {0};
    VkDescriptorSetLayoutBinding *bindings = arena_push_many(scratch.arena, VkDescriptorSetLayoutBinding, descriptors_count);
    for (i32 i = 0; i < descriptors_count; ++i) {
      bindings[i].binding = (u32)i;
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      bindings[i].descriptorCount = (u32)descriptors[i];
      bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    }
    VkDescriptorSetLayoutCreateInfo create_descriptor_set_layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = (u32)descriptors_count,
      .pBindings = bindings,
    };
    VkResult create_descriptor_set_layout_result = vkCreateDescriptorSetLayout(context->device.virtual, &create_descriptor_set_layout_info, NULL, &descriptor_set_layout);
    Assert(create_descriptor_set_layout_result == VK_SUCCESS);

    create_pipeline_layout_info.setLayoutCount = 1;
    create_pipeline_layout_info.pSetLayouts = &descriptor_set_layout;

    VkDescriptorSetLayout descriptor_set_layouts[MAX_FRAMES_IN_FLIGHT];
    for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      descriptor_set_layouts[i] = descriptor_set_layout;
    }
    VkDescriptorSetAllocateInfo alloc_descriptor_set_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = context->pool_descriptor,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = descriptor_set_layouts,
    };
    VkResult alloc_descriptor_set_result = vkAllocateDescriptorSets(context->device.virtual, &alloc_descriptor_set_info, context->descriptor_sets);
    Assert(alloc_descriptor_set_result == VK_SUCCESS);

    VkResult create_pipeline_layout_result = vkCreatePipelineLayout(context->device.virtual, &create_pipeline_layout_info, NULL, &pipeline->layout);
    Assert(create_pipeline_layout_result == VK_SUCCESS);
    ScratchEnd(scratch);
  }

  i64 actual_layout_count = layout_elements_count;
  VkVertexInputBindingDescription vertex_input_binding = {0};
  vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  for (i64 i = 0; i < layout_elements_count; ++i) {
    vertex_input_binding.stride += (u32)rhi_shadertype_map_size[layout[i].type];
    if (layout[i].type == RHI_ShaderDataType_Mat3F32) {
      actual_layout_count += 2;
    } else if (layout[i].type == RHI_ShaderDataType_Mat4F32) {
      actual_layout_count += 3;
    }
  }

  VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo create_pipeline_dynamic_states_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = Arrsize(dynamic_states),
    .pDynamicStates = dynamic_states,
  };
  VkPipelineViewportStateCreateInfo create_viewport_state_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &context->swapchain.viewport,
    .scissorCount = 1,
    .pScissors = &context->swapchain.scissor,
  };
  VkPipelineVertexInputStateCreateInfo create_vertex_input_state_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertex_input_binding,
    .vertexAttributeDescriptionCount = (u32)actual_layout_count,
  };
  VkPipelineInputAssemblyStateCreateInfo create_assembly_state_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };
  VkPipelineRasterizationStateCreateInfo create_rasterization_state_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
  };
  VkPipelineMultisampleStateCreateInfo create_multisample_state_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1.0f,
    .pSampleMask = 0,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE,
  };
  VkPipelineColorBlendAttachmentState pipeline_colorblend_attachment = {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
  };
  VkPipelineColorBlendStateCreateInfo create_colorblend_state_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &pipeline_colorblend_attachment,
    .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
  };
  VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM;
  VkPipelineRenderingCreateInfoKHR create_pipeline_rendering_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
    .pNext = VK_NULL_HANDLE,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &color_format,
  };

  {
    Scratch scratch = ScratchBegin(&arena, 1);
    VkVertexInputAttributeDescription *vertex_input_attributes = arena_push_many(scratch.arena, VkVertexInputAttributeDescription, actual_layout_count);
    create_vertex_input_state_info.pVertexAttributeDescriptions = vertex_input_attributes;
    u32 offset = 0, location = 0;
    for (i32 i = 0; i < layout_elements_count; ++i) {
      vertex_input_attributes[location].binding = 0;
      vertex_input_attributes[location].location = location;
      vertex_input_attributes[location].offset = offset;
      if (layout[i].type == RHI_ShaderDataType_Mat3F32 || layout[i].type == RHI_ShaderDataType_Mat4F32) {
        vertex_input_attributes[location].format = rhi_vulkan_shadertype_map_format[RHI_ShaderDataType_Vec3F32];
        u32 repeat_for = (layout[i].type == RHI_ShaderDataType_Mat3F32) ? 2 : 3;
        for (u32 j = 0, inner_offset = offset; j < repeat_for; ++j) {
          location += 1;
          inner_offset += (u32)rhi_shadertype_map_size[RHI_ShaderDataType_Vec3F32];
          vertex_input_attributes[location].binding = 0;
          vertex_input_attributes[location].location = location;
          vertex_input_attributes[location].offset = inner_offset;
          vertex_input_attributes[location].format = rhi_vulkan_shadertype_map_format[RHI_ShaderDataType_Vec3F32];
        }
      } else {
        vertex_input_attributes[location].format = rhi_vulkan_shadertype_map_format[layout[i].type];
        location += 1;
      }

      offset += (u32)rhi_shadertype_map_size[layout[i].type];
    }

    VkGraphicsPipelineCreateInfo create_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .layout = pipeline->layout,
      .renderPass = VK_NULL_HANDLE,
      .pNext = &create_pipeline_rendering_info,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = -1,
      .stageCount = 2,
      .pStages = shader->values,
      .pVertexInputState = &create_vertex_input_state_info,
      .pInputAssemblyState = &create_assembly_state_info,
      .pViewportState = &create_viewport_state_info,
      .pRasterizationState = &create_rasterization_state_info,
      .pMultisampleState = &create_multisample_state_info,
      .pColorBlendState = &create_colorblend_state_info,
      .pDynamicState = &create_pipeline_dynamic_states_info,
    };
    VkResult create_pipeline_result = vkCreateGraphicsPipelines(context->device.virtual, VK_NULL_HANDLE, 1, &create_pipeline_info, NULL, &pipeline->handle);
    Assert(create_pipeline_result == VK_SUCCESS);

    ScratchEnd(scratch);
  }

  RHI_Handle res = {{ (u64)pipeline }};
  return res;
}

fn void rhi_pipeline_destroy(RHI_Handle hcontext, RHI_Handle hpipeline) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context *)hcontext.h[0];
  RHI_Vulkan_Pipeline *pipeline = (RHI_Vulkan_Pipeline *)hpipeline.h[0];
  Assert(context);
  Assert(pipeline);
  vkDestroyPipelineLayout(context->device.virtual, pipeline->layout, NULL);
}

fn RHI_Handle rhi_buffer_alloc(Arena *arena, RHI_Handle hcontext, i32 size, RHI_BufferType type) {
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
  case RHI_BufferType_Uniform: {
    create_buffer_info.size = (u32)(MAX_FRAMES_IN_FLIGHT * align_forward(size, (i64)context->device.properties.limits.minUniformBufferOffsetAlignment));
    create_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
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
  for (u32 i = 0; i < context->device.properties_memory.memoryTypeCount; i++) {
    if ((memory_requirements.memoryTypeBits & (1 << i)) &&
        (context->device.properties_memory.memoryTypes[i].propertyFlags & memory_flags)) {
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

fn void* rhi_buffer_memory_map(RHI_Handle hcontext, RHI_Handle hbuffer, i32 size, i32 offset) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context *)hcontext.h[0];
  RHI_Vulkan_Buffer *buffer = (RHI_Vulkan_Buffer *)hbuffer.h[0];
  void *res = 0;
  vkMapMemory(context->device.virtual, buffer->memory, (u32)offset, (u32)size, 0, &res);
  return res;
}

fn void rhi_buffer_memory_unmap(RHI_Handle hcontext, RHI_Handle hbuffer) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context *)hcontext.h[0];
  RHI_Vulkan_Buffer *buffer = (RHI_Vulkan_Buffer *)hbuffer.h[0];
  vkUnmapMemory(context->device.virtual, buffer->memory);
}

fn void rhi_command_queue_push(RHI_Handle hcontext, RHI_Command cmd) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context *)hcontext.h[0];

  switch (cmd.type) {
  case RHI_CommandType_Copy: {
    context->cmdbuffs[tls_ctx.id].transfer.submitted_count += 1;
    RHI_Vulkan_Buffer *source_buffer = (RHI_Vulkan_Buffer *)cmd.copy.source.h[0];
    RHI_Vulkan_Buffer *target_buffer = (RHI_Vulkan_Buffer *)cmd.copy.target.h[0];

    // TODO(lb): queuing a second copy commands overrides the previous one
    VkCommandBufferBeginInfo cmdbuff_begin_info = {0};
    cmdbuff_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbuff_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(context->cmdbuffs[tls_ctx.id].transfer.handles[context->frame_current], &cmdbuff_begin_info);
    {
      VkBufferCopy buffer_copy_region = {0};
      buffer_copy_region.size = (u32)cmd.copy.size;
      buffer_copy_region.srcOffset = (u32)cmd.copy.source_offset;
      buffer_copy_region.dstOffset = (u32)cmd.copy.target_offset;
      vkCmdCopyBuffer(context->cmdbuffs[tls_ctx.id].transfer.handles[context->frame_current], source_buffer->handle, target_buffer->handle, 1, &buffer_copy_region);
    }
    vkEndCommandBuffer(context->cmdbuffs[tls_ctx.id].transfer.handles[context->frame_current]);
  } break;
  case RHI_CommandType_Clear_Color: {
    memcopy(context->clear_value.color.float32, cmd.clear_color.values, 4 * sizeof(f32));
  } break;
  case RHI_CommandType_Frame_Begin: {
    i32 window_width, window_height;
  swapchain_should_resize:;
    os_window_get_size(context->hwindow, &window_width, &window_height);
    if (context->swapchain.image_width != window_width || context->swapchain.image_height != window_height) {
      vkDeviceWaitIdle(context->device.virtual);
      rhi_vulkan_swapchain_destroy(context);
      rhi_vulkan_swapchain_create(cmd.frame_begin.arena, context, window_width, window_height);
      goto swapchain_should_resize;
    }

    // ======================================================================
    // Uniform updates
    RHI_Vulkan_Buffer *buffer = (RHI_Vulkan_Buffer *)cmd.frame_begin.uniform_buffer.h[0];
    u64 offset = (u64)context->frame_current * context->device.properties.limits.minUniformBufferOffsetAlignment;
    u64 size = (u64)align_forward(cmd.frame_begin.uniform_size, (i64)context->device.properties.limits.minUniformBufferOffsetAlignment);

    u8 *memory = 0;
    vkMapMemory(context->device.virtual, buffer->memory, offset, (u32)cmd.frame_begin.uniform_size, 0, (void**)&memory);
    memcopy(memory, cmd.frame_begin.uniform_data, cmd.frame_begin.uniform_size);

    VkMappedMemoryRange mapped_range_info = {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .memory = buffer->memory,
      .offset = offset,
      .size = size,
    };
    VkResult mapped_range_result = vkFlushMappedMemoryRanges(context->device.virtual, 1, &mapped_range_info);
    Assert(mapped_range_result == VK_SUCCESS);
    vkUnmapMemory(context->device.virtual, buffer->memory);

    VkDescriptorBufferInfo descriptor_buffer_info = {
      .buffer = buffer->handle,
      .offset = offset,
      .range = size,
    };
    VkWriteDescriptorSet descriptor_set_write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = context->descriptor_sets[context->frame_current],
      .dstBinding = (u32)cmd.frame_begin.binding,
      .dstArrayElement = (u32)cmd.frame_begin.array_element_index,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .pBufferInfo = &descriptor_buffer_info,
    };
    vkUpdateDescriptorSets(context->device.virtual, 1, &descriptor_set_write, 0, NULL);
    // ======================================================================

    vkWaitForFences(context->device.virtual, 1, &context->fences_in_flight[context->frame_current], VK_TRUE, U64_MAX);
    VkResult acquire_next_image_result = vkAcquireNextImageKHR(context->device.virtual, context->swapchain.handle,
                                                               UINT64_MAX, context->semaphores_image_available[context->frame_current],
                                                               VK_NULL_HANDLE, &context->frame_current_image);
    Assert(acquire_next_image_result == VK_SUCCESS);

    VkCommandBuffer cmdbuff = context->cmdbuffs[tls_ctx.id].graphics.handles[context->frame_current];
    vkResetCommandBuffer(cmdbuff, 0);
    VkCommandBufferBeginInfo cmdbuff_begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = 0,
    };
    VkRenderingAttachmentInfo rendering_color_attachment_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = context->swapchain.imageviews[context->frame_current_image],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = context->clear_value,
    };
    VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{0, 0}, {(u32)window_width, (u32)window_height}},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &rendering_color_attachment_info,
    };
    vkBeginCommandBuffer(cmdbuff, &cmdbuff_begin_info);
    VkImageMemoryBarrier image_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = context->swapchain.images[context->frame_current_image],
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      }
    };
    vkCmdPipelineBarrier(cmdbuff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &image_barrier);
    vkCmdBeginRendering(cmdbuff, &rendering_info);

    RHI_Vulkan_Pipeline *pipeline = (RHI_Vulkan_Pipeline *)cmd.frame_begin.pipeline.h[0];
    Assert(pipeline);
    vkCmdBindPipeline(cmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle);
    vkCmdSetViewport(cmdbuff, 0, 1, &context->swapchain.viewport);
    vkCmdSetScissor(cmdbuff, 0, 1, &context->swapchain.scissor);
  } break;
  case RHI_CommandType_Frame_Draw_Index: {
    RHI_Vulkan_Pipeline *pipeline = (RHI_Vulkan_Pipeline *)cmd.draw_index.pipeline.h[0];
    RHI_Vulkan_Buffer *vertex_buffer = (RHI_Vulkan_Buffer *)cmd.draw_index.vertex_buffer.h[0];
    RHI_Vulkan_Buffer *index_buffer = (RHI_Vulkan_Buffer *)cmd.draw_index.index_buffer.h[0];

    VkCommandBuffer cmdbuff = context->cmdbuffs[tls_ctx.id].graphics.handles[context->frame_current];
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdbuff, 0, 1, &vertex_buffer->handle, offsets);
    vkCmdBindIndexBuffer(cmdbuff, index_buffer->handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmdbuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &context->descriptor_sets[context->frame_current], 0, NULL);
    vkCmdDrawIndexed(cmdbuff, (u32)cmd.draw_index.indices_count, 1, 0, 0, 0);
  } break;
  case RHI_CommandType_Frame_End: {
    VkCommandBuffer cmdbuff = context->cmdbuffs[tls_ctx.id].graphics.handles[context->frame_current];
    vkCmdEndRendering(cmdbuff);
    VkImageMemoryBarrier image_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = 0,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = context->swapchain.images[context->frame_current_image],
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      }
    };
    vkCmdPipelineBarrier(cmdbuff, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &image_barrier);
    vkEndCommandBuffer(cmdbuff);

    if (context->images_in_flight[context->frame_current_image] != VK_NULL_HANDLE) {
      vkWaitForFences(context->device.virtual, 1, &context->images_in_flight[context->frame_current_image], VK_TRUE, U64_MAX);
    }
    context->images_in_flight[context->frame_current_image] = context->fences_in_flight[context->frame_current];

    VkSemaphore waitSemaphores[] = {context->semaphores_image_available[context->frame_current]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {context->semaphores_image_finished[context->frame_current_image]};
    VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = waitSemaphores,
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmdbuff,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signalSemaphores,
    };
    vkResetFences(context->device.virtual, 1, &context->fences_in_flight[context->frame_current]);
    VkResult submit_result = vkQueueSubmit(context->device.queue.graphics, 1, &submitInfo, context->fences_in_flight[context->frame_current]);
    Assert(submit_result == VK_SUCCESS);

    VkSwapchainKHR swapChains[] = {context->swapchain.handle};
    VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signalSemaphores,
      .swapchainCount = 1,
      .pSwapchains = swapChains,
      .pImageIndices = &context->frame_current_image,
    };
    VkResult queue_present_result = vkQueuePresentKHR(context->device.queue.present, &presentInfo);
    if (queue_present_result != VK_ERROR_OUT_OF_DATE_KHR) {
      i32 window_width, window_height;
      vkDeviceWaitIdle(context->device.virtual);
      os_window_get_size(context->hwindow, &window_width, &window_height);
      rhi_vulkan_swapchain_destroy(context);
      rhi_vulkan_swapchain_create(cmd.frame_end.arena, context, window_width, window_height);
    }

    context->frame_current = (context->frame_current + 1) % MAX_FRAMES_IN_FLIGHT;
  } break;
  }
}

fn void rhi_command_queue_submit(RHI_Handle hcontext) {
  RHI_Vulkan_Context *context = (RHI_Vulkan_Context *)hcontext.h[0];
  if (context->cmdbuffs[tls_ctx.id].transfer.submitted_count > 0) {
    VkSubmitInfo submit_transfer_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &context->cmdbuffs[tls_ctx.id].transfer.handles[context->frame_current],
    };
    vkQueueSubmit(context->device.queue.transfer, 1, &submit_transfer_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(context->device.queue.transfer);
    context->cmdbuffs[tls_ctx.id].transfer.submitted_count = 0;
  }
  // TODO(lb): is this even useful? all the graphics commands go through `RHI_CommandType_Frame_Begin/End`
  if (context->cmdbuffs[tls_ctx.id].graphics.submitted_count > 0) {
    VkSubmitInfo submit_graphics_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &context->cmdbuffs[tls_ctx.id].graphics.handles[context->frame_current],
    };
    vkQueueSubmit(context->device.queue.graphics, 1, &submit_graphics_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(context->device.queue.graphics);
    context->cmdbuffs[tls_ctx.id].graphics.submitted_count = 0;
  }
}

internal void rhi_init(void) {
  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .engineVersion = VK_MAKE_VERSION(1,3,0),
    .apiVersion = VK_API_VERSION_1_3,
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

internal bool rhi_vulkan_device_init(RHI_Vulkan_Context *context) {
  local const char *device_extensions_required[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,

    // NOTE(lb): this is to bypass framebuffers & renderpasses
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
    "VK_KHR_create_renderpass2",
  };

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

  vkGetPhysicalDeviceProperties(context->device.physical, &context->device.properties);

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
    if (device_extensions_found_count != Arrsize(device_extensions_required)) {
      ScratchEnd(scratch);
      return false;
    }
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

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature = {0};
    dynamic_rendering_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamic_rendering_feature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceFeatures devfeatures = {0};
    VkDeviceCreateInfo create_device_info = {0};
    create_device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_device_info.pNext = &dynamic_rendering_feature;
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

  vkGetPhysicalDeviceMemoryProperties(context->device.physical, &context->device.properties_memory);
  vkGetDeviceQueue(context->device.virtual, context->device.queue_idx.graphics, 0, &context->device.queue.graphics);
  vkGetDeviceQueue(context->device.virtual, context->device.queue_idx.present,  0, &context->device.queue.present);
  vkGetDeviceQueue(context->device.virtual, context->device.queue_idx.transfer, 0, &context->device.queue.transfer);

  return true;
}

internal void rhi_vulkan_device_print(RHI_Vulkan_Device *device) {
  VkPhysicalDeviceFeatures features = {0};
  vkGetPhysicalDeviceFeatures(device->physical, &features);

  char *device_type_string = 0;
  switch (device->properties.deviceType) {
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
  u32 vk_major = VK_VERSION_MAJOR(device->properties.apiVersion);
  u32 vk_minor = VK_VERSION_MINOR(device->properties.apiVersion);
  u32 vk_patch = VK_VERSION_PATCH(device->properties.apiVersion);
  u32 driver_major = VK_VERSION_MAJOR(device->properties.driverVersion);
  u32 driver_minor = VK_VERSION_MINOR(device->properties.driverVersion);
  u32 driver_patch = VK_VERSION_PATCH(device->properties.driverVersion);
  dbg_println("Device Name:     \t%s", device->properties.deviceName);
  dbg_println("  Type:          \t%s", device_type_string);
  dbg_println("  Vendor ID:     \t%d", device->properties.vendorID);
  dbg_println("  Device ID:     \t%d", device->properties.deviceID);
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

  i32 width = 0, height = 0;
  os_window_get_size(hwindow, &width, &height);
  Assert(width > 0 && height > 0);
  rhi_vulkan_swapchain_create(arena, context, width, height);
}

internal void rhi_vulkan_swapchain_create(Arena *arena, RHI_Vulkan_Context *context, i32 width, i32 height) {
  context->swapchain.image_width = width;
  context->swapchain.image_height = height;

  VkSurfaceCapabilitiesKHR swapchain_capabilities = {0};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->device.physical, context->surface, &swapchain_capabilities);
  if (swapchain_capabilities.currentExtent.width == U32_MAX) {
    context->swapchain.extent.width = Clamp((u32)width, swapchain_capabilities.minImageExtent.width, swapchain_capabilities.maxImageExtent.width);
    context->swapchain.extent.height = Clamp((u32)height, swapchain_capabilities.minImageExtent.height, swapchain_capabilities.maxImageExtent.height);
  } else {
    context->swapchain.extent = swapchain_capabilities.currentExtent;
  }

  context->swapchain.viewport.x = 0.f;
  context->swapchain.viewport.y = (f32)context->swapchain.extent.height;
  context->swapchain.viewport.width = (f32)context->swapchain.extent.width;
  context->swapchain.viewport.height = -(f32)context->swapchain.extent.height;
  context->swapchain.viewport.minDepth = 0.f;
  context->swapchain.viewport.maxDepth = 1.f;
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

internal void rhi_vulkan_swapchain_destroy(RHI_Vulkan_Context *context) {
  vkDeviceWaitIdle(context->device.virtual);
  for (u32 i = 0; i < context->swapchain.image_count; ++i) {
    vkDestroyImageView(context->device.virtual, context->swapchain.imageviews[i], 0);
  }
  vkDestroySwapchainKHR(context->device.virtual, context->swapchain.handle, 0);
}
