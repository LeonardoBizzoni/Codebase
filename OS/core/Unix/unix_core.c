global UNX_State unx_state = {0};

fn OS_SystemInfo *os_getSystemInfo(void) {
  return &unx_state.info;
}

fn UNX_Primitive* unx_primitive_alloc(UNX_PrimitiveType type) {
  pthread_mutex_lock(&unx_state.primitive_lock);
  UNX_Primitive *res = unx_state.primitive_freelist;
  if (res) {
    StackPop(unx_state.primitive_freelist);
    memzero(res, sizeof(UNX_Primitive));
  } else {
    res = New(unx_state.arena, UNX_Primitive);
  }
  pthread_mutex_unlock(&unx_state.primitive_lock);

  res->type = type;
  return res;
}

fn void unx_primitive_free(UNX_Primitive *ptr) {
  pthread_mutex_lock(&unx_state.primitive_lock);
  StackPush(unx_state.primitive_freelist, ptr);
  pthread_mutex_unlock(&unx_state.primitive_lock);
}

fn void* unx_thread_entry(void *args) {
  UNX_Primitive *wrap_args = (UNX_Primitive *)args;
  ThreadFunc *func = (ThreadFunc *)wrap_args->thread.func;
  func(wrap_args->thread.args);
  return 0;
}

