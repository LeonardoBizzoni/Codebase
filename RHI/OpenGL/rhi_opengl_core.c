fn RHI_Handle rhi_shader_from_file(Arena *arena, RHI_Handle hcontext,
                                   String8 vertex_shader_path,
                                   String8 fragment_shader_path) {
  Unused(arena); Unused(hcontext);
  RHI_OpenglObj vertex =
    rhi_opengl_shader_program_from_file(vertex_shader_path,
                                        RHI_ShaderType_Vertex);
  RHI_OpenglObj fragment =
    rhi_opengl_shader_program_from_file(fragment_shader_path,
                                        RHI_ShaderType_Pixel);

  RHI_OpenglObj shader = glCreateProgram();
  glAttachShader(shader, vertex);
  glAttachShader(shader, fragment);
  glLinkProgram(shader);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  RHI_Handle res = {{ (u64)shader }};
  return res;
}

fn RHI_Handle rhi_buffer_alloc(Arena *arena, RHI_Handle hcontext,
                               i32 size, RHI_BufferType type) {
  Unused(hcontext);
  RHI_OpenglPrimitive *prim = arena_push(arena, RHI_OpenglPrimitive);
  Assert(prim);
  switch (type) {
  case RHI_BufferType_Staging: {
    prim->type = RHI_OpenglPrimitiveType_Buffer;
    glGenBuffers(1, &prim->buffer);
    glBindBuffer(GL_COPY_WRITE_BUFFER, prim->buffer);
    glBufferData(GL_COPY_WRITE_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
  } break;
  case RHI_BufferType_Vertex: {
    prim->type = RHI_OpenglPrimitiveType_Buffer;
    glGenBuffers(1, &prim->buffer);
    glBindBuffer(GL_ARRAY_BUFFER, prim->buffer);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
  } break;
  case RHI_BufferType_Index: {
    prim->type = RHI_OpenglPrimitiveType_Index;
    prim->index.vertex_count = (u32)size / sizeof(i32);
    glGenBuffers(1, &prim->index.buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim->index.buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
  } break;
  }
  RHI_Handle res = {{ (u64)prim }};
  return res;
}

fn void rhi_buffer_host_send(RHI_Handle hcontext, RHI_Handle hbuffer,
                             const void *data, i32 size) {
  Unused(hcontext);
  RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive*)hbuffer.h[0];
  Assert(prim->type == RHI_OpenglPrimitiveType_Buffer ||
         prim->type == RHI_OpenglPrimitiveType_Index);
  RHI_OpenglObj buffer = prim->buffer;
  glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
  void *buffer_data = glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, size,
                                       GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  Assert(buffer_data);
  memcopy(buffer_data, data, size);
  glUnmapBuffer(GL_COPY_WRITE_BUFFER);
}

fn void rhi_buffer_copy(RHI_Handle hcontext, RHI_Handle htarget_buffer,
                        RHI_Handle hsource_buffer, i32 size) {
  Unused(hcontext);

  RHI_OpenglPrimitive *target_prim = (RHI_OpenglPrimitive*)htarget_buffer.h[0];
  RHI_OpenglPrimitive *source_prim = (RHI_OpenglPrimitive*)hsource_buffer.h[0];
  Assert(target_prim->type == RHI_OpenglPrimitiveType_Buffer ||
         target_prim->type == RHI_OpenglPrimitiveType_Index);
  Assert(source_prim->type == RHI_OpenglPrimitiveType_Buffer ||
         source_prim->type == RHI_OpenglPrimitiveType_Index);

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

  glBindBuffer(GL_COPY_READ_BUFFER, source_buffer);
  glBindBuffer(GL_COPY_WRITE_BUFFER, target_buffer);
  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, size);
}

fn void rhi_draw(RHI_Handle hcontext, RHI_Handle vertex,
                 RHI_Handle index, RHI_Handle shader) {
  Unused(hcontext);
  RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive*)index.h[0];
  Assert(prim->type == RHI_OpenglPrimitiveType_Index);

  rhi_opengl_buffer_bind(vertex);
  rhi_opengl_index_bind(index);
  rhi_opengl_shader_bind(shader);
  glDrawElements(GL_TRIANGLES, prim->index.vertex_count, GL_UNSIGNED_INT, 0);
}

internal void _rhi_buffer_set_layout(RHI_Handle hcontext, RHI_Handle hbuffer,
                                     RHI_BufferElement *layout, u32 layout_size) {
  Unused(hcontext);
  rhi_opengl_buffer_bind(hbuffer);

  i32 stride = 0;
  for (u32 i = 0; i < layout_size; ++i) {
    stride += rhi_opengl_size_from_shadertype(layout[i].type);
  }

  isize offset = 0;
  for (u32 i = 0; i < layout_size; ++i) {
    glEnableVertexAttribArray(i);
    glVertexAttribPointer(i, rhi_opengl_count_from_shadertype(layout[i].type),
                          rhi_opengl_type_from_shadertype(layout[i].type),
                          layout[i].to_normalize ? GL_TRUE : GL_FALSE,
                          stride, (const void*)offset);
    offset += rhi_opengl_size_from_shadertype(layout[i].type);
  }
}

