global OS_Posix_State os_posix_state = {0};

// =============================================================================
// System information retrieval
fn OS_SystemInfo *os_sysinfo(void) {
  return &os_posix_state.info;
}

// =============================================================================
// DateTime
fn time64 os_local_now(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64_from_unix((u64)tms.tv_sec + os_posix_state.unix_utc_offset);
  res |= (u64)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_local_dateTimeNow(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = datetime_from_unix((u64)tms.tv_sec +
                                    os_posix_state.unix_utc_offset);
  res.ms = (u16)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn time64 os_local_fromUTCTime64(time64 t) {
  u64 utc_time = unix_from_time64(t);
  time64 res = time64_from_unix(utc_time + os_posix_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_local_fromUTCDateTime(DateTime *dt) {
  u64 utc_time = unix_from_datetime(dt);
  DateTime res = datetime_from_unix(utc_time + os_posix_state.unix_utc_offset);
  res.ms = dt->ms;
  return res;
}

fn time64 os_utc_now(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64_from_unix((u64)tms.tv_sec);
  res |= (u64)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_utc_dateTimeNow(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = datetime_from_unix((u64)tms.tv_sec);
  res.ms = (u16)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn time64 os_utc_localizedTime64(i8 utc_offset) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64_from_unix((u64)(tms.tv_sec + utc_offset * UNIX_HOUR));
  res |= (u64)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_utc_localizedDateTime(i8 utc_offset) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = datetime_from_unix((u64)(tms.tv_sec + utc_offset * UNIX_HOUR));
  res.ms = (u16)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn time64 os_utc_fromLocalTime64(time64 t) {
  u64 local_time = unix_from_time64(t);
  time64 res = time64_from_unix(local_time - os_posix_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_utc_fromLocalDateTime(DateTime *dt) {
  u64 local_time = unix_from_datetime(dt);
  DateTime res = datetime_from_unix(local_time - os_posix_state.unix_utc_offset);
  res.ms = dt->ms;
  return res;
}

fn void os_sleep_milliseconds(u32 ms) {
  usleep((useconds_t)(ms * 1e3));
}

fn OS_Handle os_timer_start(void) {
  OS_Posix_Primitive *prim = os_posix_primitive_alloc(OS_Posix_Primitive_Timer);
  if (clock_gettime(CLOCK_MONOTONIC, &prim->timer) != 0) {
    os_posix_primitive_release(prim);
    prim = 0;
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_timer_free(OS_Handle handle) {
  os_posix_primitive_release((OS_Posix_Primitive*)handle.h[0]);
}

fn u64 os_timer_elapsed_between(OS_Handle start, OS_Handle end, OS_TimerGranularity unit) {
  struct timespec tstart = ((OS_Posix_Primitive *)start.h[0])->timer;
  struct timespec tend = ((OS_Posix_Primitive *)end.h[0])->timer;
  u64 diff_nanos = (u64)(tend.tv_sec - tstart.tv_sec) * (u64)1e9 +
                   (u64)(tend.tv_nsec - tstart.tv_nsec);

  u64 res = 0;
  switch (unit) {
    case OS_TimerGranularity_min: {
      res = (u64)(((f64)diff_nanos / 1e9) / 60.0);
    } break;
    case OS_TimerGranularity_sec: {
      res = (u64)((f64)diff_nanos / 1e9);
    } break;
    case OS_TimerGranularity_ms: {
      res = (u64)((f64)diff_nanos / 1e6);
    } break;
    case OS_TimerGranularity_ns: {
      res = diff_nanos;
    } break;
  }
  return res;
}

fn f64 os_time_now(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (f64)((ts.tv_sec * OS_POSIX_TIME_FREQ + ts.tv_nsec) -
               os_posix_state.time_offset) /
         OS_POSIX_TIME_FREQ;
}

fn u64 os_time_update_frequency(void) {
  return OS_POSIX_TIME_FREQ;
}

// =============================================================================
// Memory allocation
fn void* os_reserve(i64 bytes) {
  Assert(bytes > 0);
  void *res = mmap(0, (u64)bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return (res != MAP_FAILED ? res : 0);
}

fn void os_release(void *base, i64 bytes) {
  Assert(bytes > 0);
  i32 res = munmap(base, (u64)bytes);
  Assert(res == 0);
}

fn void os_commit(void *base, i64 bytes) {
  Assert(bytes > 0);
  i32 res = mprotect(base, (u64)bytes, PROT_READ | PROT_WRITE);
  Assert(res == 0);
}

fn void os_decommit(void *base, i64 bytes) {
  Assert(bytes > 0);
  i32 res = mprotect(base, (u64)bytes, PROT_NONE);
  Assert(res == 0);
  res = madvise(base, (u64)bytes, MADV_DONTNEED);
  Assert(res == 0);
}

// =============================================================================
// Threads & Processes stuff
fn OS_Handle os_thread_start(Func_Thread *thread_main, void *args) {
  Assert(thread_main);

  OS_Posix_Primitive *prim = os_posix_primitive_alloc(OS_Posix_Primitive_Thread);
  prim->thread.func = thread_main;
  prim->thread.args = args;

  if (pthread_create(&prim->thread.handle, 0, os_posix_thread_entry, prim) != 0) {
    os_posix_primitive_release(prim);
    prim = 0;
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_thread_kill(OS_Handle thd_handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)thd_handle.h[0];
  if (!prim || prim->type != OS_Posix_Primitive_Thread) { return; }
  (void)pthread_kill(prim->thread.handle, 0);
  os_posix_primitive_release(prim);
}

fn void os_thread_cancel(OS_Handle thd_handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)thd_handle.h[0];
  if (!prim || prim->type != OS_Posix_Primitive_Thread) { return; }
  (void)pthread_cancel(prim->thread.handle);
  os_posix_primitive_release(prim);
}

fn bool os_thread_join(OS_Handle thd_handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)thd_handle.h[0];
  if (!prim || prim->type != OS_Posix_Primitive_Thread) { return false; }
  bool res = pthread_join(prim->thread.handle, 0) == 0;
  os_posix_primitive_release(prim);
  return res;
}

fn void os_thread_cancelpoint(void) {
  pthread_testcancel();
}

fn OS_Handle os_proc_spawn(String8 command) {
  pid_t pid = fork();
  Assert(pid >= 0);
  if (!pid) {
    Scratch scratch = ScratchBegin(0, 0);
    Assert(execl("/bin/sh", "sh", "-c",
                 cstr_from_str8(scratch.arena, command), NULL) != -1);
    Unreachable();
    ScratchEnd(scratch);
  }

  OS_Handle res = {{(u64)pid}};
  return res;
}

fn void os_proc_kill(OS_Handle handle) {
  (void)kill((pid_t)handle.h[0], SIGKILL);
}

fn i32 os_proc_wait(OS_Handle handle) {
  i32 child_res = 0;
  (void)waitpid((pid_t)handle.h[0], &child_res, 0);

  if (WIFEXITED(child_res)) {
    return WEXITSTATUS(child_res);
  } else if (WIFSTOPPED(child_res)) {
    return WSTOPSIG(child_res);
  } else if (WIFSIGNALED(child_res)) {
    return WTERMSIG(child_res);
  }
  return -1;
}

fn void os_exit(u8 status_code) {
  exit(status_code);
}

fn void os_atexit(Func_Void *callback) {
  atexit(callback);
}

fn OS_Handle os_mutex_alloc(void) {
  OS_Posix_Primitive *prim = os_posix_primitive_alloc(OS_Posix_Primitive_Mutex);
  pthread_mutexattr_t attr;
  if (pthread_mutexattr_init(&attr) != 0 ||
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
    os_posix_primitive_release(prim);
    prim = 0;
  } else {
    (void)pthread_mutex_init(&prim->mutex, &attr);
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_mutex_lock(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  (void)pthread_mutex_lock(&prim->mutex);
}

fn bool os_mutex_trylock(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  return pthread_mutex_trylock(&prim->mutex) == 0;
}

fn void os_mutex_unlock(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  (void)pthread_mutex_unlock(&prim->mutex);
}

fn void os_mutex_free(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  pthread_mutex_destroy(&prim->mutex);
  os_posix_primitive_release(prim);
}

fn OS_Handle os_rwlock_alloc(void) {
  OS_Posix_Primitive *prim = os_posix_primitive_alloc(OS_Posix_Primitive_Rwlock);
  pthread_rwlock_init(&prim->rwlock, 0);

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_rwlock_read_lock(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  (void)pthread_rwlock_rdlock(&prim->rwlock);
}

fn bool os_rwlock_read_trylock(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  return pthread_rwlock_tryrdlock(&prim->rwlock) == 0;
}

fn void os_rwlock_read_unlock(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  (void)pthread_rwlock_unlock(&prim->rwlock);
}

fn void os_rwlock_write_lock(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  (void)pthread_rwlock_wrlock(&prim->rwlock);
}

fn bool os_rwlock_write_trylock(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  return pthread_rwlock_trywrlock(&prim->rwlock) == 0;
}

fn void os_rwlock_write_unlock(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  (void)pthread_rwlock_unlock(&prim->rwlock);
}

fn void os_rwlock_free(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  pthread_rwlock_destroy(&prim->rwlock);
  os_posix_primitive_release(prim);
}

fn OS_Handle os_cond_alloc(void) {
  OS_Posix_Primitive *prim = os_posix_primitive_alloc(OS_Posix_Primitive_CondVar);
  if (pthread_cond_init(&prim->condvar.cond, 0)) {
    os_posix_primitive_release(prim);
    prim = 0;
  } else {
    (void)pthread_mutex_init(&prim->condvar.mutex, 0);
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_cond_signal(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  (void)pthread_cond_signal(&prim->condvar.cond);
}

fn void os_cond_broadcast(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  (void)pthread_cond_broadcast(&prim->condvar.cond);
}

fn bool os_cond_wait(OS_Handle cond_handle, OS_Handle mutex_handle,
                     u32 wait_at_most_microsec) {
  OS_Posix_Primitive *cond_prim = (OS_Posix_Primitive *)cond_handle.h[0];
  OS_Posix_Primitive *mutex_prim = (OS_Posix_Primitive *)mutex_handle.h[0];

  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += (i32)((f64)wait_at_most_microsec / 1e6);
    abstime.tv_nsec += (i32)((i32)1e3 * (wait_at_most_microsec % (u32)1e6));
    if (abstime.tv_nsec >= (i32)1e9) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= (i32)1e9;
    }

    return pthread_cond_timedwait(&cond_prim->condvar.cond, &mutex_prim->mutex, &abstime) == 0;
  } else {
    (void)pthread_cond_wait(&cond_prim->condvar.cond, &mutex_prim->mutex);
    return true;
  }
}

fn bool os_cond_waitrw_read(OS_Handle cond_handle, OS_Handle rwlock_handle,
                            u32 wait_at_most_microsec) {
  OS_Posix_Primitive *cond_prim = (OS_Posix_Primitive *)cond_handle.h[0];
  OS_Posix_Primitive *rwlock_prim = (OS_Posix_Primitive *)rwlock_handle.h[0];

  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += (i32)((f64)wait_at_most_microsec / 1e6);
    abstime.tv_nsec += (i32)1e3 * (i32)(wait_at_most_microsec % (u32)1e6);
    if (abstime.tv_nsec >= (i32)1e9) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= (i32)1e9;
    }

    pthread_mutex_lock(&cond_prim->condvar.mutex);
    if (pthread_cond_timedwait(&cond_prim->condvar.cond, &cond_prim->condvar.mutex,
                               &abstime) != 0) {
      (void)pthread_mutex_unlock(&cond_prim->condvar.mutex);
      return false;
    }
    (void)pthread_rwlock_rdlock(&rwlock_prim->rwlock);
  } else {
    pthread_mutex_lock(&cond_prim->condvar.mutex);
    (void)pthread_cond_wait(&cond_prim->condvar.cond, &cond_prim->condvar.mutex);
    (void)pthread_rwlock_rdlock(&rwlock_prim->rwlock);
  }

  (void)pthread_mutex_unlock(&cond_prim->condvar.mutex);
  return true;
}

fn bool os_cond_waitrw_write(OS_Handle cond_handle, OS_Handle rwlock_handle,
                             u32 wait_at_most_microsec) {
  OS_Posix_Primitive *cond_prim = (OS_Posix_Primitive *)cond_handle.h[0];
  OS_Posix_Primitive *rwlock_prim = (OS_Posix_Primitive *)rwlock_handle.h[0];

  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += (i32)((f64)wait_at_most_microsec / 1e6);
    abstime.tv_nsec += (i32)1e3 * (wait_at_most_microsec % (u32)1e6);
    if (abstime.tv_nsec >= (i32)1e9) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= (i32)1e9;
    }

    pthread_mutex_lock(&cond_prim->condvar.mutex);
    if (pthread_cond_timedwait(&cond_prim->condvar.cond, &cond_prim->condvar.mutex,
                               &abstime) != 0) {
      (void)pthread_mutex_unlock(&cond_prim->condvar.mutex);
      return false;
    }
    (void)pthread_rwlock_wrlock(&rwlock_prim->rwlock);
  } else {
    pthread_mutex_lock(&cond_prim->condvar.mutex);
    (void)pthread_cond_wait(&cond_prim->condvar.cond, &cond_prim->condvar.mutex);
    (void)pthread_rwlock_wrlock(&rwlock_prim->rwlock);
  }

  (void)pthread_mutex_unlock(&cond_prim->condvar.mutex);
  return true;
}

fn bool os_cond_free(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  i32 res = pthread_cond_destroy(&prim->condvar.cond) &
            pthread_mutex_destroy(&prim->condvar.mutex);
  os_posix_primitive_release(prim);
  return res == 0;
}

fn OS_Handle os_semaphore_alloc(OS_SemaphoreKind kind, u32 init_count,
                                u32 max_count, String8 name) {
  OS_Posix_Primitive *prim = os_posix_primitive_alloc(OS_Posix_Primitive_Semaphore);
  prim->semaphore.kind = kind;
  prim->semaphore.max_count = max_count;
  switch (kind) {
    case OS_SemaphoreKind_Thread: {
      prim->semaphore.sem = (sem_t *)os_reserve(sizeof(sem_t));
      os_commit(prim->semaphore.sem, sizeof(sem_t));
      if (sem_init(prim->semaphore.sem, 0, init_count)) {
        os_release(prim->semaphore.sem, sizeof(sem_t));
        prim->semaphore.sem = 0;
      }
    } break;
    case OS_SemaphoreKind_Process: {
      Assert(name.size > 0);
      Scratch scratch = ScratchBegin(0, 0);
      char *path = cstr_from_str8(scratch.arena, name);
      (void)sem_unlink(path);
      prim->semaphore.sem = sem_open(path, O_CREAT,
                                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, init_count);
      if (prim->semaphore.sem == SEM_FAILED) {
        prim->semaphore.sem = 0;
      }
      ScratchEnd(scratch);
    } break;
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn bool os_semaphore_signal(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  if (prim->semaphore.count + 1 >= prim->semaphore.max_count ||
      sem_post(prim->semaphore.sem)) {
    return false;
  }
  return ++prim->semaphore.count;
}

fn bool os_semaphore_wait(OS_Handle handle, u32 wait_at_most_microsec) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += (i32)((f64)wait_at_most_microsec / 1e6);
    abstime.tv_nsec += (i32)1e3 * (wait_at_most_microsec % (u32)1e6);
    if (abstime.tv_nsec >= (i32)1e9) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= (i32)1e9;
    }

    return sem_timedwait(prim->semaphore.sem, &abstime) == 0;
  } else {
    return sem_wait(prim->semaphore.sem) == 0;
  }
}

fn bool os_semaphore_trywait(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  return sem_trywait(prim->semaphore.sem) == 0;
}

fn void os_semaphore_free(OS_Handle handle) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive *)handle.h[0];
  switch (prim->semaphore.kind) {
    case OS_SemaphoreKind_Thread: {
      (void)sem_destroy(prim->semaphore.sem);
      os_release(prim->semaphore.sem, sizeof(sem_t));
    } break;
    case OS_SemaphoreKind_Process: {
      (void)sem_close(prim->semaphore.sem);
    } break;
  }
  os_posix_primitive_release(prim);
}

fn SharedMem os_sharedmem_open(String8 name, i64 bytes, OS_AccessFlag flags) {
  SharedMem res = {0};
  res.path = name;

  i32 access_flags = os_posix_flags_from_acf(flags);
  Scratch scratch = ScratchBegin(0, 0);
  i32 shm_fd = shm_open(cstr_from_str8(scratch.arena, name), access_flags,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (shm_fd < 0) { return res; }
  res.file_handle.h[0] = (u64)shm_fd;
  ScratchEnd(scratch);

  Assert(bytes >= 0);
  Assert(!ftruncate((i32)res.file_handle.h[0], bytes));
  res.prop.size = bytes;
  res.content = (u8*)mmap(0, (u64)bytes, PROT_READ | PROT_WRITE,
                           MAP_SHARED, (i32)res.file_handle.h[0], 0);
  Assert(res.content != MAP_FAILED);

  return res;
}

fn void os_sharedmem_close(SharedMem *shm) {
  os_posix_sharedmem_unlink_name(shm);
  munmap(shm->content, (usize)shm->prop.size);
  close((i32)shm->file_handle.h[0]);
}

// =============================================================================
// Dynamic libraries
fn OS_Handle os_lib_open(String8 path) {
  OS_Handle result = {0};
  Scratch scratch = ScratchBegin(0, 0);
  void *handle = dlopen(cstr_from_str8(scratch.arena, path),
                        RTLD_NOW | RTLD_GLOBAL);
  Assert(handle);
  ScratchEnd(scratch);
  return result;
}

fn Func_Void *os_lib_lookup(OS_Handle lib, String8 symbol) {
  void *handle = (void*)lib.h[0];
  Scratch scratch = ScratchBegin(0, 0);
  Func_Void *result = (Func_Void*)(u64)dlsym(handle, cstr_from_str8(scratch.arena, symbol));
  Assert(result);
  ScratchEnd(scratch);
  return result;
}

fn i32 os_lib_close(OS_Handle lib) {
  void *handle = (void*)lib.h[0];
  return (handle ? dlclose(handle) : 0);
}

// =============================================================================
// Misc
fn String8 os_currentDir(Arena *arena) {
  char *wd = getcwd(0, 0);
  isize size = str8_len(wd);
  u8 *copy = arena_push_many(arena, u8, size);
  memcopy(copy, wd, size);
  free(wd);
  String8 res = str8(copy, size);
  return res;
}

// =============================================================================
// Networking
fn NetInterfaceList os_net_interfaces(Arena *arena) {
  NetInterfaceList res = {0};
  struct ifaddrs *ifaddr;
  if (getifaddrs(&ifaddr) == -1) {
    return res;
  }

  for (struct ifaddrs *ifa = ifaddr; ifa && ifa->ifa_addr; ifa = ifa->ifa_next) {
    if (!(ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6))
      { continue; }

    NetInterface *inter = arena_push(arena, NetInterface);

    String8 interface = str8_from_cstr(ifa->ifa_name);
    inter->name.str = arena_push_many(arena, u8, interface.size);
    inter->name.size = interface.size;
    memcopy(inter->name.str, interface.str, interface.size);

    if (ifa->ifa_addr->sa_family == AF_INET) {
      inter->version = OS_Net_Network_IPv4;

      struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
      struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
      inter->ipv4.addr = *(IPv4*)&addr->sin_addr;
      inter->ipv4.netmask = *(IPv4*)&netmask->sin_addr;

      char cstrip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &addr->sin_addr, cstrip, INET_ADDRSTRLEN);
      String8 strip = str8_from_cstr(cstrip);
      inter->strip.str = arena_push_many(arena, u8, strip.size);
      inter->strip.size = strip.size;
      memcopy(inter->strip.str, strip.str, strip.size);
    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
      inter->version = OS_Net_Network_IPv6;

      struct sockaddr_in6 *addr = (struct sockaddr_in6 *)ifa->ifa_addr;
      struct sockaddr_in6 *netmask = (struct sockaddr_in6 *)ifa->ifa_netmask;
      inter->ipv6.addr = *(IPv6*)&addr->sin6_addr;
      inter->ipv6.netmask = *(IPv6*)&netmask->sin6_addr;

      char cstrip[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &addr->sin6_addr, cstrip, INET6_ADDRSTRLEN);
      String8 strip = str8_from_cstr(cstrip);
      inter->strip.str = arena_push_many(arena, u8, strip.size);
      inter->strip.size = strip.size;
      memcopy(inter->strip.str, strip.str, strip.size);
    }

    DLLPushBack(res.first, res.last, inter);
  }

  freeifaddrs(ifaddr);
  return res;
}

fn IP os_net_ip_from_str8(String8 name, OS_Net_Network hint) {
  IP res = {0};
  struct addrinfo *info = 0;
  struct addrinfo hints = {0};
  switch (hint) {
  case OS_Net_Network_Invalid: {
    hints.ai_family = AF_UNSPEC;
  } break;
  case OS_Net_Network_IPv4: {
    hints.ai_family = AF_INET;
  } break;
  case OS_Net_Network_IPv6: {
    hints.ai_family = AF_INET6;
  } break;
  }

  Scratch scratch = ScratchBegin(0, 0);
  getaddrinfo(cstr_from_str8(scratch.arena, name), 0,
              &hints, &info);
  ScratchEnd(scratch);
  if (!info) { return res; }
  if (info->ai_family == AF_INET) {
    res.version = OS_Net_Network_IPv4;
    memcopy(res.v4.bytes,
            &((struct sockaddr_in*)info->ai_addr)->sin_addr,
            4 * sizeof(u8));
  } else if (info->ai_family == AF_INET6) {
    res.version = OS_Net_Network_IPv6;
    memcopy(res.v6.words,
            &((struct sockaddr_in6*)info->ai_addr)->sin6_addr,
            8 * sizeof(u16));
  }

  freeaddrinfo(info);
  return res;
}

fn OS_Socket os_socket_open(String8 name, u16 port,
                            OS_Net_Transport protocol) {
  OS_Socket res = {0};
  OS_Posix_Primitive *prim = os_posix_primitive_alloc(OS_Posix_Primitive_Socket);
  IP server = os_net_ip_from_str8(name, 0);
  i32 ctype = 0, cdomain = 0;
  switch (server.version) {
    case OS_Net_Network_IPv4: {
      cdomain = AF_INET;
      struct sockaddr_in *addr = (struct sockaddr_in*)&prim->socket.addr;
      addr->sin_family = (sa_family_t)cdomain;
      addr->sin_port = htons(port);
      memcopy(&addr->sin_addr, server.v4.bytes, 4 * sizeof(u8));
      prim->socket.size = sizeof(struct sockaddr_in);
    } break;
    case OS_Net_Network_IPv6: {
      cdomain = AF_INET6;
      struct sockaddr_in6 *addr = (struct sockaddr_in6*)&prim->socket.addr;
      addr->sin6_family = (sa_family_t)cdomain;
      addr->sin6_port = htons(port);
      memcopy(&addr->sin6_addr, server.v4.bytes, 8 * sizeof(u16));
      prim->socket.size = sizeof(struct sockaddr_in6);
    } break;
    default: {
      Assert(false);
    }
  }
  switch (protocol) {
    case OS_Net_Transport_TCP: {
      ctype = SOCK_STREAM;
    } break;
    case OS_Net_Transport_UDP: {
      ctype = SOCK_DGRAM;
    } break;
    default: {
      Assert(false);
    }
  }

  prim->socket.fd = socket(cdomain, ctype, 0);
  if (prim->socket.fd == -1) {
    os_posix_primitive_release(prim);
    perror("os_socket");
    return res;
  }

  res.protocol_transport = protocol;
  res.server.addr = server;
  res.server.port = port;
  res.handle.h[0] = (u64)prim;
  return res;
}

fn void os_socket_listen(OS_Socket *socket, u8 max_backlog) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive*)socket->handle.h[0];

  i32 optval = 1;
  (void)setsockopt(prim->socket.fd, SOL_SOCKET,
                   SO_REUSEPORT | SO_REUSEADDR, &optval,
                   sizeof(optval));
  (void)bind(prim->socket.fd, &prim->socket.addr, prim->socket.size);
  (void)listen(prim->socket.fd, max_backlog);
}

fn OS_Socket os_socket_accept(OS_Socket *socket) {
  OS_Socket res = {0};

  OS_Posix_Primitive *server_prim = (OS_Posix_Primitive*)socket->handle.h[0];
  OS_Posix_Primitive *client_prim = os_posix_primitive_alloc(OS_Posix_Primitive_Socket);
  client_prim->socket.size = sizeof(client_prim->socket.addr);
  client_prim->socket.fd = accept(server_prim->socket.fd,
                                  &client_prim->socket.addr,
                                  &client_prim->socket.size);
  if (client_prim->socket.fd == -1) {
    perror("accept");
    return res;
  }

  memcopy(&res, socket, sizeof(OS_Socket));
  res.handle.h[0] = (u64)client_prim;
  switch (client_prim->socket.addr.sa_family) {
  case AF_INET: {
    struct sockaddr_in *client = (struct sockaddr_in *)&client_prim->socket.addr;
    memcopy(res.client.addr.v4.bytes, &client->sin_addr, 4 * sizeof(u8));
    res.client.port = client->sin_port;
    res.client.addr.version = OS_Net_Network_IPv4;
  } break;
  case AF_INET6: {
    struct sockaddr_in6 *client = (struct sockaddr_in6 *)&client_prim->socket.addr;
    memcopy(res.client.addr.v6.words, &client->sin6_addr, 8 * sizeof(u16));
    res.client.port = client->sin6_port;
    res.client.addr.version = OS_Net_Network_IPv6;
  } break;
  default: {
    Err("Unknown network protocol: %ld", client_prim->socket.addr.sa_family);
  }
  }
  return res;
}

fn u8* os_socket_recv(Arena *arena, OS_Socket *client, i64 buffer_size) {
  u8 *res = arena_push_many(arena, u8, buffer_size);
  OS_Posix_Primitive *prim = (OS_Posix_Primitive*)client->handle.h[0];
  i64 read_status = read(prim->socket.fd, res, (u64)buffer_size);
  Assert(read_status != -1);
  return res;
}

fn void os_socket_connect(OS_Socket *server) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive*)server->handle.h[0];
  if (connect(prim->socket.fd, &prim->socket.addr, prim->socket.size) == -1) {
    perror("os_socket_connect");
    return;
  }

  struct sockaddr client = {0};
  socklen_t client_len = sizeof(struct sockaddr);
  (void)getsockname(prim->socket.fd, &client, &client_len);
  switch (client.sa_family) {
  case AF_INET: {
    struct sockaddr_in *clientv4 = (struct sockaddr_in *)&client;
    memcopy(server->client.addr.v4.bytes, &clientv4->sin_addr, 4 * sizeof(u8));
    server->client.port = clientv4->sin_port;
    server->client.addr.version = OS_Net_Network_IPv4;
  } break;
  case AF_INET6: {
    struct sockaddr_in6 *clientv6 = (struct sockaddr_in6 *)&client;
    memcopy(server->client.addr.v6.words, &clientv6->sin6_addr, 8 * sizeof(u16));
    server->client.port = clientv6->sin6_port;
    server->client.addr.version = OS_Net_Network_IPv6;
  } break;
  }
}

fn void os_socket_send(OS_Socket *socket, String8 msg) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive*)socket->handle.h[0];
  Scratch scratch = ScratchBegin(0, 0);
  send(prim->socket.fd, cstr_from_str8(scratch.arena, msg), (usize)msg.size, 0);
  ScratchEnd(scratch);
}

