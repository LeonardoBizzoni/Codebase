global LNX_SND_State lnx_snd_state = {0};

fn void _lnx_pulse_ctx_statechange(pa_context *ctx, void *userdata) {
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}
fn void _lnx_pulse_stream_statechange(pa_stream *stream, void *userdata) {
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}
fn void _lnx_pulse_stream_drain(pa_stream *stream, i32 success, void *userdata) {
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}
fn void _lnx_pulse_stream_write(pa_stream *stream, usize success, void *userdata) {
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}

fn void _lnx_snd_player(void *args) {
  LNX_Primitive *prim = (LNX_Primitive *)args;
  Assert(prim->sound.file.prop.size > 0);
  for (usize offset = 0; offset < prim->sound.file.prop.size;) {
    os_mutex_lock(prim->sound.pause_mutex);
    DeferLoop(os_mutex_unlock(prim->sound.pause_mutex)) {
      if (prim->sound.should_exit) { return; }
      while (prim->sound.paused) {
        os_cond_wait(prim->sound.pause_condvar, prim->sound.pause_mutex, 0);
      }
    }

    pa_threaded_mainloop_lock(lnx_snd_state.m);
    DeferLoop(pa_threaded_mainloop_unlock(lnx_snd_state.m)) {
      usize writable = pa_stream_writable_size(prim->sound.stream);
      if (writable > 0) {
        usize to_write = (prim->sound.file.prop.size - offset) < writable
                         ? (prim->sound.file.prop.size - offset)
                         : writable;
        pa_stream_write(prim->sound.stream, &prim->sound.file.content[offset],
                        to_write, 0, 0, PA_SEEK_RELATIVE);
        offset += to_write;
      }
      pa_threaded_mainloop_wait(lnx_snd_state.m);
    }
  }
}

fn void lnx_snd_init(void) {
  lnx_snd_state.m = pa_threaded_mainloop_new();
  lnx_snd_state.m_api = pa_threaded_mainloop_get_api(lnx_snd_state.m);
  pa_threaded_mainloop_start(lnx_snd_state.m);

  pa_threaded_mainloop_lock(lnx_snd_state.m);
  DeferLoop(pa_threaded_mainloop_unlock(lnx_snd_state.m)) {
    lnx_snd_state.ctx = pa_context_new(lnx_snd_state.m_api, APPLICATION_NAME);
    pa_context_set_state_callback(lnx_snd_state.ctx, _lnx_pulse_ctx_statechange, lnx_snd_state.m);
    pa_context_connect(lnx_snd_state.ctx, 0, PA_CONTEXT_NOFLAGS, 0);
    while (pa_context_get_state(lnx_snd_state.ctx) != PA_CONTEXT_READY) {
      pa_threaded_mainloop_wait(lnx_snd_state.m);
    }
  }
}

fn void lnx_snd_deinit(void) {
  pa_context_disconnect(lnx_snd_state.ctx);
  pa_context_unref(lnx_snd_state.ctx);
  pa_threaded_mainloop_stop(lnx_snd_state.m);
  pa_threaded_mainloop_free(lnx_snd_state.m);
}

fn OS_Handle os_snd_load(String8 path, char *name) {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Sound);
  prim->sound.pause_mutex = os_mutex_alloc();
  prim->sound.pause_condvar = os_cond_alloc();

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
  prim->sound.file = fs_fopen(scratch.arena, fs_open(path, OS_acfRead));
  if (!name) {
    name = cstr_from_str8(scratch.arena,
                          fs_filename_from_path(scratch.arena, path));
  }

  pa_threaded_mainloop_lock(lnx_snd_state.m);
  DeferLoop(pa_threaded_mainloop_unlock(lnx_snd_state.m)) {
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
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  Assert(prim->type == LNX_Primitive_Sound);
  prim->sound.player = os_thread_start(_lnx_snd_player, prim);
}

fn void os_snd_stop(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  Assert(prim->type == LNX_Primitive_Sound);
  os_mutex_lock(prim->sound.pause_mutex);
  DeferLoop(os_mutex_unlock(prim->sound.pause_mutex)) {
    prim->sound.should_exit = true;
  }

  os_snd_until_end(handle);
}

fn void os_snd_until_end(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  Assert(prim->type == LNX_Primitive_Sound);

  os_thread_join(prim->sound.player);
  pa_stream_disconnect(prim->sound.stream);
  pa_stream_unref(prim->sound.stream);

  fs_fclose(&prim->sound.file);
  os_mutex_free(prim->sound.pause_mutex);
  os_cond_free(prim->sound.pause_condvar);
  lnx_primitiveFree(prim);
}

fn void os_snd_pause(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  Assert(prim->type == LNX_Primitive_Sound);
  os_mutex_lock(prim->sound.pause_mutex);
  DeferLoop(os_mutex_unlock(prim->sound.pause_mutex)) {
    prim->sound.paused = true;
  }
}

fn void os_snd_resume(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  Assert(prim->type == LNX_Primitive_Sound);
  os_mutex_lock(prim->sound.pause_mutex);
  DeferLoop(os_mutex_unlock(prim->sound.pause_mutex)) {
    prim->sound.paused = false;
    os_cond_signal(prim->sound.pause_condvar);
  }
}
