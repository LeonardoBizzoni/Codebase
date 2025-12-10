fn RHI_Handle rhi_shader_from_file(Arena *arena, RHI_Handle hcontext,
                                   String8 vertex_shader_path,
                                   String8 fragment_shader_path) {
  Unused(arena); Unused(hcontext);
  RHI_OpenglObj vertex =
    rhi_opengl_shader_program_from_file(vertex_shader_path,
                                        RHI_ShaderType_Vertex);
  RHI_OpenglObj fragment =
    rhi_opengl_shader_program_from_file(fragment_shader_path,
                                        RHI_ShaderType_Fragment);

  RHI_OpenglObj shader = glCreateProgram();
  rhi_opengl_call(glAttachShader(shader, vertex));
  rhi_opengl_call(glAttachShader(shader, fragment));
  rhi_opengl_call(glLinkProgram(shader));
  rhi_opengl_call(glDeleteShader(vertex));
  rhi_opengl_call(glDeleteShader(fragment));
  RHI_Handle res = {{ (u64)shader }};
  return res;
}

fn RHI_Handle rhi_pipeline_create(Arena *arena, RHI_Handle hcontext, RHI_Handle hshader,
                                  RHI_BufferLayoutElement *layout, i32 layout_elements_count,
                                  String8 *uniform_buffer_objects, i32 uniform_buffer_objects_count) {
  Unused(hcontext);

  RHI_OpenglPrimitive *prim = arena_push(arena, RHI_OpenglPrimitive);
  prim->type = RHI_OpenglPrimitiveType_Pipeline;
  prim->pipeline.shader = (RHI_OpenglObj)hshader.h[0];

  i32 stride = 0;
  for (u32 i = 0; i < (u32)layout_elements_count; ++i) {
    stride += rhi_shadertype_map_size[layout[i].type];
  }
  isize offset = 0;
  for (u32 i = 0; i < (u32)layout_elements_count; ++i) {
    rhi_opengl_call(glEnableVertexAttribArray(i));
    rhi_opengl_call(glVertexAttribPointer(i, rhi_shadertype_map_element_count[layout[i].type],
                                          rhi_opengl_type_from_shadertype(layout[i].type),
                                          layout[i].to_normalize ? GL_TRUE : GL_FALSE,
                                          stride, (const void*)offset));
    offset += rhi_shadertype_map_size[layout[i].type];
  }

  RHI_OpenglObj shader = (RHI_OpenglObj)hshader.h[0];
  rhi_opengl_call(glUseProgram(shader));
  prim->pipeline.ubos_count = uniform_buffer_objects_count;
  prim->pipeline.ubos = arena_push_many(arena, RHI_OpenglObj, uniform_buffer_objects_count);
  {
    Scratch scratch = ScratchBegin(&arena, 1);
    for (i32 i = 0; i < uniform_buffer_objects_count; ++i) {
      rhi_opengl_error_clear();
      prim->pipeline.ubos[i] = glGetUniformBlockIndex(shader, cstr_from_str8(scratch.arena, uniform_buffer_objects[i]));
      rhi_opengl_error_check();
      rhi_opengl_call(glUniformBlockBinding(shader, prim->pipeline.ubos[i], (u32)i));
    }
    ScratchEnd(scratch);
  }

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH);

  RHI_Handle res = {{ (u64)prim }};
  return res;
}

fn void rhi_pipeline_destroy(RHI_Handle hcontext, RHI_Handle hpipeline) {
  Unused(hcontext);
  Unused(hpipeline);
}

fn RHI_Handle rhi_buffer_alloc(Arena *arena, RHI_Handle hcontext, i32 size, RHI_BufferType type) {
  Unused(hcontext);
  RHI_OpenglPrimitive *prim = arena_push(arena, RHI_OpenglPrimitive);
  Assert(prim);
  switch (type) {
  case RHI_BufferType_Staging: {
    prim->type = RHI_OpenglPrimitiveType_Buffer;
    rhi_opengl_call(glGenBuffers(1, &prim->buffer));
    rhi_opengl_call(glBindBuffer(GL_COPY_WRITE_BUFFER, prim->buffer));
    rhi_opengl_call(glBufferData(GL_COPY_WRITE_BUFFER, size, NULL, GL_DYNAMIC_DRAW));
  } break;
  case RHI_BufferType_Vertex: {
    prim->type = RHI_OpenglPrimitiveType_Buffer;
    rhi_opengl_call(glGenBuffers(1, &prim->buffer));
    rhi_opengl_call(glBindBuffer(GL_ARRAY_BUFFER, prim->buffer));
    rhi_opengl_call(glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW));
  } break;
  case RHI_BufferType_Index: {
    prim->type = RHI_OpenglPrimitiveType_Index;
    prim->index.vertex_count = (u32)size / sizeof(i32);
    rhi_opengl_call(glGenBuffers(1, &prim->index.buffer));
    rhi_opengl_call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim->index.buffer));
    rhi_opengl_call(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW));
  } break;
  case RHI_BufferType_Uniform: {
    prim->type = RHI_OpenglPrimitiveType_Buffer;
    rhi_opengl_call(glGenBuffers(1, &prim->buffer));
    rhi_opengl_call(glBindBuffer(GL_UNIFORM_BUFFER, prim->buffer));
    rhi_opengl_call(glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW));
  } break;
  }
  RHI_Handle res = {{ (u64)prim }};
  return res;
}