fn void os_socket_close(OS_Socket *socket) {
  OS_Posix_Primitive *prim = (OS_Posix_Primitive*)socket->handle.h[0];
  shutdown(prim->socket.fd, SHUT_RDWR);
  close(prim->socket.fd);
  os_posix_primitive_release(prim);
}

// =============================================================================
// File reading and writing/appending
fn String8 os_fs_read(Arena *arena, OS_Handle file) {
  int fd = (i32)file.h[0];
  String8 result = {0};
  if (!fd) { return result; }

  struct stat file_stat;
  if (!fstat(fd, &file_stat)) {
    u8 *buffer = arena_push_many(arena, u8, file_stat.st_size);
    if (pread(fd, buffer, (usize)file_stat.st_size, 0) >= 0) {
      result.str = buffer;
      result.size = file_stat.st_size;
    }
  }

  return result;
}

fn String8 os_fs_read_virtual(Arena *arena, OS_Handle file, i64 bytes) {
  Assert(bytes > 0);
  int fd = (i32)file.h[0];
  String8 result = {0};
  if (!fd) { return result; }

  u8 *buffer = arena_push_many(arena, u8, bytes);
  if (pread(fd, buffer, (u64)bytes, 0) >= 0) {
    result.str = buffer;
    result.size = str8_len((char *)buffer);
  }
  return result;
}

