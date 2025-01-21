#ifndef OS_INC_H
#define OS_INC_H

#include "thread.h"
#include "file.h"
#include "dynlib.h"

/* Super temporary */
#if OS_LINUX
#include "Linux/X11/window.h"
#include "Linux/opengl.h"
#elif OS_BSD
#include "BSD/opengl.h"
#include "BSD/X11/window.h"
#endif

#endif
