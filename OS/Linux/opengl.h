#ifndef BASE_OS_LINUX_OPENGL
#define BASE_OS_LINUX_OPENGL

#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>

// =============================================================================
// Shader functions
typedef u32 (*GL_CREATE_SHADER) (i32);
global GL_CREATE_SHADER glCreateShader;
typedef void (*GL_SHADER_SOURCE) (u32, i32, u8 **, void *);
global GL_SHADER_SOURCE glShaderSource;
typedef void (*GL_COMPILE_SHADER) (u32);
global GL_COMPILE_SHADER glCompileShader;
typedef void (*GL_GET_SHADERIV) (u32, i32, i32 *);
global GL_GET_SHADERIV glGetShaderiv;
typedef void (*GL_GET_SHADER_INFOLOG) (u32, u32, void *, char *);
global GL_GET_SHADER_INFOLOG glGetShaderInfoLog;
typedef void (*GL_ATTACH_SHADER) (u32, u32);
global GL_ATTACH_SHADER glAttachShader;
typedef void (*GL_DELETE_SHADER) (u32);
global GL_DELETE_SHADER glDeleteShader;

// =============================================================================
// Program functions
typedef u32 (*GL_CREATE_PROGRAM) ();
global GL_CREATE_PROGRAM glCreateProgram;
typedef void (*GL_GET_PROGRAMIV) (u32, i32, i32 *);
global GL_GET_PROGRAMIV glGetProgramiv;
typedef void (*GL_GET_PROGRAM_INFOLOG) (u32, u32, void *, char *);
global GL_GET_PROGRAM_INFOLOG glGetProgramInfoLog;
typedef void (*GL_LINK_PROGRAM) (u32);
global GL_LINK_PROGRAM glLinkProgram;
typedef void (*GL_USE_PROGRAM) (u32);
global GL_USE_PROGRAM glUseProgram;

// =============================================================================
// Vertex Array Object (VAO) functions
typedef void (*GL_GEN_VERTEX_ARRAYS) (u32, u32*);
global GL_GEN_VERTEX_ARRAYS glGenVertexArrays;
typedef void (*GL_BIND_VERTEX_ARRAY) (u32);
global GL_BIND_VERTEX_ARRAY glBindVertexArray;
typedef void (*GL_VERTEX_ATTRIB_POINTER) (i32, u32, i32, bool, usz, void*);
global GL_VERTEX_ATTRIB_POINTER glVertexAttribPointer;
typedef void (*GL_ENABLE_VERTEX_ATTRIB_ARRAY) (i32);
global GL_ENABLE_VERTEX_ATTRIB_ARRAY glEnableVertexAttribArray;

// =============================================================================
// Vertex Buffer Object (VBO) functions
typedef void (*GL_GEN_BUFFERS) (u32, u32*);
global GL_GEN_BUFFERS glGenBuffers;
typedef void (*GL_BIND_BUFFER) (i32, u32);
global GL_BIND_BUFFER glBindBuffer;
typedef void (*GL_BUFFER_DATA) (i32, usz, f32*, i32);
global GL_BUFFER_DATA glBufferData;

fn void opengl_init() {
  glCreateShader = (GL_CREATE_SHADER) glXGetProcAddress((u8 *)"glCreateShader");
  glShaderSource = (GL_SHADER_SOURCE) glXGetProcAddress((u8 *)"glShaderSource");
  glCompileShader = (GL_COMPILE_SHADER) glXGetProcAddress((u8 *)"glCompileShader");
  glGetShaderiv = (GL_GET_SHADERIV) glXGetProcAddress((u8 *)"glGetShaderiv");
  glGetShaderInfoLog = (GL_GET_SHADER_INFOLOG) glXGetProcAddress((u8 *)"glGetShaderInfoLog");
  glAttachShader = (GL_ATTACH_SHADER) glXGetProcAddress((u8 *)"glAttachShader");
  glDeleteShader = (GL_DELETE_SHADER) glXGetProcAddress((u8 *)"glDeleteShader");

  glCreateProgram = (GL_CREATE_PROGRAM) glXGetProcAddress((u8 *)"glCreateProgram");
  glGetProgramiv = (GL_GET_PROGRAMIV) glXGetProcAddress((u8 *)"glGetProgramiv");
  glGetProgramInfoLog = (GL_GET_PROGRAM_INFOLOG) glXGetProcAddress((u8 *)"glGetProgramInfoLog");
  glLinkProgram = (GL_LINK_PROGRAM) glXGetProcAddress((u8 *)"glLinkProgram");
  glUseProgram = (GL_USE_PROGRAM) glXGetProcAddress((u8 *)"glUseProgram");

  glGenVertexArrays = (GL_GEN_VERTEX_ARRAYS) glXGetProcAddress((u8 *)"glGenVertexArrays");
  glBindVertexArray = (GL_BIND_VERTEX_ARRAY) glXGetProcAddress((u8 *)"glBindVertexArray");
  glVertexAttribPointer = (GL_VERTEX_ATTRIB_POINTER) glXGetProcAddress((u8 *)"glVertexAttribPointer");
  glEnableVertexAttribArray = (GL_ENABLE_VERTEX_ATTRIB_ARRAY)
  glXGetProcAddress((u8 *)"glEnableVertexAttribArray");

  glGenBuffers = (GL_GEN_BUFFERS) glXGetProcAddress((u8 *)"glGenBuffers");
  glBindBuffer = (GL_BIND_BUFFER) glXGetProcAddress((u8 *)"glBindBuffer");
  glBufferData = (GL_BUFFER_DATA) glXGetProcAddress((u8 *)"glBufferData");
}

#endif