fn bool os_fs_write(OS_Handle file, String8 content) {
  if (!file.h[0]) { return false; }
  return write((i32)file.h[0], content.str, (usize)content.size)
         == content.size;
}

fn FS_Properties os_fs_get_properties(OS_Handle file) {
  FS_Properties result = {0};
  if (!file.h[0]) { return result; }

  struct stat file_stat;
  if (fstat((i32)file.h[0], &file_stat) == 0) {
    result = os_posix_properties_from_stat(&file_stat);
  }
  return result;
}

fn String8 os_fs_readlink(Arena *arena, String8 path) {
  String8 res = {0};
  res.str = arena_push_many(arena, u8, PATH_MAX);
  res.size = readlink((char *)path.str, (char *)res.str, PATH_MAX);
  if (res.size <= 0) {
    res.str = 0;
    res.size = 0;
  }

  arena_pop(arena, PATH_MAX - res.size);
  return res;
}

// =============================================================================
// Memory mapping files
fn void* os_fs_map(OS_Handle fd, i32 offset, i64 bytes) {
  Assert(offset >= 0);
  Assert(bytes > 0);

  i32 flags = fcntl((i32)fd.h[0], F_GETFL);
  if (flags < 0) { return NULL; }
  flags &= O_ACCMODE;
  if (!flags) {
    flags = PROT_READ;
  } else {
    flags = PROT_READ | PROT_WRITE;
  }
  void *res = mmap(0, (usize)bytes, flags, MAP_SHARED, (i32)fd.h[0], offset);
  return res;
}

