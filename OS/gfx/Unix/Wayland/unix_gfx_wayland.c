// wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.c
#include "xdg-shell-protocol.c"

global Wayland_State waystate = {0};

// =============================================================================
// Common Linux definitions
fn void unx_gfx_init(void) {
  waystate.arena = ArenaBuild();
  waystate.events.lock = os_mutex_alloc();

  waystate.display = wl_display_connect(0);
  Assert(waystate.display);
  waystate.registry = wl_display_get_registry(waystate.display);
  Assert(waystate.registry);

#undef global
  waystate.reglistener.global = way_registry_handle_global;
  waystate.reglistener.global_remove = way_registry_handle_global_remove;
  waystate.callback_listener.done = way_callback_frame_new;
  waystate.xdg_base_listener.ping = way_xdg_base_ping;
  waystate.xdg_surface_listener.configure = way_xdg_surface_configure;
  waystate.xdg_toplevel_listener.configure = way_xdg_toplevel_configure;
  waystate.xdg_toplevel_listener.close = way_xdg_toplevel_close;
#define global static

  wl_registry_add_listener(waystate.registry, &waystate.reglistener, 0);
  wl_display_roundtrip(waystate.display);
}

fn void unx_gfx_deinit(void) {
  os_thread_cancel(waystate.dispatcher);
  wl_display_disconnect(waystate.display);
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

fn void way_callback_frame_new(void *data, struct wl_callback *callback, u32 callback_data) {
  Wayland_Window *window = data;
  wl_callback_destroy(callback);
  callback = wl_surface_frame(window->surface);
  wl_callback_add_listener(callback, &waystate.callback_listener, window);
}

fn void way_xdg_base_ping(void *data, struct xdg_wm_base *xdg_base, u32 serial) {
  xdg_wm_base_pong(xdg_base, serial);
}

fn void way_shm_create(Wayland_Window *window) {
  u32 memsize = window->width * window->height * 4;

  Scratch scratch = ScratchBegin(0, 0);
  StringStream ss = {0};
  strstream_append_str(scratch.arena, &ss, Strlit("/"));
  strstream_append_str(scratch.arena, &ss, str8_from_i64(scratch.arena, Random(1000000, 9999999)));
  OS_MutexScope(window->shmlock) {
    os_sharedmem_close(&window->shm);
    window->shm = os_sharedmem_open(str8_from_stream(scratch.arena, ss), memsize,
                                    OS_acfRead | OS_acfWrite | OS_acfExecute);
    os_sharedmem_unlink_name(&window->shm);

    struct wl_shm_pool *pool = wl_shm_create_pool(waystate.shm, window->shm.file_handle.h[0], memsize);
    window->buffer = wl_shm_pool_create_buffer(pool, 0, window->width, window->height, window->width * 4, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);

    window->shm.content = mmap(0, memsize, PROT_READ | PROT_WRITE,
                               MAP_SHARED, window->shm.file_handle.h[0], 0);
  }
  Assert(window->shm.content != MAP_FAILED);
  close(window->shm.file_handle.h[0]);
  ScratchEnd(scratch);
}

// TODO(lb): not sure what this event does
// https://wayland.app/protocols/xdg-shell#xdg_surface:event:configure
fn void way_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, u32 serial) {
  xdg_surface_ack_configure(xdg_surface, serial);

  Wayland_Window *window = data;
  if (!window->shm.content) { way_shm_create(window); }
  wl_surface_attach(window->surface, window->buffer, 0, 0);
  wl_surface_damage(window->surface, 0, 0, window->width, window->height);
  wl_surface_commit(window->surface);
}

// https://wayland.app/protocols/xdg-shell#xdg_toplevel:event:configure
fn void way_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                   i32 width, i32 height, struct wl_array *states) {
  Wayland_Window *window = data;
  if (!(width && height) || (window->width == width &&
                             window->height == height)) { return; }

  window->width = width;
  window->height = height;
  way_shm_create(window);

  Way_WindowEvent *event = way_alloc_windowevent();
  event->value.type = OS_EventType_Expose;
  event->value.expose.width = width;
  event->value.expose.height = height;
  OS_MutexScope(window->events.lock) {
    QueuePush(window->events.list.first, window->events.list.last, event);
    os_cond_signal(window->events.condvar);
  }
}

// https://wayland.app/protocols/xdg-shell#xdg_toplevel:event:close
fn void way_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
  Wayland_Window *window = data;
  Way_WindowEvent *event = way_alloc_windowevent();
  event->value.type = OS_EventType_Kill;
  OS_MutexScope(window->events.lock) {
    QueuePush(window->events.list.first, window->events.list.last, event);
    os_cond_signal(window->events.condvar);
  }
}

