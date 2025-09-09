#ifndef OS_GFX_UNIX_WL_H
#define OS_GFX_UNIX_WL_H

#undef global
#include <stdatomic.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "xdg-shell-client-protocol.h"
#define global static

#define WL_F32_FROM_FIXED(VALUE) ((f32)VALUE / 256.f)
#define WL_EVDEV_SCANCODE_TO_XKB(EVDEV_SCANCODE) (EVDEV_SCANCODE + 8)

typedef u32 wl_keyboard_format;
enum {
  Wl_Keyboard_Format_None = 0,
  Wl_Keyboard_Format_Xkbv1 = 1,
};

typedef u32 wl_keyboard_key_state;
enum {
  Wl_Keyboard_Key_State_Released,
  Wl_Keyboard_Key_State_Pressed,
  Wl_Keyboard_Key_State_Repeated,
};

typedef u32 wl_pointer_button_state;
enum {
  Wl_Pointer_Button_State_Released,
  Wl_Pointer_Button_State_Pressed,
};

typedef u32 wl_pointer_button;
enum {
  Wl_Pointer_Btn_Left    = 0x110,
  Wl_Pointer_Btn_Right   = 0x111,
  Wl_Pointer_Btn_Middle  = 0x112,
  Wl_Pointer_Btn_Side    = 0x113,
  Wl_Pointer_Btn_Extra   = 0x114,
  Wl_Pointer_Btn_Forward = 0x115,
  Wl_Pointer_Btn_Back    = 0x116,
  Wl_Pointer_Btn_Task    = 0x117,
};

typedef u32 wl_capabilities;
enum {
  Wl_Capabilities_Pointer  = 1 << 0,
  Wl_Capabilities_Keyboard = 1 << 1,
  Wl_Capabilities_Touch    = 1 << 2,
};

typedef struct Wl_WindowEvent {
  OS_Event value;
  struct Wl_WindowEvent *next;
} Wl_WindowEvent;

typedef struct Wl_Window {
  struct wl_shm_pool *wl_shm_pool;
  struct wl_buffer *wl_buffer;
  struct wl_surface *wl_surface;
  struct wl_callback *wl_callback;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;

  atomic_bool xdg_surface_acked;
  SharedMem shm;

  u32 width, height;
  GFX_Handle gfx_context;

  struct {
    OS_Handle mutex;
    OS_Handle condvar;
    Wl_WindowEvent *first;
    Wl_WindowEvent *last;
  } events;

  struct Wl_Window *next;
  struct Wl_Window *prev;
} Wl_Window;

typedef struct {
  Arena *arena;
  OS_Handle event_dispatcher;

  struct wl_display *wl_display;
  struct wl_registry *wl_registry;
  struct wl_shm *wl_shm;
  struct wl_compositor *wl_compositor;
  struct wl_seat *wl_seat;
  struct wl_pointer *wl_pointer;
  struct wl_keyboard *wl_keyboard;
  struct wl_touch *wl_touch;
  struct xdg_wm_base *xdg_wm_base;

  struct xkb_context *xkb_context;
  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;

  struct {
    OS_Handle mutex;
    Wl_WindowEvent *first;
    Wl_WindowEvent *last;
  } events;

  Wl_Window *kbd_window_focused;
  Wl_Window *ptr_window_focused;

  Wl_Window *window_freelist;
  Wl_Window *window_first;
  Wl_Window *window_last;
} Wl_State;

fn Wl_Window* wl_window_from_surface(struct wl_surface *surface);
fn Wl_WindowEvent* wl_alloc_windowevent(void);
fn void wl_event_dispatcher(void *_);

fn void wl_registry_event_global(void *data, struct wl_registry *registry,
                                 u32 name, const char *interface, u32 version);
fn void wl_seat_event_capabilities(void *data, struct wl_seat *wl_seat, u32 capabilities);
fn void wl_seat_event_name(void *data, struct wl_seat *wl_seat, const char *name);
fn void wl_callback_event_done(void *data, struct wl_callback *wl_callback,
                               u32 callback_data);
fn void wl_keyboard_event_keymap(void *data, struct wl_keyboard *wl_keyboard,
                                 u32 format, i32 fd, u32 size);
fn void wl_keyboard_event_enter(void *data, struct wl_keyboard *wl_keyboard,
                                u32 serial, struct wl_surface *surface,
                                struct wl_array *keys);
fn void wl_keyboard_event_leave(void *data, struct wl_keyboard *wl_keyboard,
                                u32 serial, struct wl_surface *surface);
fn void wl_keyboard_event_key(void *data, struct wl_keyboard *wl_keyboard,
                              u32 serial, u32 time, u32 key, u32 state);
fn void wl_keyboard_event_modifiers(void *data,
                                    struct wl_keyboard *wl_keyboard,
                                    u32 serial, u32 mods_depressed,
                                    u32 mods_latched, u32 mods_locked, u32 group);
fn void wl_keyboard_event_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
                                      int32_t rate, int32_t delay);
fn void wl_pointer_event_enter(void *data, struct wl_pointer *wl_pointer,
                               u32 serial, struct wl_surface *surface,
                               wl_fixed_t surface_x, wl_fixed_t surface_y);
fn void wl_pointer_event_leave(void *data, struct wl_pointer *wl_pointer,
                               u32 serial, struct wl_surface *surface);
fn void wl_pointer_event_motion(void *data, struct wl_pointer *wl_pointer,
                                u32 time, wl_fixed_t surface_x, wl_fixed_t surface_y);
fn void wl_pointer_event_button(void *data, struct wl_pointer *wl_pointer,
                                u32 serial, u32 time, u32 button, u32 state);
fn void wl_pointer_event_axis(void *data, struct wl_pointer *wl_pointer,
                              u32 time, u32 axis, wl_fixed_t value);
fn void wl_pointer_event_frame(void *data, struct wl_pointer *wl_pointer);
fn void wl_pointer_event_axis_source(void *data, struct wl_pointer *wl_pointer,
                                     u32 axis_source);
fn void wl_pointer_event_axis_stop(void *data, struct wl_pointer *wl_pointer,
                                   u32 time, u32 axis);
fn void wl_pointer_event_axis_value120(void *data, struct wl_pointer *wl_pointer,
                                       u32 axis, i32 value120);
fn void wl_pointer_event_axis_relative_direction(void *data,
                                                 struct wl_pointer *wl_pointer,
                                                 u32 axis, u32 direction);

fn void xdg_wm_base_even_ping(void *data, struct xdg_wm_base *xdg_base, u32 serial);
fn void xdg_surface_event_configure(void *data, struct xdg_surface *xdg_surface, u32 serial);
fn void xdg_toplevel_event_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                     i32 width, i32 height, struct wl_array *states);
fn void xdg_toplevel_event_close(void *data, struct xdg_toplevel *xdg_toplevel);
fn void xdg_toplevel_event_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel,
                                            int32_t width, int32_t height);
fn void xdg_toplevel_event_wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel,
                                           struct wl_array *capabilities);

#endif