fn bool os_fs_unmap(void *fmap, i64 bytes) {
  Assert(bytes > 0);
  return !munmap(fmap, (u64)bytes);
}

fn File os_fs_fopen(Arena *arena, OS_Handle fd) {
  Unused(arena);
  File file = {0};
  file.file_handle = fd;
  file.path = os_fs_path_from_handle(arena, fd);
  file.prop = os_fs_get_properties(file.file_handle);
  file.content = (u8 *)os_fs_map(fd, 0, file.prop.size);
  file.mmap_handle.h[0] = (u64)file.content;
  return file;
}

fn File os_fs_fopen_tmp(Arena *arena) {
  char path[] = "/tmp/base-XXXXXX";
  i32 fd = mkstemp(path);

  String8 pathstr = str8(arena_push_many(arena, u8, Arrsize(path)),
                         Arrsize(path));
  memcopy(pathstr.str, path, Arrsize(path));

  File file = {0};
  file.file_handle.h[0] = (u64)fd;
  file.path = pathstr;
  file.prop = os_fs_get_properties(file.file_handle);
  file.content = (u8*)mmap(0, (usize)ClampBot(file.prop.size, 1),
                           PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  file.mmap_handle.h[0] = (u64)file.content;
  return file;
}

fn bool os_fs_fclose(File *file) {
  return os_fs_unmap((void*)file->mmap_handle.h[0], file->prop.size) &&
         os_fs_close(file->file_handle);
}

fn bool os_fs_fresize(File *file, i64 bytes) {
  Assert(bytes >= 0);
  if (ftruncate((i32)file->file_handle.h[0], bytes) < 0) {
    return false;
  }

  file->prop.size = bytes;
  munmap(file->content, (u64)file->prop.size);
  file->content = (u8*)mmap(0, (u64)bytes, PROT_READ | PROT_WRITE, MAP_SHARED,
                            (i32)file->file_handle.h[0], 0);
  return file->content != MAP_FAILED;
}

fn bool os_fs_file_has_changed(File *file) {
  FS_Properties prop = os_fs_get_properties(file->file_handle);
  return (file->prop.last_access_time != prop.last_access_time) ||
         (file->prop.last_modification_time != prop.last_modification_time) ||
         (file->prop.last_status_change_time != prop.last_status_change_time);
}

fn bool os_fs_fdelete(File *file) {
  return unlink((char *) file->path.str) >= 0 && os_fs_fclose(file);
}

fn bool os_fs_frename(File *file, String8 to) {
  return rename((char *) file->path.str, (char *) to.str) >= 0;
}

// =============================================================================
// Misc operation on the filesystem
fn bool os_fs_delete(String8 filepath) {
  Assert(filepath.size != 0);
  Scratch scratch = ScratchBegin(0, 0);
  i32 res = unlink(cstr_from_str8(scratch.arena, filepath));
  ScratchEnd(scratch);
  return res >= 0;
}

fn bool os_fs_rename(String8 filepath, String8 to) {
  Assert(filepath.size != 0 && to.size != 0);
  return rename((char *)filepath.str, (char *)to.str) >= 0;
}

fn bool os_fs_mkdir(String8 path) {
  Assert(path.size != 0);
  return mkdir((char *)path.str,
               S_IRWXU | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)) >= 0;
}

