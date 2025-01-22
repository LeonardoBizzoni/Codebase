inline fn Thread os_thdSpawn(OS_ThreadFunction *func) {
  return os_thdSpawnArgs(func, 0);
}

fn Thread os_thdSpawnArgs(OS_ThreadFunction *func, void *arg_data) {
  Assert(func);
  
  Thread thread_id;
  pthread_create(&thread_id, 0, func, arg_data);
  
  return thread_id;
}

inline fn void os_thdJoin(Thread id) {
  os_thdJoinReturn(id, 0);
}

fn void os_thdJoinReturn(Thread id, void **save_return_value_in) {
  (void)pthread_join(id, save_return_value_in);
}
