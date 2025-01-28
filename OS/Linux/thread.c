fn void* os_thdEntry(void *args) {
  lnx_thdData *wrap_args = (lnx_thdData *)args;
  ThreadFunc *func = (ThreadFunc *)wrap_args->func;
  func(wrap_args->args);
  return 0;
}

fn OS_Handle os_thdSpawn(ThreadFunc *thread_main, void *args) {
  Assert(thread_main);

  /* NOTE(lb): i'm pretty sure this isn't the right approach but
     i don't want this function to receive an arena and i don't have a way
     to ask the os to allocate memory that isn't inside an arena right now.
     Another idea would be to have a global arena that is used only for these
     OS functions idk. */
  Scratch scr = ScratchBegin(0, 0);
  lnx_thdData *wrap_args = New(scr.arena, lnx_thdData);
  wrap_args->func = thread_main;
  wrap_args->args = args;

  OS_Handle res = {0};
  pthread_t *thread_id = (pthread_t *)&res.h;
  i32 maybeErr = pthread_create(thread_id, 0, os_thdEntry, wrap_args);
  ScratchEnd(scr);

  if (maybeErr == 0) {
    return res;
  } else {
    res.h[0] = 0;
    return res;
  }
}

fn void os_thdJoin(OS_Handle thd_handle, void **return_buff) {
  (void)pthread_join((pthread_t)thd_handle.h[0], return_buff);
}
