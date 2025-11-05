#ifndef RHI_OPENGL_CORE_H
#define RHI_OPENGL_CORE_H

#include <GL/gl.h>

typedef u32 RHI_OpenglObj;

typedef u8 RHI_OpenglPrimitiveType;
enum {
  RHI_OpenglPrimitiveType_None,
  RHI_OpenglPrimitiveType_Buffer,
  RHI_OpenglPrimitiveType_Index,
  RHI_OpenglPrimitiveType_COUNT,
};

typedef struct {
  RHI_OpenglPrimitiveType type;
  union {
    RHI_OpenglObj buffer;
    struct {
      RHI_OpenglObj buffer;
      i32 vertex_count;
    } index;
  };
} RHI_OpenglPrimitive;

#define rhi_opengl_call(X)    \
  do {                        \
    rhi_opengl_error_clear(); \
    X;                        \
    rhi_opengl_error_check(); \
  } while (0)

fn void rhi_opengl_context_set_active(RHI_Handle handle);
fn void rhi_opengl_context_commit(RHI_Handle handle);
fn void rhi_opengl_context_destroy(RHI_Handle handle);

fn void rhi_opengl_draw(RHI_Handle hcontext, RHI_Handle vertex,
                        RHI_Handle index, RHI_Handle shader);

fn RHI_Handle rhi_opengl_shader_from_file(String8 vertex_shader_path,
                                          String8 fragment_shader_path);
fn RHI_Handle rhi_opengl_shader_from_str8(String8 vertex_shader_code,
                                          String8 fragment_shader_code);
fn void rhi_opengl_shader_bind(RHI_Handle shader);
fn void rhi_opengl_shader_uniform_set_vec3f32(RHI_Handle shader,
                                              String8 name, Vec3F32 value);

fn RHI_Handle rhi_opengl_buffer_layout_alloc(Arena *arena);
fn void rhi_opengl_buffer_layout_push_f32(Arena *arena, RHI_Handle buffer_layout, i32 count);

fn RHI_Handle rhi_opengl_buffer_alloc(const void *data, i32 size);
fn void rhi_opengl_buffer_set_layout(RHI_Handle buffer, RHI_Handle buffer_layout);
fn void rhi_opengl_buffer_copy_data(RHI_Handle buffer, const void *data, i32 size);
fn void rhi_opengl_buffer_bind(RHI_Handle buffer);

fn RHI_Handle rhi_opengl_index_alloc(Arena *arena, const i32 *data, i32 vertex_count);
fn void rhi_opengl_index_bind(RHI_Handle index);


internal RHI_OpenglPrimitive*
rhi_opengl_primitive_alloc(Arena *arena, RHI_OpenglPrimitiveType type);
internal RHI_OpenglObj
rhi_opengl_shader_program_from_file(String8 filepath, RHI_ShaderType type);
internal RHI_OpenglObj
rhi_opengl_shader_program_from_str8(String8 source, RHI_ShaderType type);

internal u32 rhi_opengl_type_from_shadertype(RHI_ShaderDataType type);

internal void rhi_opengl_vao_setup(void);

internal void rhi_opengl_error_clear(void);
internal void rhi_opengl_error_check(void);

#define GL_FUNCTIONS(X)                                            \
  X(PFNGLGENBUFFERSPROC, glGenBuffers)                             \
  X(PFNGLBINDBUFFERPROC, glBindBuffer)                             \
  X(PFNGLBUFFERDATAPROC, glBufferData)                             \
  X(PFNGLMAPBUFFERRANGEPROC, glMapBufferRange)                     \
  X(PFNGLUNMAPBUFFERPROC, glUnmapBuffer)                           \
  X(PFNGLCOPYBUFFERSUBDATAPROC, glCopyBufferSubData)               \
  X(PFNGLCREATEBUFFERSPROC, glCreateBuffers)                       \
                                                                   \
  X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                   \
  X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                   \
  X(PFNGLCREATEVERTEXARRAYSPROC, glCreateVertexArrays)             \
  X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)           \
  X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)   \
                                                                   \
  X(PFNGLCREATESHADERPROC, glCreateShader)                         \
  X(PFNGLDELETESHADERPROC, glDeleteShader)                         \
  X(PFNGLATTACHSHADERPROC, glAttachShader)                         \
  X(PFNGLLINKPROGRAMPROC, glLinkProgram)                           \
  X(PFNGLSHADERSOURCEPROC, glShaderSource)                         \
  X(PFNGLCOMPILESHADERPROC, glCompileShader)                       \
  X(PFNGLGETSHADERIVPROC, glGetShaderiv)                           \
  X(PFNGLUSEPROGRAMPROC, glUseProgram)                             \
  X(PFNGLCREATEPROGRAMPROC, glCreateProgram)                       \
                                                                   \
  X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)             \
  X(PFNGLUNIFORM3FPROC, glUniform3f)                               \
  X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv)

#define X(Type, Name) global Type Name;
GL_FUNCTIONS(X)
#undef X

#endif