internal i32 rhi_opengl_size_from_shadertype(RHI_ShaderDataType type) {
  switch (type) {
  case RHI_ShaderDataType_Float: {
    return sizeof(f32);
  } break;
  case RHI_ShaderDataType_Int: {
    return sizeof(i32);
  } break;
  case RHI_ShaderDataType_Bool: {
    return sizeof(bool);
  } break;
  case RHI_ShaderDataType_Vec2F32: {
    return 2 * sizeof(f32);
  } break;
  case RHI_ShaderDataType_Vec2I32: {
    return 2 * sizeof(i32);
  } break;
  case RHI_ShaderDataType_Vec3F32: {
    return 3 * sizeof(f32);
  } break;
  case RHI_ShaderDataType_Vec3I32: {
    return 3 * sizeof(i32);
  } break;
  case RHI_ShaderDataType_Vec4F32: {
    return 4 * sizeof(f32);
  } break;
  case RHI_ShaderDataType_Vec4I32: {
    return 4 * sizeof(i32);
  } break;
  case RHI_ShaderDataType_Mat3F32: {
    return 3 * 3 * sizeof(f32);
  } break;
  case RHI_ShaderDataType_Mat4F32: {
    return 4 * 4 * sizeof(f32);
  } break;
  }
  Assert(false);
  return 0;
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

internal i32 rhi_opengl_count_from_shadertype(RHI_ShaderDataType type) {
  switch (type) {
  case RHI_ShaderDataType_Vec4F32:
  case RHI_ShaderDataType_Vec4I32: {
    return 4;
  } break;
  case RHI_ShaderDataType_Mat4F32: {
    return 4 * 4;
  } break;
  case RHI_ShaderDataType_Mat3F32: {
    return 3 * 3;
  } break;
  case RHI_ShaderDataType_Vec3I32:
  case RHI_ShaderDataType_Vec3F32: {
    return 3;
  } break;
  case RHI_ShaderDataType_Vec2I32:
  case RHI_ShaderDataType_Vec2F32: {
    return 2;
  } break;
  case RHI_ShaderDataType_Float:
  case RHI_ShaderDataType_Bool:
  case RHI_ShaderDataType_Int: {
    return 1;
  } break;
  }
  Assert(false);
  return 0;
}

internal RHI_Handle rhi_opengl_shader_from_str8(String8 vertex_shader_code,
                                                String8 fragment_shader_code) {
  RHI_OpenglObj vertex =
    rhi_opengl_shader_program_from_str8(vertex_shader_code,
                                        RHI_ShaderType_Vertex);
  RHI_OpenglObj fragment =
    rhi_opengl_shader_program_from_str8(fragment_shader_code,
                                        RHI_ShaderType_Pixel);

  RHI_OpenglObj shader = glCreateProgram();
  glAttachShader(shader, vertex);
  glAttachShader(shader, fragment);
  glLinkProgram(shader);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  RHI_Handle res = {{ (u64)shader }};
  return res;
}

internal void rhi_opengl_shader_bind(RHI_Handle shader) {
  RHI_OpenglObj glshader = (RHI_OpenglObj)shader.h[0];
  glUseProgram(glshader);
}

internal void rhi_opengl_buffer_bind(RHI_Handle hbuffer) {
  RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive*)hbuffer.h[0];
  glBindBuffer(GL_ARRAY_BUFFER, prim->buffer);
}

internal void rhi_opengl_index_bind(RHI_Handle index) {
  RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive*)index.h[0];
  Assert(prim->type == RHI_OpenglPrimitiveType_Index);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim->index.buffer);
}


internal RHI_OpenglObj
rhi_opengl_shader_program_from_file(String8 filepath, RHI_ShaderType type) {
  RHI_OpenglObj res = {0};
  OS_Handle shader_source = os_fs_open(filepath, OS_acfRead);
  {
    Scratch scratch = ScratchBegin(0, 0);
    String8 content = os_fs_read(scratch.arena, shader_source);
    res = rhi_opengl_shader_program_from_str8(content, type);
    ScratchEnd(scratch);
  }
  os_fs_close(shader_source);
  return res;
}

internal RHI_OpenglObj
rhi_opengl_shader_program_from_str8(String8 source, RHI_ShaderType type) {
  GLenum gltype = 0;
  switch (type) {
  case RHI_ShaderType_Vertex: {
    gltype = GL_VERTEX_SHADER;
  } break;
  case RHI_ShaderType_Pixel: {
    gltype = GL_FRAGMENT_SHADER;
  } break;
  }

  RHI_OpenglObj shader = glCreateShader(gltype);
  {
    Scratch scratch = ScratchBegin(0, 0);
    const char *cstr = cstr_from_str8(scratch.arena, source);
    glShaderSource(shader, 1, &cstr, 0);
    glCompileShader(shader);
    ScratchEnd(scratch);
  }
  return shader;
}

internal void rhi_opengl_vao_setup(void) {
  RHI_OpenglObj glvao;
  glGenVertexArrays(1, &glvao);
  glBindVertexArray(glvao);
}
