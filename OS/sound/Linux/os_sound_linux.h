#ifndef OS_SOUND_LINUX_H
#define OS_SOUND_LINUX_H

#include <pulse/pulseaudio.h>

typedef struct {
  pa_threaded_mainloop *m;
  pa_mainloop_api *m_api;
  pa_context *ctx;
} LNX_SND_State;

fn void lnx_snd_init(void);
fn void lnx_snd_deinit(void);

fn void _lnx_pulse_ctx_statechange(pa_context *ctx, void *userdata);
fn void _lnx_pulse_stream_statechange(pa_stream *stream, void *userdata);
fn void _lnx_pulse_stream_drain(pa_stream *stream, i32 success, void *userdata);
fn void _lnx_pulse_stream_write(pa_stream *stream, usize success, void *userdata);
fn void _lnx_snd_player(void *args);

#endif