fn bool os_fs_rmdir(String8 path) {
  Assert(path.size != 0);
  return rmdir((char *)path.str) >= 0;
}

fn String8 os_fs_filename_from_path(Arena *arena, String8 path) {
  Scratch scratch = ScratchBegin(&arena, 1);
  char *fullname = basename(cstr_from_str8(scratch.arena, path));
  ScratchEnd(scratch);

  isize last_dot = 0, size = 0;
  for (; fullname[size]; ++size) {
    if (fullname[size] == '.') { last_dot = size; }
  }
  return str8((u8*)fullname, size - (size - last_dot));
}

// =============================================================================
// File iteration
fn OS_FileIter* os_fs_iter_begin(Arena *arena, String8 path) {
  OS_FileIter *os_iter = arena_push(arena, OS_FileIter);
  OS_Posix_FileIter *iter = (OS_Posix_FileIter *)os_iter->memory;
  iter->path = path;
  Scratch scratch = ScratchBegin(&arena, 1);
  iter->dir = opendir(cstr_from_str8(scratch.arena, path));
  ScratchEnd(scratch);

  return os_iter;
}

fn bool os_fs_iter_next(Arena *arena, OS_FileIter *os_iter, OS_FileInfo *info_out) {
  local const String8 currdir = StrlitInit(".");
  local const String8 parentdir = StrlitInit("..");

  String8 str = {0};
  OS_Posix_FileIter *iter = (OS_Posix_FileIter *)os_iter->memory;
  struct dirent *entry = 0;

  if (!iter->dir) { return false; }
  Scratch scratch = ScratchBegin(&arena, 1);
  do {
    do {
      entry = readdir(iter->dir);
      if (!entry) { return false; }
      str = str8_from_cstr(entry->d_name);
    } while (str8_eq(str, currdir) || str8_eq(str, parentdir));

    str = str8_format(scratch.arena, "%.*s/%.*s", Strexpand(iter->path), Strexpand(str));
    struct stat file_stat = {0};
    if (stat((char *)str.str, &file_stat) != 0) {
      ScratchEnd(scratch);
      return false;
    }

    info_out->properties = os_posix_properties_from_stat(&file_stat);
  } while (os_iter->filter_allowed && !(info_out->properties.type & os_iter->filter_allowed));

  info_out->name.size = str.size;
  info_out->name.str = arena_push_many(arena, u8, str.size);
  memcopy(info_out->name.str, str.str, str.size);
  ScratchEnd(scratch);
  return true;
}

