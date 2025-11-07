#ifndef OS_GFX_POSIX_H
#define OS_GFX_POSIX_H

#undef internal
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include <X11/extensions/Xrandr.h>
#define internal static

typedef struct OS_GFX_Posix_Window {
  struct OS_GFX_Posix_Window *next;
  struct OS_GFX_Posix_Window *prev;
  Window xwindow;
  GC xgc;
} OS_GFX_Posix_Window;

typedef struct {
  OS_GFX_Posix_Window *first_window;
  OS_GFX_Posix_Window *last_window;
  OS_GFX_Posix_Window *freelist_window;

  Display *xdisplay;
  XVisualInfo xvisual;
  i32 xscreen;
  Window xroot;

  XRRMonitorInfo *xmonitors;
  i32 xmonitors_count;

  Atom xatom_close;
  Atom xatom_change_state;
  Atom xatom_active_window;
  Atom xatom_state;
  Atom xatom_state_hidden;
  Atom xatom_state_fullscreen;
  Atom xatom_state_maximized_horz;
  Atom xatom_state_maximized_vert;
  Atom xatom_motif_wm_hints;
} OS_GFX_Posix_State;

typedef struct {
  u64 flags;
  u64 functions;
  u64 decorations;
  u64 input_mode;
  u64 status;
} OS_GFX_Posix_MotifWMHint;
StaticAssert(sizeof(OS_GFX_Posix_MotifWMHint) == 5 * sizeof(long), invalid_motif_struct);

internal void os_gfx_init(void);
internal void os_gfx_deinit(void);

internal XRRMonitorInfo* os_gfx_posix_monitor_from_window(OS_GFX_Posix_Window *window);
internal bool os_posix_window_check_xatom(OS_GFX_Posix_Window *window, Atom target);
internal OS_GFX_Posix_Window* os_posix_window_from_xwindow(Window xwindow);
internal KeySym os_posix_keysym_from_os_key(OS_Key key);

#endif
