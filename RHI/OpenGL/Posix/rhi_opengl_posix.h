#ifndef RHI_OPENGL_UNIX_H
#define RHI_OPENGL_UNIX_H

#include <GL/glx.h>

typedef struct {
  Arena *arena;
  GLXContext glx_context;
} RHI_Opengl_Posix_State;

#endif
