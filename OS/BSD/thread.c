inline fn Thread *threadSpawn(OS_ThreadFunction *func) {
  return threadSpawnArgs(func, 0);
}

fn Thread *threadSpawnArgs(OS_ThreadFunction *func, void *arg_data) {
  Assert(func);
  
  Thread *thread;
  i32 res = pthread_create(&thread, 0, func, arg_data);
  
  if (!thread || res != 0) {
    perror("`Base::OS::spawn_thread`");
  }
  
  return thread;
}

inline fn void threadJoin(Thread *tcb) {
  threadJoinReturn(tcb, 0);
}

fn void threadJoinReturn(Thread *tcb, void **save_return_value_in) {
  (void)pthread_join(tcb, save_return_value_in);
}