fn void way_event_dispatcher(void *_) {
  while (1) {
    wl_display_flush(waystate.display);
    wl_display_dispatch(waystate.display);
    os_thread_cancelpoint();
  }
}

fn Way_WindowEvent* way_alloc_windowevent(void) {
  Way_WindowEvent *event = 0;
  OS_MutexScope(waystate.events.lock) {
    event = waystate.events.freelist.first;
    if (event) {
      QueuePop(waystate.events.freelist.first);
      memzero(event, sizeof(Way_WindowEvent));
    } else {
      event = New(waystate.arena, Way_WindowEvent);
    }
  }
  Assert(event);
  return event;
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
  window->shmlock = os_mutex_alloc();
  window->events.lock = os_mutex_alloc();
  window->events.condvar = os_cond_alloc();
  window->width = width;
  window->height = height;

  window->surface = wl_compositor_create_surface(waystate.compositor);
  struct wl_callback *callback = wl_surface_frame(window->surface);
  wl_callback_add_listener(callback, &waystate.callback_listener, window);

  window->xdg_surface = xdg_wm_base_get_xdg_surface(waystate.xdg_base, window->surface);
  xdg_surface_add_listener(window->xdg_surface, &waystate.xdg_surface_listener, window);
  window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
  xdg_toplevel_add_listener(window->xdg_toplevel, &waystate.xdg_toplevel_listener, window);
  {
    Scratch scratch = ScratchBegin(0, 0);
    xdg_toplevel_set_title(window->xdg_toplevel, cstr_from_str8(scratch.arena, name));
    ScratchEnd(scratch);
  }

  // TODO(lb): should be moved in the `init`
  waystate.dispatcher = os_thread_start(way_event_dispatcher, 0);

  OS_Window res = {0};
  res.handle.h[0] = (u64)window;
  res.width = width;
  res.height = height;
  return res;
}

fn void os_window_show(OS_Window window_) {
  Wayland_Window *window = (Wayland_Window*)window_.handle.h[0];
  wl_surface_commit(window->surface);
}

fn void os_window_hide(OS_Window handle) {}

fn void os_window_close(OS_Window window_) {
  Wayland_Window *window = (Wayland_Window*)window_.handle.h[0];
  if (window->buffer) { wl_buffer_destroy(window->buffer); }
  xdg_toplevel_destroy(window->xdg_toplevel);
  xdg_surface_destroy(window->xdg_surface);
  wl_surface_destroy(window->surface);
  os_sharedmem_close(&window->shm);

  DLLDelete(waystate.first_window, waystate.last_window, window);
  StackPush(waystate.freelist_window, window);
}

fn void os_window_swapBuffers(OS_Window handle) {}

fn void os_window_render(OS_Window window_, void *mem) {
  Wayland_Window *window = (Wayland_Window*)window_.handle.h[0];
  u32 width = Min(window_.width, window->width);
  u32 height = Min(window_.height, window->height);
  OS_MutexScope(window->shmlock) {
    memcopy(window->shm.content, mem, 4 * width * height);
  }

  wl_surface_attach(window->surface, window->buffer, 0, 0);
  wl_surface_damage(window->surface, 0, 0, width, height);
  wl_surface_commit(window->surface);
}

fn OS_Event os_window_get_event(OS_Window window_) {
  OS_Event res = {0};
  Way_WindowEvent *event = 0;
  Wayland_Window *window = (Wayland_Window*)window_.handle.h[0];
  OS_MutexScope(window->events.lock) {
    event = window->events.list.first;
    QueuePop(window->events.list.first);
  }

  if (event) {
    memcopy(&res, &event->value, sizeof(OS_Event));
    OS_MutexScope(waystate.events.lock) {
      QueuePush(waystate.events.freelist.first, waystate.events.freelist.last, event);
    }
  }
  return res;
}

fn OS_Event os_window_wait_event(OS_Window window_) {
  OS_Event res = {0};
  Way_WindowEvent *event = 0;
  Wayland_Window *window = (Wayland_Window*)window_.handle.h[0];
  OS_MutexScope(window->events.lock) {
    event = window->events.list.first;
    for (; !event; event = window->events.list.first) {
      os_cond_wait(window->events.condvar, window->events.lock, 0);
    }
    QueuePop(window->events.list.first);
  }

  memcopy(&res, &event->value, sizeof(OS_Event));
  OS_MutexScope(waystate.events.lock) {
    QueuePush(waystate.events.freelist.first, waystate.events.freelist.last, event);
  }
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
