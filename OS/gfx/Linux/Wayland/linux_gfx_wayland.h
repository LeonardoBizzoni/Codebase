#ifndef OS_GFX_LINUX_WAYLAND_H
#define OS_GFX_LINUX_WAYLAND_H

#undef global
#include <wayland-client.h>
#define global static

// wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-client-protocol.h
#include "xdg-shell-client-protocol.h"

typedef struct Wayland_Window {
  struct wl_surface *surface;
  struct wl_buffer *buffer;

  struct xdg_surface *xdg_surface;
  struct xdg_surface_listener xdg_surface_listener;
  struct xdg_toplevel *xdg_toplevel;
  struct xdg_toplevel_listener xdg_toplevel_listener;

  SharedMem shm;
  struct Wayland_Window *next;
  struct Wayland_Window *prev;
} Wayland_Window;

typedef struct {
  Arena *arena;

  Wayland_Window *freelist_window;
  Wayland_Window *first_window;
  Wayland_Window *last_window;

  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_registry_listener reglistener;
  struct wl_compositor *compositor;
  struct wl_shm *shm;

  struct xdg_wm_base *xdg_base;
  struct xdg_wm_base_listener xdg_base_listener;
} Wayland_State;

fn void way_registry_handle_global(void *data, struct wl_registry *registry,
                                   u32 name, const char *interface, u32 version);
fn void way_registry_handle_global_remove(void *data, struct wl_registry *registry, u32 name);

fn void way_xdg_base_ping(void *data, struct xdg_wm_base *xdg_base, u32 serial);
fn void way_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, u32 serial);
fn void way_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                   i32 width, i32 height, struct wl_array *states);
fn void way_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel);

#endif
