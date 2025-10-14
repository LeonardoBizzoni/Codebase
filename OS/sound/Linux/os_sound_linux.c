global LNX_SND_State lnx_snd_state = {0};

fn void _lnx_pulse_ctx_statechange(pa_context *ctx, void *userdata) {
  Unused(ctx);
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}
fn void _lnx_pulse_stream_statechange(pa_stream *stream, void *userdata) {
  Unused(stream);
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}
fn void _lnx_pulse_stream_drain(pa_stream *stream, i32 success, void *userdata) {
  Unused(stream);
  Unused(success);
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}
fn void _lnx_pulse_stream_write(pa_stream *stream, usize success, void *userdata) {
  Unused(stream);
  Unused(success);
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}

fn void _lnx_snd_player(void *args) {
  UNX_Primitive *prim = (UNX_Primitive *)args;
  Assert(prim->sound.file.prop.size > 0);
  for (usize offset = 0;;) {
    os_mutex_scope(prim->sound.pausexit_mutex) {
      if (prim->sound.should_exit) { return; }
      while (prim->sound.paused) {
        os_cond_wait(prim->sound.pausexit_condvar,
                     prim->sound.pausexit_mutex, 0);
      }
    }

    os_mutex_scope(prim->sound.player_mutex) {
      offset = prim->sound.player_offset;
    }
    if (offset >= (usize)prim->sound.file.prop.size) {return;}

    DeferLoop(pa_threaded_mainloop_lock(lnx_snd_state.m),
              pa_threaded_mainloop_unlock(lnx_snd_state.m)) {
      usize writable = pa_stream_writable_size(prim->sound.stream);
      if (writable > 0) {
        usize to_write = ((usize)prim->sound.file.prop.size - offset) < writable
                         ? ((usize)prim->sound.file.prop.size - offset)
                         : writable;
        pa_stream_write(prim->sound.stream, &prim->sound.file.content[offset],
                        to_write, 0, 0, PA_SEEK_RELATIVE);
        os_mutex_scope(prim->sound.player_mutex) {
          prim->sound.player_offset += to_write;
        }
      }
      pa_threaded_mainloop_wait(lnx_snd_state.m);
    }
  }
}

fn void lnx_snd_init(void) {
  lnx_snd_state.m = pa_threaded_mainloop_new();
  lnx_snd_state.m_api = pa_threaded_mainloop_get_api(lnx_snd_state.m);
  pa_threaded_mainloop_start(lnx_snd_state.m);

  DeferLoop(pa_threaded_mainloop_lock(lnx_snd_state.m), pa_threaded_mainloop_unlock(lnx_snd_state.m))
  {
    lnx_snd_state.ctx = pa_context_new(lnx_snd_state.m_api, APPLICATION_NAME);
    pa_context_set_state_callback(lnx_snd_state.ctx, _lnx_pulse_ctx_statechange, lnx_snd_state.m);
    pa_context_connect(lnx_snd_state.ctx, 0, PA_CONTEXT_NOFLAGS, 0);
    while (pa_context_get_state(lnx_snd_state.ctx) != PA_CONTEXT_READY) {
      pa_threaded_mainloop_wait(lnx_snd_state.m);
    }
  }
}

fn OS_Handle os_snd_load(String8 path, char *name) {
  UNX_Primitive *prim = unx_primitive_alloc(UNX_Primitive_Sound);
  prim->sound.player_mutex = os_mutex_alloc();
  prim->sound.pausexit_mutex = os_mutex_alloc();
  prim->sound.pausexit_condvar = os_cond_alloc();

  if (str8_ends_with_str8(path, Strlit(".raw"))) {
    prim->sound.sample_info.format = PA_SAMPLE_S16NE;
    prim->sound.sample_info.channels = 2;
    prim->sound.sample_info.rate = 44100;
  } else if (str8_ends_with_str8(path, Strlit(".wav"))) {
    // TODO(lb): parse wav file header
  } else if (str8_ends_with_str8(path, Strlit(".mp3"))) {
    // TODO(lb): parse mp3 file header
  } else if (str8_ends_with_str8(path, Strlit(".flac"))) {
    // TODO(lb): parse flac file header
  } else {
    Err("Unsupported audio file format.");
    OS_Handle res = {0};
    return res;
  }

  Scratch scratch = ScratchBegin(0, 0);
  prim->sound.file = os_fs_fopen(scratch.arena, os_fs_open(path, OS_acfRead));
  if (!name) {
    name = cstr_from_str8(scratch.arena,
                          os_fs_filename_from_path(scratch.arena, path));
  }

  DeferLoop(pa_threaded_mainloop_lock(lnx_snd_state.m), pa_threaded_mainloop_unlock(lnx_snd_state.m))
  {
    prim->sound.stream = pa_stream_new(lnx_snd_state.ctx, name,
                                       &prim->sound.sample_info, 0);
    ScratchEnd(scratch);
    pa_stream_set_state_callback(prim->sound.stream, _lnx_pulse_stream_statechange,
                                 lnx_snd_state.m);
    pa_stream_connect_playback(prim->sound.stream, 0, 0, PA_STREAM_NOFLAGS, 0, 0);
    while (pa_stream_get_state(prim->sound.stream) != PA_STREAM_READY) {
      pa_threaded_mainloop_wait(lnx_snd_state.m);
    }
    pa_stream_set_write_callback(prim->sound.stream, _lnx_pulse_stream_write,
                                 lnx_snd_state.m);
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_snd_start(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  Assert(prim->type == UNX_Primitive_Sound);
  prim->sound.player = os_thread_start(_lnx_snd_player, prim);
}

fn void os_snd_stop(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  Assert(prim->type == UNX_Primitive_Sound);
  os_mutex_scope(prim->sound.pausexit_mutex) {
    prim->sound.should_exit = true;
  }

  os_snd_until_end(handle);
}

fn void os_snd_skip(OS_Handle handle, i64 ms) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  Assert(prim->type == UNX_Primitive_Sound);
  u64 byte_inc = 0;
  TrackByteOffset_from_ms(prim->sound, (f64)ms, byte_inc);

  os_mutex_scope(prim->sound.player_mutex) {
    prim->sound.player_offset += byte_inc;
  }
}

fn void os_snd_goto(OS_Handle handle, u64 ms) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  Assert(prim->type == UNX_Primitive_Sound);

  os_mutex_scope(prim->sound.player_mutex) {
    TrackByteOffset_from_ms(prim->sound, (f64)ms, prim->sound.player_offset);
  }
}

fn void os_snd_until_end(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  Assert(prim->type == UNX_Primitive_Sound);

  os_thread_join(prim->sound.player);
  pa_stream_disconnect(prim->sound.stream);
  pa_stream_unref(prim->sound.stream);

  os_fs_fclose(&prim->sound.file);
  os_mutex_free(prim->sound.pausexit_mutex);
  os_cond_free(prim->sound.pausexit_condvar);
  unx_primitive_free(prim);
}

fn void os_snd_pause(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  Assert(prim->type == UNX_Primitive_Sound);
  os_mutex_scope(prim->sound.pausexit_mutex) {
    prim->sound.paused = true;
  }
}

fn void os_snd_resume(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  Assert(prim->type == UNX_Primitive_Sound);
  os_mutex_scope(prim->sound.pausexit_mutex) {
    prim->sound.paused = false;
    os_cond_signal(prim->sound.pausexit_condvar);
  }
}
