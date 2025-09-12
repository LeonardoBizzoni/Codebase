#ifndef OS_SOUND_LINUX_H
#define OS_SOUND_LINUX_H

#include <pulse/pulseaudio.h>

#define TrackByteOffset_from_ms(SND_PRIM, MS, RES)                 \
_stmt(                                                             \
  u8 _bytesXsample = 0;                                            \
  switch((SND_PRIM).sample_info.format) {                          \
  case PA_SAMPLE_U8:                                               \
  case PA_SAMPLE_ALAW:                                             \
  case PA_SAMPLE_ULAW: {                                           \
    _bytesXsample = 1;                                             \
  } break;                                                         \
  case PA_SAMPLE_S16LE:                                            \
  case PA_SAMPLE_S16BE: {                                          \
    _bytesXsample = 2;                                             \
  } break;                                                         \
  default: {                                                       \
    _bytesXsample = 3;                                             \
  } break;                                                         \
  }                                                                \
  (RES) = (usize)((MS) / 1e3 * _bytesXsample *                     \
                  (SND_PRIM).sample_info.channels *                \
                  (SND_PRIM).sample_info.rate);                    \
)

typedef struct {
  pa_threaded_mainloop *m;
  pa_mainloop_api *m_api;
  pa_context *ctx;
} LNX_SND_State;

fn void lnx_snd_init(void);

fn void _lnx_pulse_ctx_statechange(pa_context *ctx, void *userdata);
fn void _lnx_pulse_stream_statechange(pa_stream *stream, void *userdata);
fn void _lnx_pulse_stream_drain(pa_stream *stream, i32 success, void *userdata);
fn void _lnx_pulse_stream_write(pa_stream *stream, usize success, void *userdata);
fn void _lnx_snd_player(void *args);

#endif