fn void* rhi_buffer_memory_map(RHI_Handle hcontext, RHI_Handle hbuffer, i32 size, i32 offset) {
  Unused(hcontext);
  RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive*)hbuffer.h[0];
  Assert(prim->type == RHI_OpenglPrimitiveType_Buffer || prim->type == RHI_OpenglPrimitiveType_Index);
  RHI_OpenglObj buffer = prim->buffer;
  rhi_opengl_call(glBindBuffer(GL_COPY_WRITE_BUFFER, buffer));
  rhi_opengl_error_clear();
  void *res = glMapBufferRange(GL_COPY_WRITE_BUFFER, offset, size,
                               GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  return res;
}

fn void rhi_buffer_memory_unmap(RHI_Handle hcontext, RHI_Handle hbuffer) {
  Unused(hcontext);
  RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive*)hbuffer.h[0];
  Assert(prim->type == RHI_OpenglPrimitiveType_Buffer || prim->type == RHI_OpenglPrimitiveType_Index);
  RHI_OpenglObj buffer = prim->buffer;
  rhi_opengl_call(glBindBuffer(GL_COPY_WRITE_BUFFER, buffer));
  rhi_opengl_call(glUnmapBuffer(GL_COPY_WRITE_BUFFER));
}

fn void rhi_command_queue_push(RHI_Handle hcontext, RHI_Command cmd) {
  switch (cmd.type) {
  case RHI_CommandType_Copy: {
    RHI_OpenglPrimitive *target_prim = (RHI_OpenglPrimitive*)cmd.copy.target.h[0];
    RHI_OpenglPrimitive *source_prim = (RHI_OpenglPrimitive*)cmd.copy.source.h[0];
    Assert(target_prim->type == RHI_OpenglPrimitiveType_Buffer || target_prim->type == RHI_OpenglPrimitiveType_Index);
    Assert(source_prim->type == RHI_OpenglPrimitiveType_Buffer || source_prim->type == RHI_OpenglPrimitiveType_Index);

    RHI_OpenglObj source_buffer;
    RHI_OpenglObj target_buffer;
    switch (source_prim->type) {
    case RHI_OpenglPrimitiveType_Buffer: {
      source_buffer = source_prim->buffer;
    } break;
    case RHI_OpenglPrimitiveType_Index: {
      source_buffer = source_prim->index.buffer;
    } break;
    }
    switch (target_prim->type) {
    case RHI_OpenglPrimitiveType_Buffer: {
      target_buffer = target_prim->buffer;
    } break;
    case RHI_OpenglPrimitiveType_Index: {
      target_buffer = target_prim->index.buffer;
    } break;
    }

    rhi_opengl_call(glBindBuffer(GL_COPY_READ_BUFFER, source_buffer));
    rhi_opengl_call(glBindBuffer(GL_COPY_WRITE_BUFFER, target_buffer));
    rhi_opengl_call(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, cmd.copy.source_offset, cmd.copy.target_offset, cmd.copy.size));
  } break;
  case RHI_CommandType_Clear_Color: {
    glClearColor(cmd.clear_color.x, cmd.clear_color.y, cmd.clear_color.z, cmd.clear_color.w);
  } break;
  case RHI_CommandType_Frame_Begin: {
    i32 window_width, window_height;
    OS_Handle window_handle = {{ (u64)hcontext.h[0] }};
    os_window_get_size(window_handle, &window_width, &window_height);
    glViewport(0, 0, window_width, window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RHI_OpenglPrimitive *uniform_buffer = (RHI_OpenglPrimitive *)cmd.frame_begin.uniform_buffer.h[0];
    Assert(uniform_buffer->type == RHI_OpenglPrimitiveType_Buffer);
    rhi_opengl_call(glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer->buffer));
    rhi_opengl_call(glBindBufferRange(GL_UNIFORM_BUFFER, (u32)cmd.frame_begin.binding, uniform_buffer->buffer, 0, cmd.frame_begin.uniform_size));
    rhi_opengl_call(glBufferSubData(GL_UNIFORM_BUFFER, 0, (u32)cmd.frame_begin.uniform_size, cmd.frame_begin.uniform_data));
  } break;
  case RHI_CommandType_Frame_Draw_Index: {
    RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive *)cmd.draw_index.pipeline.h[0];
    Assert(prim->type == RHI_OpenglPrimitiveType_Pipeline);

    RHI_OpenglPrimitive *prim_vertex_buffer = (RHI_OpenglPrimitive*)cmd.draw_index.vertex_buffer.h[0];
    rhi_opengl_call(glBindBuffer(GL_ARRAY_BUFFER, prim_vertex_buffer->buffer));
    RHI_OpenglPrimitive *prim_index_buffer = (RHI_OpenglPrimitive*)cmd.draw_index.index_buffer.h[0];
    rhi_opengl_call(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim_index_buffer->index.buffer));

    rhi_opengl_call(glUseProgram(prim->pipeline.shader));
    rhi_opengl_call(glDrawElements(GL_TRIANGLES, cmd.draw_index.indices_count, GL_UNSIGNED_INT, 0));
  } break;
  case RHI_CommandType_Frame_End: {
    rhi_opengl_context_swap_buffers(hcontext);
  } break;
  }
}

