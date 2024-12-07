#ifndef BASE_INC_H
#define BASE_INC_H

#include "base.h"
#include "memory.h"
#include "arena.h"
#include "list.h"
#include "chrono.h"
#include "clock.h"
#include "string.h"

#include "OS/dynlib.h"
#include "OS/file_properties.h"
#include "OS/file.h"

#if OS_LINUX
  #include "OS/Linux/thread.h"

  #if GFX_X11
    #include "OS/Linux/X11/window.h"
  #elif GFX_WAYLAND
    #include "OS/Linux/Wayland/window.h"
  #endif
#elif OS_BSD
  #include "OS/BSD/thread.h"

  #if GFX_X11
    #include "OS/BSD/X11/window.h"
  #elif GFX_WAYLAND
    #include "OS/BSD/Wayland/window.h"
  #endif
#elif OS_WINDOWS
  #include "OS/Windows/thread.h"
  #include "OS/Windows/D3D/window.h"
#endif

#include "serializer/csv.h"

#include "AI/decision_tree.h"

#endif