#ifndef OS_GFX_H
#define OS_GFX_H

fn OS_Handle os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height);
fn void os_window_show(OS_Handle handle);
fn void os_window_hide(OS_Handle handle);
fn void os_window_close(OS_Handle handle);

#endif
