fn void rhi_opengl_draw(RHI_Handle vertex, RHI_Handle index, RHI_Handle shader) {
  RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive*)index.h[0];
  Assert(prim->type == RHI_OpenglPrimitiveType_Index);

  rhi_opengl_buffer_bind(vertex);
  rhi_opengl_index_bind(index);
  rhi_opengl_shader_bind(shader);
  glDrawElements(GL_TRIANGLES, prim->index.vertex_count, GL_UNSIGNED_INT, 0);
}

fn RHI_Handle rhi_opengl_shader_from_file(String8 vertex_shader_path,
                                          String8 fragment_shader_path) {
  RHI_OpenglObj vertex =
    rhi_opengl_shader_program_from_file(vertex_shader_path,
                                        RHI_ShaderType_Vertex);
  RHI_OpenglObj fragment =
    rhi_opengl_shader_program_from_file(fragment_shader_path,
                                        RHI_ShaderType_Fragment);

  RHI_OpenglObj shader = glCreateProgram();
  glAttachShader(shader, vertex);
  glAttachShader(shader, fragment);
  glLinkProgram(shader);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  RHI_Handle res = {{ (u64)shader }};
  return res;
}

fn RHI_Handle rhi_opengl_shader_from_str8(String8 vertex_shader_code,
                                          String8 fragment_shader_code) {
  RHI_OpenglObj vertex =
    rhi_opengl_shader_program_from_str8(vertex_shader_code,
                                        RHI_ShaderType_Vertex);
  RHI_OpenglObj fragment =
    rhi_opengl_shader_program_from_str8(fragment_shader_code,
                                        RHI_ShaderType_Fragment);

  RHI_OpenglObj shader = glCreateProgram();
  glAttachShader(shader, vertex);
  glAttachShader(shader, fragment);
  glLinkProgram(shader);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  RHI_Handle res = {{ (u64)shader }};
  return res;
}

fn void rhi_opengl_shader_bind(RHI_Handle shader) {
  RHI_OpenglObj glshader = (RHI_OpenglObj)shader.h[0];
  glUseProgram(glshader);
}

fn void rhi_opengl_shader_uniform_set_vec3f32(RHI_Handle shader,
                                              String8 name, Vec3F32 value) {
  RHI_OpenglObj glshader = (RHI_OpenglObj)shader.h[0];
  rhi_opengl_shader_bind(shader);

  {
    Scratch scratch = ScratchBegin(0, 0);
    const char *cstr_name = cstr_from_str8(scratch.arena, name);
    // TODO(lb): cache name->uniform_location?
    i32 location = glGetUniformLocation(glshader, cstr_name);
    Assert(location >= 0);
    glUniform3f(location, value.x, value.y, value.z);
    ScratchEnd(scratch);
  }
}

fn RHI_Handle rhi_opengl_buffer_layout_alloc(Arena *arena) {
  RHI_OpenglPrimitive *prim = rhi_opengl_primitive_alloc(arena, RHI_OpenglPrimitiveType_BufferLayout);
  RHI_Handle res = {{ (u64)prim }};
  return res;
}

fn void rhi_opengl_buffer_layout_push_f32(Arena *arena, RHI_Handle buffer_layout, i32 count) {
  RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive*)buffer_layout.h[0];
  Assert(prim->type == RHI_OpenglPrimitiveType_BufferLayout);
  Assert(count > 0);
  prim->buffer_layout.stride += (u32)count * sizeof(f32);

  struct RHI_OpenglPrimitive_BufferLayoutElement *elem = arena_push(arena, struct RHI_OpenglPrimitive_BufferLayoutElement);
  Assert(elem);
  elem->type = GL_FLOAT;
  elem->count = count;
  elem->size = sizeof(f32);
  elem->to_normalize = GL_FALSE;

  QueuePush(prim->buffer_layout.first, prim->buffer_layout.last, elem);
}

fn RHI_Handle rhi_opengl_buffer_alloc(const void *data, i32 size) {
  RHI_OpenglObj buffer; glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, size, data, data ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
  RHI_Handle res = {{ (u64)buffer }};
  return res;
}

fn void rhi_opengl_buffer_set_layout(RHI_Handle buffer, RHI_Handle buffer_layout) {
  rhi_opengl_buffer_bind(buffer);
  RHI_OpenglPrimitive *prim_buffer_layout = (RHI_OpenglPrimitive*)buffer_layout.h[0];
  Assert(prim_buffer_layout->type == RHI_OpenglPrimitiveType_BufferLayout);

  u32 index = 0;
  usize offset = 0;
  for (struct RHI_OpenglPrimitive_BufferLayoutElement *curr = prim_buffer_layout->buffer_layout.first;
       curr;
       curr = curr->next, index += 1) {
    glVertexAttribPointer(index, curr->count, curr->type, curr->to_normalize,
                          prim_buffer_layout->buffer_layout.stride,
                          (const void*)offset);
    glEnableVertexAttribArray(index);
    offset += (u32)curr->count * curr->size;
  }
}

fn void rhi_opengl_buffer_copy_data(RHI_Handle buffer, const void *data, i32 size) {
  rhi_opengl_buffer_bind(buffer);
  glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

fn void rhi_opengl_buffer_bind(RHI_Handle buffer) {
  RHI_OpenglObj prim = (RHI_OpenglObj)buffer.h[0];
  glBindBuffer(GL_ARRAY_BUFFER, prim);
}

fn RHI_Handle rhi_opengl_index_alloc(Arena *arena, const i32 *data, i32 vertex_count) {
  RHI_OpenglPrimitive *prim = rhi_opengl_primitive_alloc(arena, RHI_OpenglPrimitiveType_Index);
  prim->index.vertex_count = vertex_count;
  glGenBuffers(1, &prim->index.id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim->index.id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, (u32)vertex_count * sizeof(*data),
               data, GL_STATIC_DRAW);
  RHI_Handle res = {{ (u64)prim }};
  return res;
}

fn void rhi_opengl_index_bind(RHI_Handle index) {
  RHI_OpenglPrimitive *prim = (RHI_OpenglPrimitive*)index.h[0];
  Assert(prim->type == RHI_OpenglPrimitiveType_Index);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim->index.id);
}


internal RHI_OpenglPrimitive*
rhi_opengl_primitive_alloc(Arena *arena, RHI_OpenglPrimitiveType type) {
  RHI_OpenglPrimitive *prim = arena_push(arena, RHI_OpenglPrimitive);
  Assert(prim);
  prim->type = type;
  return prim;
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
  case RHI_ShaderType_Fragment: {
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
