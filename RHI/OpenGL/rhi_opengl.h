#ifndef RHI_OPENGL_CORE_H
#define RHI_OPENGL_CORE_H

#include <GL/gl.h>

typedef u32 RHI_OpenglObj;

typedef u8 RHI_OpenglPrimitiveType;
enum {
  RHI_OpenglPrimitiveType_None,
  RHI_OpenglPrimitiveType_Buffer,
  RHI_OpenglPrimitiveType_Index,
  RHI_OpenglPrimitiveType_Pipeline,
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
    struct {
      RHI_OpenglObj shader;
      RHI_BufferLayoutElement *layout;
      i64 layout_elements_count;
    } pipeline;
  };
} RHI_OpenglPrimitive;

#define rhi_opengl_call(X)    \
  do {                        \
    rhi_opengl_error_clear(); \
    X;                        \
    rhi_opengl_error_check(); \
  } while (0)

// Platform dependent
fn void rhi_opengl_context_set_active(RHI_Handle handle);
fn void rhi_opengl_context_swap_buffers(RHI_Handle hcontext);

internal u32 rhi_opengl_type_from_shadertype(RHI_ShaderDataType type);

internal RHI_OpenglObj rhi_opengl_shader_program_from_file(String8 filepath, RHI_ShaderType type);
internal RHI_OpenglObj rhi_opengl_shader_program_from_str8(String8 source, RHI_ShaderType type);

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
