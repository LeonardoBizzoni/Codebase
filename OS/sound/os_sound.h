#ifndef OS_SOUND_H
#define OS_SOUND_H

fn OS_Handle os_snd_load(String8 path, char *name);

fn void os_snd_start(OS_Handle handle);
fn void os_snd_stop(OS_Handle handle);
fn void os_snd_until_end(OS_Handle handle);

fn void os_snd_pause(OS_Handle handle);
fn void os_snd_resume(OS_Handle handle);

#endif
