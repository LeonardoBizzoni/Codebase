fn Thread os_thdSpawn(thd_fn thread_main, void *arg_data) {
  Assert(thread_main);
  Thread thread_id;
  pthread_create(&thread_id, 0, func, arg_data);
  
  return thread_id;
}

fn void os_thdJoin(Thread id, void **return_buff) {
  (void)pthread_join(id, return_buff);
}
