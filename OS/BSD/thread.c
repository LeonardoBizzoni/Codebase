fn Thread os_thdSpawn(thd_fn thread_main, void *args) {
  Assert(thread_main);

  Thread thread;
  i32 res = pthread_create(&thread, 0, thread_main, args);
  if (!thread || res != 0) {
    perror("`Base::OS::spawn_thread`");
  }
  
  return thread;
}

inline fn void os_thdJoin(Thread tcb, void **return_buff) {
  (void)pthread_join(tcb, return_buff);
}