fn void rhi_command_queue_submit(RHI_Handle hcontext) {
  Unused(hcontext);
}

internal u32 rhi_opengl_type_from_shadertype(RHI_ShaderDataType type) {
  switch (type) {
  case RHI_ShaderDataType_Mat4F32:
  case RHI_ShaderDataType_Mat3F32:
  case RHI_ShaderDataType_Vec4F32:
  case RHI_ShaderDataType_Vec3F32:
  case RHI_ShaderDataType_Vec2F32:
  case RHI_ShaderDataType_Float: {
    return GL_FLOAT;
  } break;
  case RHI_ShaderDataType_Vec4I32:
  case RHI_ShaderDataType_Vec3I32:
  case RHI_ShaderDataType_Vec2I32:
  case RHI_ShaderDataType_Int: {
    return GL_INT;
  } break;
  case RHI_ShaderDataType_Bool: {
    return GL_BOOL;
  } break;
  }
  Assert(false);
  return 0;
}

internal RHI_OpenglObj rhi_opengl_shader_program_from_file(String8 filepath, RHI_ShaderType type) {
  RHI_OpenglObj res = {0};
  OS_Handle shader_source = os_fs_open(filepath, OS_AccessFlag_Read);
  {
    Scratch scratch = ScratchBegin(0, 0);
    String8 content = os_fs_read(scratch.arena, shader_source);
    res = rhi_opengl_shader_program_from_str8(content, type);
    ScratchEnd(scratch);
  }
  os_fs_close(shader_source);
  return res;
}

internal RHI_OpenglObj rhi_opengl_shader_program_from_str8(String8 source, RHI_ShaderType type) {
  GLenum gltype = 0;
  switch (type) {
  case RHI_ShaderType_Vertex: {
    gltype = GL_VERTEX_SHADER;
  } break;
  case RHI_ShaderType_Fragment: {
    gltype = GL_FRAGMENT_SHADER;
  } break;
  }

  RHI_OpenglObj shader = glCreateShader(gltype);
  {
    Scratch scratch = ScratchBegin(0, 0);
    const char *cstr = cstr_from_str8(scratch.arena, source);
    rhi_opengl_call(glShaderSource(shader, 1, &cstr, 0));
    rhi_opengl_call(glCompileShader(shader));

    GLint success = 0;
    rhi_opengl_call(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
    Assert(success);
    ScratchEnd(scratch);
  }
  return shader;
}

internal void rhi_opengl_vao_setup(void) {
  RHI_OpenglObj glvao;
  rhi_opengl_call(glGenVertexArrays(1, &glvao));
  rhi_opengl_call(glBindVertexArray(glvao));
}

internal void rhi_opengl_error_clear(void) {
  while (glGetError() != GL_NO_ERROR);
}

internal void rhi_opengl_error_check(void) {
  for (GLenum error; (error = glGetError()); ) {
    switch (error) {
    case GL_INVALID_ENUM: {
      dbg_print("OpenGL error: invalid enum (0x%x)", GL_INVALID_ENUM);
    } break;
    case GL_INVALID_VALUE: {
      dbg_print("OpenGL error: invalid value (0x%x)", GL_INVALID_VALUE);
    } break;
    case GL_INVALID_OPERATION: {
      dbg_print("OpenGL error: invalid operation (0x%x)", GL_INVALID_OPERATION);
    } break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: {
      dbg_print("OpenGL error: invalid framebuffer operation (0x%x)", GL_INVALID_FRAMEBUFFER_OPERATION);
    } break;
    case GL_OUT_OF_MEMORY: {
      dbg_print("OpenGL error: out of memory (0x%x)", GL_OUT_OF_MEMORY);
    } break;
    }
    Assert(false);
  }
}
