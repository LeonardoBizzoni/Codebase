// wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.c
#include "xdg-shell-protocol.c"

global Wayland_State waystate = {0};

// =============================================================================
// Common Linux definitions
fn void lnx_gfx_init(void) {
  waystate.arena = ArenaBuild();
  waystate.display = wl_display_connect(0);
  Assert(waystate.display);

  waystate.registry = wl_display_get_registry(waystate.display);
  Assert(waystate.registry);

  waystate.xdg_base_listener.ping = way_xdg_base_ping;

#undef global
  waystate.reglistener.global = way_registry_handle_global;
  waystate.reglistener.global_remove = way_registry_handle_global_remove;
#define global static
  wl_registry_add_listener(waystate.registry, &waystate.reglistener, 0);

  wl_display_roundtrip(waystate.display);
}

// =============================================================================
// Wayland specific definitions
fn void way_registry_handle_global(void *data, struct wl_registry *registry,
                                   u32 name, const char *interface, u32 version) {
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    waystate.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    waystate.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    waystate.xdg_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(waystate.xdg_base, &waystate.xdg_base_listener, 0);
  }
}

fn void way_registry_handle_global_remove(void *data, struct wl_registry *registry, u32 name) {}

fn void way_xdg_base_ping(void *data, struct xdg_wm_base *xdg_base, u32 serial) {
  xdg_wm_base_pong(xdg_base, serial);
}

fn void way_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, u32 serial) {
   xdg_surface_ack_configure(xdg_surface, serial);
}

fn void way_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                   i32 width, i32 height, struct wl_array *states) {
}

fn void way_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
}

// =============================================================================
// OS agnostic definitions
fn OS_Window os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height) {
  Wayland_Window *window = waystate.freelist_window;
  if (window) {
    memzero(window, sizeof(Wayland_Window));
    StackPop(waystate.freelist_window);
  } else {
    window = New(waystate.arena, Wayland_Window);
  }
  DLLPushBack(waystate.first_window, waystate.last_window, window);

  u32 memsize = width * height * 4;
  window->surface = wl_compositor_create_surface(waystate.compositor);

  window->shm = os_sharedmem_open(name, memsize,
                                  OS_acfRead | OS_acfWrite | OS_acfExecute);
  struct wl_shm_pool *pool = wl_shm_create_pool(waystate.shm, window->shm.file_handle.h[0], memsize);
  window->buffer = wl_shm_pool_create_buffer(pool, 0, width, height, width * 4, WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);

  window->xdg_surface = xdg_wm_base_get_xdg_surface(waystate.xdg_base, window->surface);
  window->xdg_surface_listener.configure = way_xdg_surface_configure;
  xdg_surface_add_listener(window->xdg_surface, &window->xdg_surface_listener, 0);

  window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
  window->xdg_toplevel_listener.configure = way_xdg_toplevel_configure;
  window->xdg_toplevel_listener.close = way_xdg_toplevel_close;
  xdg_toplevel_add_listener(window->xdg_toplevel, &window->xdg_toplevel_listener, 0);

  Scratch scratch = ScratchBegin(0, 0);
  xdg_toplevel_set_title(window->xdg_toplevel, cstr_from_str8(scratch.arena, name));
  ScratchEnd(scratch);

  wl_surface_attach(window->surface, window->buffer, x, y);
  wl_surface_damage(window->surface, 0, 0, width, height);

  OS_Window res = {0};
  res.handle.h[0] = (u64)window;
  return res;
}

fn void os_window_show(OS_Window window_) {
  Wayland_Window *window = (Wayland_Window*)window_.handle.h[0];
  wl_surface_commit(window->surface);

  // TODO(lb): needs to stay running to enable events i think.
  //           probably needs its own thread just like on Win32
  while (wl_display_dispatch(waystate.display));
}

fn void os_window_hide(OS_Window handle) {}

fn void os_window_close(OS_Window window_) {
  Wayland_Window *window = (Wayland_Window*)window_.handle.h[0];
  wl_surface_destroy(window->surface);
  // TODO(lb): figure out what else needs to be destroyed and how to do it

  DLLDelete(waystate.first_window, waystate.last_window, window);
  StackPush(waystate.freelist_window, window);
}

fn void os_window_swapBuffers(OS_Window handle) {}

fn void os_window_render(OS_Window window_, void *mem) {}

fn OS_Event os_window_get_event(OS_Window handle) {
  OS_Event res = {0};
  return res;
}

fn OS_Event os_window_wait_event(OS_Window handle) {
  OS_Event res = {0};
  wl_display_dispatch(waystate.display);
  return res;
}

fn String8 os_keyname_from_event(Arena *arena, OS_Event event) {
  String8 res = {0};
  return res;
}

#if USING_OPENGL
fn void opengl_init(OS_Window handle) {}

fn void opengl_deinit(OS_Window handle) {}

fn void opengl_make_current(OS_Window handle) {}
#endif
