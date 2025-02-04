#include <X11/Xlib.h>

global LNX11_State lnx11_state = {0};

fn void lnx_gfx_init() {
  lnx11_state.arena = ArenaBuild();
  lnx11_state.xdisplay = XOpenDisplay(0);

  lnx11_state.xatom_close = XInternAtom(lnx11_state.xdisplay, "WM_DELETE_WINDOW", False);
}

fn OS_Handle os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height) {
  LNX11_Window *window = lnx11_state.freelist_window;
  if (window) {
    memZero(window, sizeof(LNX11_Window));
    StackPop(lnx11_state.freelist_window);
  } else {
    window = New(lnx11_state.arena, LNX11_Window);
  }
  DLLPushBack(lnx11_state.first_window, lnx11_state.last_window, window);

  window->name = name;
  window->width = width;
  window->height = height;

  XSetWindowAttributes attr = { .event_mask = ExposureMask | FocusChangeMask |
                                              EnterWindowMask | LeaveWindowMask |
                                              ButtonPressMask | PointerMotionMask |
                                              KeyPressMask | KeymapStateMask };
  window->xwindow = XCreateWindow(lnx11_state.xdisplay,
				  XDefaultRootWindow(lnx11_state.xdisplay),
				  x, y, width, height,
				  0,  CopyFromParent,
				  InputOutput, CopyFromParent,
				  CWEventMask, &attr);
  XSetWMProtocols(lnx11_state.xdisplay, window->xwindow,
		  &lnx11_state.xatom_close, 1);

  Scratch scratch = ScratchBegin(&lnx11_state.arena, 1);
  XStoreName(lnx11_state.xdisplay, window->xwindow,
	     cstrFromStr8(scratch.arena, name));
  ScratchEnd(scratch);

  OS_Handle res = {(u64)window};
  return res;
}

fn void os_window_show(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  XMapWindow(lnx11_state.xdisplay, window->xwindow);
}

fn void os_window_hide(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  XUnmapWindow(lnx11_state.xdisplay, window->xwindow);
}

fn void os_window_close(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  XUnmapWindow(lnx11_state.xdisplay, window->xwindow);
  XDestroyWindow(lnx11_state.xdisplay, window->xwindow);
  StackPush(lnx11_state.freelist_window, window);
}
