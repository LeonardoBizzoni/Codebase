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

typedef u32 OS_Key;
enum {
  OS_Key_None,
  OS_Key_Esc,
  OS_Key_F1,
  OS_Key_F2,
  OS_Key_F3,
  OS_Key_F4,
  OS_Key_F5,
  OS_Key_F6,
  OS_Key_F7,
  OS_Key_F8,
  OS_Key_F9,
  OS_Key_F10,
  OS_Key_F11,
  OS_Key_F12,
  OS_Key_Tick,
  OS_Key_1,
  OS_Key_2,
  OS_Key_3,
  OS_Key_4,
  OS_Key_5,
  OS_Key_6,
  OS_Key_7,
  OS_Key_8,
  OS_Key_9,
  OS_Key_0,
  OS_Key_Minus,
  OS_Key_Equal,
  OS_Key_Backspace,
  OS_Key_Tab,
  OS_Key_Q,
  OS_Key_W,
  OS_Key_E,
  OS_Key_R,
  OS_Key_T,
  OS_Key_Y,
  OS_Key_U,
  OS_Key_I,
  OS_Key_O,
  OS_Key_P,
  OS_Key_LeftBracket,
  OS_Key_RightBracket,
  OS_Key_BackSlash,
  OS_Key_CapsLock,
  OS_Key_A,
  OS_Key_S,
  OS_Key_D,
  OS_Key_F,
  OS_Key_G,
  OS_Key_H,
  OS_Key_J,
  OS_Key_K,
  OS_Key_L,
  OS_Key_Semicolon,
  OS_Key_Quote,
  OS_Key_Return,
  OS_Key_Shift,
  OS_Key_Z,
  OS_Key_X,
  OS_Key_C,
  OS_Key_V,
  OS_Key_B,
  OS_Key_N,
  OS_Key_M,
  OS_Key_Comma,
  OS_Key_Period,
  OS_Key_Slash,
  OS_Key_Ctrl,
  OS_Key_Alt,
  OS_Key_Space,
  OS_Key_ScrollLock,
  OS_Key_Pause,
  OS_Key_Insert,
  OS_Key_Home,
  OS_Key_Delete,
  OS_Key_End,
  OS_Key_PageUp,
  OS_Key_PageDown,
  OS_Key_Up,
  OS_Key_Left,
  OS_Key_Down,
  OS_Key_Right,
  OS_Key_COUNT,
};

typedef struct OS_Event {
  struct OS_Event *next;
  struct OS_Event *last;

  OS_EventType type;
  OS_Handle window;

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

typedef struct OS_EventList {
  struct OS_Event *first;
  struct OS_Event *last;

  OS_Event event;
  isize count;
} OS_EventList;

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
fn void os_window_get_size(OS_Handle window, i32 *width, i32 *height);

fn OS_EventList os_get_events(Arena *arena, bool wait);
fn bool os_key_is_down(OS_Key key);

fn String8 os_keyname_from_event(Arena *arena, OS_Event event);

#endif
