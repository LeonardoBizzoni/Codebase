#ifndef OS_GFX_H
#define OS_GFX_H

typedef u32 OS_EventType;
enum {
  OS_EventType_None,
  OS_EventType_Kill,
  OS_EventType_Expose,
  OS_EventType_KeyDown,
  OS_EventType_KeyUp,
};

typedef struct {
  OS_EventType type;

  union {
    struct {
      i32 width;
      i32 height;
    } expose;
    struct {
      u32 keycode;  // keyboard layout dependent
      u32 scancode; // keyboard layout independent
    } key;
  };
} OS_Event;

fn OS_Handle os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height);
fn void os_window_show(OS_Handle handle);
fn void os_window_hide(OS_Handle handle);
fn void os_window_close(OS_Handle handle);

fn void os_window_swapBuffers(OS_Handle handle);

fn OS_Event os_window_get_event(OS_Handle handle);
fn OS_Event os_window_wait_event(OS_Handle handle);

fn String8 os_keyname_from_event(Arena *arena, OS_Event event);

#if USING_OPENGL
fn void opengl_init(OS_Handle handle);
fn void opengl_deinit(OS_Handle handle);

fn void opengl_make_current(OS_Handle handle);
#endif

#endif