fn void os_fs_iter_end(OS_FileIter *os_iter) {
  OS_Posix_FileIter *iter = (OS_Posix_FileIter *)os_iter->memory;
  if (iter->dir) { closedir(iter->dir); }
}

// ===================================================================
// Platform dependent functions
internal OS_Posix_Primitive* os_posix_primitive_alloc(OS_Posix_PrimitiveType type) {
  pthread_mutex_lock(&os_posix_state.primitive_lock);
  OS_Posix_Primitive *res = os_posix_state.primitive_freelist;
  if (res) {
    StackPop(os_posix_state.primitive_freelist);
    memzero(res, sizeof(OS_Posix_Primitive));
  } else {
    res = arena_push(os_posix_state.arena, OS_Posix_Primitive);
  }
  pthread_mutex_unlock(&os_posix_state.primitive_lock);

  res->type = type;
  return res;
}

internal void os_posix_primitive_release(OS_Posix_Primitive *ptr) {
  pthread_mutex_lock(&os_posix_state.primitive_lock);
  StackPush(os_posix_state.primitive_freelist, ptr);
  pthread_mutex_unlock(&os_posix_state.primitive_lock);
}

internal void* os_posix_thread_entry(void *args) {
  OS_Posix_Primitive *wrap_args = (OS_Posix_Primitive *)args;
  Func_Thread *func = (Func_Thread *)wrap_args->thread.func;
  func(wrap_args->thread.args);
  return 0;
}

