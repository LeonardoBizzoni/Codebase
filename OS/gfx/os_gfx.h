#ifndef OS_GFX_H
#define OS_GFX_H

typedef u32 GFX_Api;
enum {
  GFX_Api_Opengl = 1 << 0,
  /* More? */
};

typedef u32 OS_EventType;
enum {
  OS_EventType_None,
  OS_EventType_Kill,
  OS_EventType_Expose,
};

typedef struct {
  OS_EventType type;

  union {
    struct {
      i32 width;
      i32 height;
    } expose;
  };
} OS_Event;

fn OS_Handle os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height,
                            GFX_Api api);
fn void os_window_show(OS_Handle handle);
fn void os_window_hide(OS_Handle handle);
fn void os_window_close(OS_Handle handle);

fn void os_window_swapBuffers(OS_Handle handle);
fn OS_Event os_window_get_event(OS_Handle handle);

#endif
