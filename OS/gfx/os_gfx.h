#ifndef OS_GFX_H
#define OS_GFX_H

#define MAX_SUPPORTED_GAMEPAD 4

typedef OS_Handle GFX_Handle;

typedef u32 OS_EventType;
enum {
  OS_EventType_None,
  OS_EventType_Kill,
  OS_EventType_Expose,

  OS_EventType_KeyDown,
  OS_EventType_KeyUp,

  OS_EventType_BtnDrag,
  OS_EventType_BtnDown,
  OS_EventType_BtnUp,

  OS_EventType_PointerMotion,

  OS_EventType_GamepadConnected,
  OS_EventType_GamepadDisconnected,
};

typedef u16 OS_PtrButton;
enum {
  OS_PtrButton_None,
  OS_PtrButton_Left,
  OS_PtrButton_Right,
  OS_PtrButton_Middle,
  OS_PtrButton_Side,
  OS_PtrButton_Extra,
  OS_PtrButton_Forward,
  OS_PtrButton_Back,
  OS_PtrButton_Task,
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
    struct {
      OS_PtrButton id;
      Vec2f32 position;
    } btn;
    struct {
      OS_PtrButton id;
      Vec2f32 end;
      Vec2f32 start;
    } drag;
    Vec2f32 pointer;
  };
} OS_Event;

typedef struct {
  i32 half_transitions; // from pressed to release or viceversa discarding the transition back
  bool ended_down;
} OS_BtnState;

typedef struct {
  bool active;

  Vec2f32 stick_left;  // negative X values left, positive right
  Vec2f32 stick_right; // negative Y values down, positivie up

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
  Vec2f32 pointer;
  OS_Gamepad gamepad[MAX_SUPPORTED_GAMEPAD];
} OS_InputDeviceState;

fn OS_Handle os_window_open(String8 name, i32 width, i32 height);
fn void os_window_show(OS_Handle window);
fn void os_window_hide(OS_Handle window);
fn void os_window_close(OS_Handle window);

fn OS_Event os_window_get_event(OS_Handle window);
fn OS_Event os_window_wait_event(OS_Handle window);

fn String8 os_keyname_from_event(Arena *arena, OS_Event event);

#endif