internal FS_Properties os_posix_properties_from_stat(struct stat *stat) {
  FS_Properties result = {0};
  result.ownerID = stat->st_uid;
  result.groupID = stat->st_gid;
  result.size = stat->st_size;
  result.last_access_time = time64_from_unix((u64)stat->st_atime);
  result.last_modification_time = time64_from_unix((u64)stat->st_mtime);
  result.last_status_change_time = time64_from_unix((u64)stat->st_ctime);

  switch (stat->st_mode & S_IFMT) {
  case S_IFBLK:  result.type = OS_FileType_BlkDevice;  break;
  case S_IFCHR:  result.type = OS_FileType_CharDevice; break;
  case S_IFDIR:  result.type = OS_FileType_Dir;        break;
  case S_IFIFO:  result.type = OS_FileType_Pipe;       break;
  case S_IFLNK:  result.type = OS_FileType_Link;       break;
  case S_IFSOCK: result.type = OS_FileType_Socket;     break;
  case S_IFREG:  result.type = OS_FileType_Regular;    break;
  }

  result.user = OS_Permissions_Unknown;
  if (stat->st_mode & S_IRUSR) { result.user |= OS_Permissions_Read; }
  if (stat->st_mode & S_IWUSR) { result.user |= OS_Permissions_Write; }
  if (stat->st_mode & S_IXUSR) { result.user |= OS_Permissions_Execute; }

  result.group = OS_Permissions_Unknown;
  if (stat->st_mode & S_IRGRP) { result.group |= OS_Permissions_Read; }
  if (stat->st_mode & S_IWGRP) { result.group |= OS_Permissions_Write; }
  if (stat->st_mode & S_IXGRP) { result.group |= OS_Permissions_Execute; }

  result.other = OS_Permissions_Unknown;
  if (stat->st_mode & S_IROTH) { result.other |= OS_Permissions_Read; }
  if (stat->st_mode & S_IWOTH) { result.other |= OS_Permissions_Write; }
  if (stat->st_mode & S_IXOTH) { result.other |= OS_Permissions_Execute; }

  return result;
}