fn FS_Properties unx_properties_from_stat(struct stat *stat) {
  FS_Properties result = {0};
  result.ownerID = stat->st_uid;
  result.groupID = stat->st_gid;
  result.size = (usize)stat->st_size;
  result.last_access_time = time64_from_unix(stat->st_atime);
  result.last_modification_time = time64_from_unix(stat->st_mtime);
  result.last_status_change_time = time64_from_unix(stat->st_ctime);

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

fn String8 unx_gethostname(void) {
  char name[HOST_NAME_MAX];
  (void)gethostname(name, HOST_NAME_MAX);

  String8 namestr = str8((u8 *)name, str8_len(name));
  return namestr;
}

fn i32 unx_flags_from_acf(OS_AccessFlags acf) {
  i32 res = 0;
  if ((acf & OS_acfRead) && (acf & OS_acfWrite)) {
    res |= O_RDWR;
  } else if(acf & OS_acfRead) {
    res |= O_RDONLY;
  } else if(acf & OS_acfWrite) {
    res |= O_WRONLY | O_CREAT | O_TRUNC;
  }
  if (acf & OS_acfExecute) { res |= O_EXCL | O_CREAT; }
  if (acf & OS_acfAppend) { res |= O_APPEND | O_CREAT; }
  return res;
}

// =============================================================================
// DateTime
fn time64 os_local_now(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64_from_unix(tms.tv_sec + unx_state.unix_utc_offset);
  res |= (u64)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_local_dateTimeNow(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = datetime_from_unix(tms.tv_sec + unx_state.unix_utc_offset);
  res.ms = (u16)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn time64 os_local_fromUTCTime64(time64 t) {
  u64 utc_time = unix_from_time64(t);
  time64 res = time64_from_unix(utc_time + unx_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_local_fromUTCDateTime(DateTime *dt) {
  u64 utc_time = unix_from_datetime(dt);
  DateTime res = datetime_from_unix(utc_time + unx_state.unix_utc_offset);
  res.ms = dt->ms;
  return res;
}

fn time64 os_utc_now(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64_from_unix(tms.tv_sec);
  res |= (u64)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_utc_dateTimeNow(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = datetime_from_unix(tms.tv_sec);
  res.ms = (u16)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn time64 os_utc_localizedTime64(i8 utc_offset) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64_from_unix(tms.tv_sec + utc_offset * UNIX_HOUR);
  res |= (u64)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_utc_localizedDateTime(i8 utc_offset) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = datetime_from_unix(tms.tv_sec + utc_offset * UNIX_HOUR);
  res.ms = (u16)((f64)tms.tv_nsec / 1e6);
  return res;
}

fn time64 os_utc_fromLocalTime64(time64 t) {
  u64 local_time = unix_from_time64(t);
  time64 res = time64_from_unix(local_time - unx_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_utc_fromLocalDateTime(DateTime *dt) {
  u64 local_time = unix_from_datetime(dt);
  DateTime res = datetime_from_unix(local_time - unx_state.unix_utc_offset);
  res.ms = dt->ms;
  return res;
}

fn void os_sleep_milliseconds(u32 ms) {
  usleep((useconds_t)(ms * 1e3));
}

fn void unx_sleep_nanoseconds(u32 ns) {
  struct timespec duration = {
    .tv_sec = ns / (u32)1e9,
    .tv_nsec = ns % (u32)1e9,
  };
  (void)nanosleep(&duration, 0);
}

fn OS_Handle os_timer_start(void) {
  UNX_Primitive *prim = unx_primitive_alloc(UNX_Primitive_Timer);
  if (clock_gettime(CLOCK_MONOTONIC, &prim->timer) != 0) {
    unx_primitive_free(prim);
    prim = 0;
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn u64 os_timer_elapsed_start2end(OS_TimerGranularity unit, OS_Handle start, OS_Handle end) {
  struct timespec tstart = ((UNX_Primitive *)start.h[0])->timer;
  struct timespec tend = ((UNX_Primitive *)end.h[0])->timer;
  u64 diff_nanos = (tend.tv_sec - tstart.tv_sec) * (u64)1e9 +
                   (tend.tv_nsec - tstart.tv_nsec);

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

fn void os_timer_free(OS_Handle handle) {
  unx_primitive_free((UNX_Primitive*)handle.h[0]);
}

// =============================================================================
// Memory allocation
fn void* os_reserve(usize size) {
  void *res = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (res == MAP_FAILED) {
    res = 0;
  }

  return res;
}

fn void os_release(void *base, usize size) {
  (void)munmap(base, size);
}

fn void os_commit(void *base, usize size) {
  Assert(mprotect(base, size, PROT_READ | PROT_WRITE) == 0);
}

fn void os_decommit(void *base, usize size) {
  (void)mprotect(base, size, PROT_NONE);
  (void)madvise(base, size, MADV_DONTNEED);
}

// =============================================================================
// Threads & Processes stuff
fn OS_Handle os_thread_start(ThreadFunc *thread_main, void *args) {
  Assert(thread_main);

  UNX_Primitive *prim = unx_primitive_alloc(UNX_Primitive_Thread);
  prim->thread.func = thread_main;
  prim->thread.args = args;

  if (pthread_create(&prim->thread.handle, 0, unx_thread_entry, prim) != 0) {
    unx_primitive_free(prim);
    prim = 0;
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_thread_kill(OS_Handle thd_handle) {
  UNX_Primitive *prim = (UNX_Primitive *)thd_handle.h[0];
  if (!prim || prim->type != UNX_Primitive_Thread) { return; }
  (void)pthread_kill(prim->thread.handle, 0);
  unx_primitive_free(prim);
}

fn void os_thread_cancel(OS_Handle thd_handle) {
  UNX_Primitive *prim = (UNX_Primitive *)thd_handle.h[0];
  if (!prim || prim->type != UNX_Primitive_Thread) { return; }
  (void)pthread_cancel(prim->thread.handle);
  unx_primitive_free(prim);
}

fn bool os_thread_join(OS_Handle thd_handle) {
  UNX_Primitive *prim = (UNX_Primitive *)thd_handle.h[0];
  if (!prim || prim->type != UNX_Primitive_Thread) { return false; }
  bool res = pthread_join(prim->thread.handle, 0) == 0;
  unx_primitive_free(prim);
  return res;
}

fn void os_thread_cancelpoint(void) {
  pthread_testcancel();
}

fn OS_ProcHandle os_proc_spawn(void) {
  UNX_Primitive *prim = unx_primitive_alloc(UNX_Primitive_Process);
  prim->proc = fork();

  OS_ProcHandle res = {prim->proc == 0, {{(u64)prim}}};
  return res;
}

fn void os_proc_kill(OS_ProcHandle proc) {
  Assert(!proc.is_child);
  UNX_Primitive *prim = (UNX_Primitive *)proc.handle.h[0];
  (void)kill(prim->proc, SIGKILL);
  unx_primitive_free(prim);
}

fn OS_ProcStatus os_proc_wait(OS_ProcHandle proc) {
  Assert(!proc.is_child);
  UNX_Primitive *prim = (UNX_Primitive *)proc.handle.h[0];
  i32 child_res;
  (void)waitpid(prim->proc, &child_res, 0);
  unx_primitive_free(prim);

  OS_ProcStatus res = {0};
  if (WIFEXITED(child_res)) {
    res.state = OS_ProcState_Finished;
    res.exit_code = WEXITSTATUS(child_res);
  } else if (WCOREDUMP(child_res)) {
    res.state = OS_ProcState_CoreDump;
  } else if (WIFSIGNALED(child_res)) {
    res.state = OS_ProcState_Killed;
  }
  return res;
}

fn void os_exit(u8 status_code) {
  exit(status_code);
}

fn void os_atexit(VoidFunc *callback) {
  atexit(callback);
}

fn OS_Handle os_mutex_alloc(void) {
  UNX_Primitive *prim = unx_primitive_alloc(UNX_Primitive_Mutex);
  pthread_mutexattr_t attr;
#ifndef PLATFORM_CODERBOT
  if (pthread_mutexattr_init(&attr) != 0 ||
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
#else
  if (pthread_mutexattr_init(&attr) != 0) {
#endif
    unx_primitive_free(prim);
    prim = 0;
  } else {
    (void)pthread_mutex_init(&prim->mutex, &attr);
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_mutex_lock(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  (void)pthread_mutex_lock(&prim->mutex);
}

fn bool os_mutex_trylock(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  return pthread_mutex_trylock(&prim->mutex) == 0;
}

fn void os_mutex_unlock(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  (void)pthread_mutex_unlock(&prim->mutex);
}

fn void os_mutex_free(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  pthread_mutex_destroy(&prim->mutex);
  unx_primitive_free(prim);
}

fn OS_Handle os_rwlock_alloc(void) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = unx_primitive_alloc(UNX_Primitive_Rwlock);
  pthread_rwlock_init(&prim->rwlock, 0);

  OS_Handle res = {{(u64)prim}};
  return res;
#else
  OS_Handle res = {{0}};
  return res;
#endif
}

fn void os_rwlock_read_lock(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  (void)pthread_rwlock_rdlock(&prim->rwlock);
#endif
}

fn bool os_rwlock_read_trylock(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  return pthread_rwlock_tryrdlock(&prim->rwlock) == 0;
#else
  return false;
#endif
}

fn void os_rwlock_read_unlock(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  (void)pthread_rwlock_unlock(&prim->rwlock);
#endif
}

fn void os_rwlock_write_lock(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  (void)pthread_rwlock_wrlock(&prim->rwlock);
#endif
}

fn bool os_rwlock_write_trylock(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  return pthread_rwlock_trywrlock(&prim->rwlock) == 0;
#else
  return false;
#endif
}

fn void os_rwlock_write_unlock(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  (void)pthread_rwlock_unlock(&prim->rwlock);
#endif
}

fn void os_rwlock_free(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  pthread_rwlock_destroy(&prim->rwlock);
  unx_primitive_free(prim);
#endif
}

fn OS_Handle os_cond_alloc(void) {
  UNX_Primitive *prim = unx_primitive_alloc(UNX_Primitive_CondVar);
  if (pthread_cond_init(&prim->condvar.cond, 0)) {
    unx_primitive_free(prim);
    prim = 0;
  } else {
    (void)pthread_mutex_init(&prim->condvar.mutex, 0);
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_cond_signal(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  (void)pthread_cond_signal(&prim->condvar.cond);
}

fn void os_cond_broadcast(OS_Handle handle) {
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  (void)pthread_cond_broadcast(&prim->condvar.cond);
}

fn bool os_cond_wait(OS_Handle cond_handle, OS_Handle mutex_handle,
                     u32 wait_at_most_microsec) {
  UNX_Primitive *cond_prim = (UNX_Primitive *)cond_handle.h[0];
  UNX_Primitive *mutex_prim = (UNX_Primitive *)mutex_handle.h[0];

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
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *cond_prim = (UNX_Primitive *)cond_handle.h[0];
  UNX_Primitive *rwlock_prim = (UNX_Primitive *)rwlock_handle.h[0];

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
#else
  return false;
#endif
}

fn bool os_cond_waitrw_write(OS_Handle cond_handle, OS_Handle rwlock_handle,
                             u32 wait_at_most_microsec) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *cond_prim = (UNX_Primitive *)cond_handle.h[0];
  UNX_Primitive *rwlock_prim = (UNX_Primitive *)rwlock_handle.h[0];

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
#else
  return false;
#endif
}

fn bool os_cond_free(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  i32 res = pthread_cond_destroy(&prim->condvar.cond) &
            pthread_mutex_destroy(&prim->condvar.mutex);
  unx_primitive_free(prim);
  return res == 0;
#else
  return false;
#endif
}

fn OS_Handle os_semaphore_alloc(OS_SemaphoreKind kind, u32 init_count,
                                u32 max_count, String8 name) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = unx_primitive_alloc(UNX_Primitive_Semaphore);
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
      AssertMsg(name.size > 0,
                "Semaphores sharable between processes must be named.");

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
#else
  OS_Handle res = {{0}};
  return res;
#endif
}

fn bool os_semaphore_signal(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  if (prim->semaphore.count + 1 >= prim->semaphore.max_count ||
      sem_post(prim->semaphore.sem)) {
    return false;
  }
  return ++prim->semaphore.count;
#else
  return false;
#endif
}

fn bool os_semaphore_wait(OS_Handle handle, u32 wait_at_most_microsec) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
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
#else
  return false;
#endif
}

fn bool os_semaphore_trywait(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  return sem_trywait(prim->semaphore.sem) == 0;
#else
  return false;
#endif
}

fn void os_semaphore_free(OS_Handle handle) {
#ifndef PLATFORM_CODERBOT
  UNX_Primitive *prim = (UNX_Primitive *)handle.h[0];
  switch (prim->semaphore.kind) {
    case OS_SemaphoreKind_Thread: {
      (void)sem_destroy(prim->semaphore.sem);
      os_release(prim->semaphore.sem, sizeof(sem_t));
    } break;
    case OS_SemaphoreKind_Process: {
      (void)sem_close(prim->semaphore.sem);
    } break;
  }
  unx_primitive_free(prim);
#endif
}

fn SharedMem os_sharedmem_open(String8 name, usize size, OS_AccessFlags flags) {
  SharedMem res = {};
  res.path = name;

  i32 access_flags = unx_flags_from_acf(flags);
  Scratch scratch = ScratchBegin(0, 0);
  res.file_handle.h[0] = shm_open(cstr_from_str8(scratch.arena, name), access_flags,
                                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  AssertMsg(res.file_handle.h[0] >= 0, "shm_open failed");
  ScratchEnd(scratch);

  Assert(!ftruncate((i32)res.file_handle.h[0], size));
  res.prop.size = size;
  res.content = (u8*)mmap(0, size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, (i32)res.file_handle.h[0], 0);
  Assert(res.content != MAP_FAILED);

  return res;
}

fn void os_sharedmem_resize(SharedMem *shm, usize size) {
  Assert(size);
  Assert(!ftruncate((i32)shm->file_handle.h[0], size));
  munmap(shm->content, shm->prop.size);
  shm->content = (u8*)mmap(0, size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, (i32)shm->file_handle.h[0], 0);
  AssertMsg(shm->content != MAP_FAILED, "sharedmem resize mmap failed");
  shm->prop.size = size;
}

fn void os_sharedmem_unlink_name(SharedMem *shm) {
  if (!shm->path.size) { return; }
  Scratch scratch = ScratchBegin(0, 0);
  shm_unlink(cstr_from_str8(scratch.arena, shm->path));
  ScratchEnd(scratch);
}

fn void os_sharedmem_close(SharedMem *shm) {
  os_sharedmem_unlink_name(shm);
  munmap(shm->content, shm->prop.size);
  close((i32)shm->file_handle.h[0]);
}

// =============================================================================
// Dynamic libraries
fn OS_Handle os_lib_open(String8 path) {
  OS_Handle result = {0};
#ifndef PLATFORM_CODERBOT
  Scratch scratch = ScratchBegin(0, 0);
  void *handle = dlopen(cstr_from_str8(scratch.arena, path), RTLD_NOW | RTLD_GLOBAL);
  AssertMsg(handle, dlerror());
  ScratchEnd(scratch);
#endif
  return result;
}

fn VoidFunc *os_lib_lookup(OS_Handle lib, String8 symbol) {
#ifndef PLATFORM_CODERBOT
  void *handle = (void*)lib.h[0];
  Scratch scratch = ScratchBegin(0, 0);
  VoidFunc *result = (VoidFunc*)(u64)dlsym(handle, cstr_from_str8(scratch.arena, symbol));
  /* AssertMsg(result, dlerror()); */
  ScratchEnd(scratch);
  return result;
#else
  return (VoidFunc*)0;
#endif
}

fn i32 os_lib_close(OS_Handle lib) {
#ifndef PLATFORM_CODERBOT
  void *handle = (void*)lib.h[0];
  return (handle ? dlclose(handle) : 0);
#else
  return 0;
#endif
}

// =============================================================================
// Misc
fn String8 os_currentDir(Arena *arena) {
  char *wd = getcwd(0, 0);
  isize size = str8_len(wd);
  u8 *copy = New(arena, u8, size);
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

    NetInterface *inter = New(arena, NetInterface);

    String8 interface = str8_from_cstr(ifa->ifa_name);
    inter->name.str = New(arena, u8, interface.size);
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
      inter->strip.str = New(arena, u8, strip.size);
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
      inter->strip.str = New(arena, u8, strip.size);
      inter->strip.size = strip.size;
      memcopy(inter->strip.str, strip.str, strip.size);
    }

    DLLPushBack(res.first, res.last, inter);
  }

  freeifaddrs(ifaddr);
  return res;
}

fn NetInterface os_net_interface_lookup(String8 interface) {
  NetInterface res = {};

  Scratch scratch = ScratchBegin(0, 0);
  NetInterfaceList inters = os_net_interfaces(scratch.arena);
  for (NetInterface *curr = inters.first; curr; curr = curr->next) {
    if (str8_eq(curr->strip, interface) || str8_eq(curr->name, interface)) {
      memcopy(&res, curr, sizeof(NetInterface));
      break;
    }
  }
  ScratchEnd(scratch);

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
  UNX_Primitive *prim = unx_primitive_alloc(UNX_Primitive_Socket);
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
      Panic("Invalid server address.");
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
      Panic("Invalid transport protocol.");
    }
  }

  prim->socket.fd = socket(cdomain, ctype, 0);
  if (prim->socket.fd == -1) {
    unx_primitive_free(prim);
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
  UNX_Primitive *prim = (UNX_Primitive*)socket->handle.h[0];

  i32 optval = 1;
  (void)setsockopt(prim->socket.fd, SOL_SOCKET,
                   SO_REUSEPORT | SO_REUSEADDR, &optval,
                   sizeof(optval));
  (void)bind(prim->socket.fd, &prim->socket.addr, prim->socket.size);
  (void)listen(prim->socket.fd, max_backlog);
}

fn OS_Socket os_socket_accept(OS_Socket *socket) {
  OS_Socket res = {0};

  UNX_Primitive *server_prim = (UNX_Primitive*)socket->handle.h[0];
  UNX_Primitive *client_prim = unx_primitive_alloc(UNX_Primitive_Socket);
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

fn u8* os_socket_recv(Arena *arena, OS_Socket *client, usize buffer_size) {
  u8 *res = New(arena, u8, buffer_size);
  UNX_Primitive *prim = (UNX_Primitive*)client->handle.h[0];
  (void)read(prim->socket.fd, res, buffer_size);
  return res;
}

fn void os_socket_connect(OS_Socket *server) {
  UNX_Primitive *prim = (UNX_Primitive*)server->handle.h[0];
  if (connect(prim->socket.fd, &prim->socket.addr,
              prim->socket.size) == -1) {
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

fn void os_socket_send_str8(OS_Socket *socket, String8 msg) {
  UNX_Primitive *prim = (UNX_Primitive*)socket->handle.h[0];
  Scratch scratch = ScratchBegin(0, 0);
  send(prim->socket.fd, cstr_from_str8(scratch.arena, msg), msg.size, 0);
  ScratchEnd(scratch);
}

fn void os_socket_close(OS_Socket *socket) {
  UNX_Primitive *prim = (UNX_Primitive*)socket->handle.h[0];
  shutdown(prim->socket.fd, SHUT_RDWR);
  close(prim->socket.fd);
  unx_primitive_free(prim);
}

// =============================================================================
// File reading and writing/appending
fn String8 fs_read_virtual(Arena *arena, OS_Handle file, usize size) {
  int fd = (i32)file.h[0];
  String8 result = {};
  if(!fd) { return result; }

  u8 *buffer = New(arena, u8, size);
  if(pread(fd, buffer, size, 0) >= 0) {
    result.str = buffer;
    result.size = str8_len((char *)buffer);
  }

  return result;
}

fn String8 fs_read(Arena *arena, OS_Handle file) {
  int fd = (i32)file.h[0];
  String8 result = {};
  if(!fd) { return result; }

  struct stat file_stat;
  if (!fstat(fd, &file_stat)) {
    u8 *buffer = New(arena, u8, file_stat.st_size);
    if(pread(fd, buffer, file_stat.st_size, 0) >= 0) {
      result.str = buffer;
      result.size = file_stat.st_size;
    }
  }

  return result;
}

inline fn bool fs_write(OS_Handle file, String8 content) {
  if(!file.h[0]) { return false; }
  return write((i32)file.h[0], content.str, content.size) == (isize)content.size;
}

fn FS_Properties fs_get_properties(OS_Handle file) {
  FS_Properties result = {0};
  if(!file.h[0]) { return result; }

  struct stat file_stat;
  if (fstat((i32)file.h[0], &file_stat) == 0) {
    result = unx_properties_from_stat(&file_stat);
  }
  return result;
}

fn String8 fs_readlink(Arena *arena, String8 path) {
  String8 res = {};
  res.str = New(arena, u8, PATH_MAX);
  res.size = readlink((char *)path.str, (char *)res.str, PATH_MAX);
  if (res.size <= 0) {
    res.str = 0;
    res.size = 0;
  }

  arena_pop(arena, PATH_MAX - res.size);
  return res;
}

// =============================================================================
// Memory mapping files for easier and faster handling
fn File fs_fopen(Arena *arena, OS_Handle fd) {
  File file = {};
  i32 flags = fcntl((i32)fd.h[0], F_GETFL);
  if (flags < 0) { return file; }
  flags &= O_ACCMODE;
  if (!flags) {
    flags = PROT_READ;
  } else {
    flags = PROT_READ | PROT_WRITE;
  }

  file.file_handle = fd;
  file.path = fs_path_from_handle(arena, fd);
  file.prop = fs_get_properties(file.file_handle);
  file.content = (u8 *)mmap(0, ClampBot(file.prop.size, 1),
                            flags, MAP_SHARED, (i32)fd.h[0], 0);
  file.mmap_handle.h[0] = (u64)file.content;

  return file;
}

fn File fs_fopen_tmp(Arena *arena) {
  char path[] = "/tmp/base-XXXXXX";
  i32 fd = mkstemp(path);

  String8 pathstr = str8(New(arena, u8, Arrsize(path)), Arrsize(path));
  memcopy(pathstr.str, path, Arrsize(path));

  File file = {};
  file.file_handle.h[0] = fd;
  file.path = pathstr;
  file.prop = fs_get_properties(file.file_handle);
  file.content = (u8*)mmap(0, ClampBot(file.prop.size, 1),
                           PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  file.mmap_handle.h[0] = (u64)file.content;
  return file;
}

inline fn bool fs_fclose(File *file) {
  return !munmap((void *)file->mmap_handle.h[0], file->prop.size) &&
         fs_close(file->file_handle);
}

inline fn bool fs_fresize(File *file, usize size) {
  if (ftruncate((i32)file->file_handle.h[0], size) < 0) {
    return false;
  }

  file->prop.size = size;
  (void)munmap(file->content, file->prop.size);
  file->content = (u8*)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                            (i32)file->file_handle.h[0], 0);
  return (isize)file->content > 0;
}

inline fn bool fs_has_changed(File *file) {
  FS_Properties prop = fs_get_properties(file->file_handle);
  return (file->prop.last_access_time != prop.last_access_time) ||
         (file->prop.last_modification_time != prop.last_modification_time) ||
         (file->prop.last_status_change_time != prop.last_status_change_time);
}

inline fn bool fs_fdelete(File *file) {
  return unlink((char *) file->path.str) >= 0 && fs_fclose(file);
}

inline fn bool fs_frename(File *file, String8 to) {
  return rename((char *) file->path.str, (char *) to.str) >= 0;
}

// =============================================================================
// Misc operation on the filesystem
inline fn bool fs_delete(String8 filepath) {
  Assert(filepath.size != 0);
  Scratch scratch = ScratchBegin(0, 0);
  i32 res = unlink(cstr_from_str8(scratch.arena, filepath));
  ScratchEnd(scratch);
  return res >= 0;
}

inline fn bool fs_rename(String8 filepath, String8 to) {
  Assert(filepath.size != 0 && to.size != 0);
  return rename((char *)filepath.str, (char *)to.str) >= 0;
}

inline fn bool fs_mkdir(String8 path) {
  Assert(path.size != 0);
  return mkdir((char *)path.str,
               S_IRWXU | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)) >= 0;
}

inline fn bool fs_rmdir(String8 path) {
  Assert(path.size != 0);
  return rmdir((char *)path.str) >= 0;
}

fn String8 fs_filename_from_path(Arena *arena, String8 path) {
  Scratch scratch = ScratchBegin(&arena, 1);
  char *fullname = basename(cstr_from_str8(scratch.arena, path));
  ScratchEnd(scratch);

  usize last_dot = 0, size = 0;
  for (; fullname[size]; ++size) {
    if (fullname[size] == '.') { last_dot = size; }
  }
  return str8((u8*)fullname, size - (size - last_dot));
}

fn OS_FileIter* fs_iter_begin(Arena *arena, String8 path) {
  OS_FileIter *os_iter = New(arena, OS_FileIter);
  UNX_FileIter *iter = (UNX_FileIter *)os_iter->memory;
  iter->path = path;
  Scratch scratch = ScratchBegin(&arena, 1);
  iter->dir = opendir(cstr_from_str8(scratch.arena, path));
  ScratchEnd(scratch);

  return os_iter;
}

fn OS_FileIter* fs_iter_begin_filtered(Arena *arena, String8 path, OS_FileType allowed) {
  OS_FileIter *os_iter = New(arena, OS_FileIter);
  UNX_FileIter *iter = (UNX_FileIter *)os_iter->memory;
  iter->path = path;
  Scratch scratch = ScratchBegin(&arena, 1);
  iter->dir = opendir(cstr_from_str8(scratch.arena, path));
  ScratchEnd(scratch);
  os_iter->filter_allowed = allowed;

  return os_iter;
}

fn bool fs_iter_next(Arena *arena, OS_FileIter *os_iter, OS_FileInfo *info_out) {
  local const String8 currdir = StrlitInit(".");
  local const String8 parentdir = StrlitInit("..");

  String8 str = {};
  UNX_FileIter *iter = (UNX_FileIter *)os_iter->memory;
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

    info_out->properties = unx_properties_from_stat(&file_stat);
  } while (os_iter->filter_allowed && !(info_out->properties.type & os_iter->filter_allowed));

  info_out->name.size = str.size;
  info_out->name.str = New(arena, u8, str.size);
  memcopy(info_out->name.str, str.str, str.size);
  ScratchEnd(scratch);
  return true;
}

fn void fs_iter_end(OS_FileIter *os_iter) {
  UNX_FileIter *iter = (UNX_FileIter *)os_iter->memory;
  if (iter->dir) { closedir(iter->dir); }
}
