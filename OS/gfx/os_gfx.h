#ifndef OS_GFX_H
#define OS_GFX_H

#define MAX_SUPPORTED_GAMEPAD 4

typedef u32 OS_EventType;
enum {
  OS_EventType_None,
  OS_EventType_Kill,
  OS_EventType_Expose,

  OS_EventType_KeyDown,
  OS_EventType_KeyUp,

  OS_EventType_GamepadConnected,
  OS_EventType_GamepadDisconnected,
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

typedef struct {
  i32 half_transitions; // from pressed to release or viceversa discarding the transition back
  bool ended_down;
} OS_BtnState;

typedef struct {
  f32 x; // negative values left, positive right
  f32 y; // negative values down, positivie up
} OS_GamepadStick;

typedef struct {
  bool active;

  OS_GamepadStick stick_left;
  OS_GamepadStick stick_right;

  f32 r2;
  f32 l2;

  union {
    OS_BtnState buttons[14];
    struct {
      OS_BtnState dpad_up;
      OS_BtnState dpad_down;
      OS_BtnState dpad_left;
      OS_BtnState dpad_right;

      OS_BtnState start;
      OS_BtnState select;

      OS_BtnState l3;
      OS_BtnState r3;
      OS_BtnState l1;
      OS_BtnState r1;

      OS_BtnState a;
      OS_BtnState b;
      OS_BtnState x;
      OS_BtnState y;
    };
  };
} OS_Gamepad;

typedef struct {
  u32 width, height;
  OS_Handle handle;
} OS_Window;

fn OS_Window os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height);
fn void os_window_show(OS_Window handle);
fn void os_window_hide(OS_Window handle);
fn void os_window_close(OS_Window handle);

fn void os_window_swapBuffers(OS_Window handle);
fn void os_window_render(OS_Window window_, void *mem);

fn OS_Event os_window_get_event(OS_Window handle);
fn OS_Event os_window_wait_event(OS_Window handle);

fn String8 os_keyname_from_event(Arena *arena, OS_Event event);

#if USING_OPENGL
fn void opengl_init(OS_Window handle);
fn void opengl_deinit(OS_Window handle);

fn void opengl_make_current(OS_Window handle);
#endif

#endif