internal String8 os_posix_gethostname(void) {
  char name[HOST_NAME_MAX];
  (void)gethostname(name, HOST_NAME_MAX);

  String8 namestr = str8((u8 *)name, str8_len(name));
  return namestr;
}

internal i32 os_posix_flags_from_acf(OS_AccessFlag acf) {
  i32 res = 0;
  if ((acf & OS_AccessFlag_Read) && (acf & OS_AccessFlag_Write)) {
    res |= O_RDWR;
  } else if (acf & OS_AccessFlag_Read) {
    res |= O_RDONLY;
  } else if (acf & OS_AccessFlag_Write) {
    res |= O_WRONLY | O_CREAT | O_TRUNC;
  }
  if (acf & OS_AccessFlag_Execute) { res |= O_EXCL | O_CREAT; }
  if (acf & OS_AccessFlag_Append) { res |= O_APPEND | O_CREAT; }
  return res;
}

internal void os_posix_sharedmem_resize(SharedMem *shm, i64 bytes) {
  Assert(bytes);
  Assert(!ftruncate((i32)shm->file_handle.h[0], bytes));
  munmap(shm->content, (u64)shm->prop.size);
  shm->content = (u8*)mmap(0, (u64)bytes, PROT_READ | PROT_WRITE,
                           MAP_SHARED, (i32)shm->file_handle.h[0], 0);
  Assert(shm->content != MAP_FAILED);
  shm->prop.size = bytes;
}

internal void os_posix_sharedmem_unlink_name(SharedMem *shm) {
  if (!shm->path.size) { return; }
  Scratch scratch = ScratchBegin(0, 0);
  shm_unlink(cstr_from_str8(scratch.arena, shm->path));
  ScratchEnd(scratch);
}

internal void os_posix_sleep_nanoseconds(u32 ns) {
  struct timespec duration = {
    .tv_sec = ns / (u32)1e9,
    .tv_nsec = ns % (u32)1e9,
  };
  (void)nanosleep(&duration, 0);
}
