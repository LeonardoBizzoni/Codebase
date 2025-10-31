#ifndef RHI_OPENGL_UNIX_H
#define RHI_OPENGL_UNIX_H

#include <GL/gl.h>
#include <GL/glx.h>

typedef struct {
  Arena *arena;
  GLXContext glx_context;
} LNX_GL_State;

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
  X(PFNGLUSEPROGRAMPROC, glUseProgram)                             \
  X(PFNGLCREATEPROGRAMPROC, glCreateProgram)

#define X(Type, Name) global Type Name;
GL_FUNCTIONS(X)
#undef X

#endif
