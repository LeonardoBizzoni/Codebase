#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>

#include <arpa/inet.h>
#include <ifaddrs.h>

global LNX_State lnx_state = {0};

fn LNX_Primitive* lnx_primitiveAlloc(LNX_PrimitiveType type) {
  pthread_mutex_lock(&lnx_state.primitive_lock);
  LNX_Primitive *res = lnx_state.primitive_freelist;
  if (res) {
    StackPop(lnx_state.primitive_freelist);
  } else {
    res = New(lnx_state.arena, LNX_Primitive);
  }
  pthread_mutex_unlock(&lnx_state.primitive_lock);

  res->type = type;
  return res;
}

fn void lnx_primitiveFree(LNX_Primitive *ptr) {
  pthread_mutex_lock(&lnx_state.primitive_lock);
  StackPush(lnx_state.primitive_freelist, ptr);
  pthread_mutex_unlock(&lnx_state.primitive_lock);
}

fn void* lnx_threadEntry(void *args) {
  LNX_Primitive *wrap_args = (LNX_Primitive *)args;
  ThreadFunc *func = (ThreadFunc *)wrap_args->thread.func;
  func(wrap_args->thread.args);
  return 0;
}

fn FS_Properties lnx_propertiesFromStat(struct stat *stat) {
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

fn String8 lnx_gethostname(void) {
  char name[HOST_NAME_MAX];
  (void)gethostname(name, HOST_NAME_MAX);

  String8 namestr = str8((u8 *)name, str8_len(name));
  return namestr;
}

fn void lnx_parseMeminfo(void) {
  OS_Handle meminfo = fs_open(Strlit("/proc/meminfo"), OS_acfRead);
  StringStream lines = str8_split(lnx_state.arena, fs_readVirtual(lnx_state.arena,
                                                                  meminfo, 4096), '\n');
  for (StringNode *curr_line = lines.first; curr_line; curr_line = curr_line->next) {
    StringStream ss = str8_split(lnx_state.arena, curr_line->value, ':');
    for (StringNode *curr = ss.first; curr; curr = curr->next) {
      if (str8_eq(curr->value, Strlit("MemTotal"))) {
        curr = curr->next;
        lnx_state.info.total_memory = KiB(1) *
                                      u64_from_str8(str8_split(lnx_state.arena, str8_trim(curr->value),
                                                          ' ').first->value);
      } else if (str8_eq(curr->value, Strlit("Hugepagesize"))) {
        curr = curr->next;
        lnx_state.info.hugepage_size = KiB(1) *
                                       u64_from_str8(str8_split(lnx_state.arena, str8_trim(curr->value),
                                                           ' ').first->value);
        return;
      }
    }
  }
}

// =============================================================================
// System information retrieval
fn OS_SystemInfo *os_getSystemInfo(void) {
  return &lnx_state.info;
}

// =============================================================================
// DateTime
fn time64 os_local_now(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64_from_unix(tms.tv_sec + lnx_state.unix_utc_offset);
  res |= (u64)(tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_local_dateTimeNow(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = datetime_from_unix(tms.tv_sec + lnx_state.unix_utc_offset);
  res.ms = tms.tv_nsec / 1e6;
  return res;
}

fn time64 os_local_fromUTCTime64(time64 t) {
  u64 utc_time = unix_from_time64(t);
  time64 res = time64_from_unix(utc_time + lnx_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_local_fromUTCDateTime(DateTime *dt) {
  u64 utc_time = unix_from_datetime(dt);
  DateTime res = datetime_from_unix(utc_time + lnx_state.unix_utc_offset);
  res.ms = dt->ms;
  return res;
}

fn time64 os_utc_now(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64_from_unix(tms.tv_sec);
  res |= (u64)(tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_utc_dateTimeNow(void) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = datetime_from_unix(tms.tv_sec);
  res.ms = tms.tv_nsec / 1e6;
  return res;
}

fn time64 os_utc_localizedTime64(i8 utc_offset) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64_from_unix(tms.tv_sec + utc_offset * UNIX_HOUR);
  res |= (u64)(tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_utc_localizedDateTime(i8 utc_offset) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = datetime_from_unix(tms.tv_sec + utc_offset * UNIX_HOUR);
  res.ms = tms.tv_nsec / 1e6;
  return res;
}

fn time64 os_utc_fromLocalTime64(time64 t) {
  u64 local_time = unix_from_time64(t);
  time64 res = time64_from_unix(local_time - lnx_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_utc_fromLocalDateTime(DateTime *dt) {
  u64 local_time = unix_from_datetime(dt);
  DateTime res = datetime_from_unix(local_time - lnx_state.unix_utc_offset);
  res.ms = dt->ms;
  return res;
}

fn void os_sleep_milliseconds(u32 ms) {
  usleep(ms * 1e3);
}

fn OS_Handle os_timer_start(void) {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Timer);
  if (clock_gettime(CLOCK_MONOTONIC, &prim->timer) != 0) {
    lnx_primitiveFree(prim);
    prim = 0;
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn bool os_timer_elapsed_time(OS_TimerGranularity unit, OS_Handle timer, u64 how_much) {
  OS_Handle now = os_timer_start();
  return os_timer_elapsed_start2end(unit, timer, now) >= how_much;
}

fn u64 os_timer_elapsed_start2end(OS_TimerGranularity unit, OS_Handle start, OS_Handle end) {
  struct timespec tstart = ((LNX_Primitive *)start.h[0])->timer;
  struct timespec tend = ((LNX_Primitive *)end.h[0])->timer;
  u64 diff_nanos = (tend.tv_sec - tstart.tv_sec) * 1e9 +
                   (tend.tv_nsec - tstart.tv_nsec);

  lnx_primitiveFree((LNX_Primitive *)start.h[0]);
  lnx_primitiveFree((LNX_Primitive *)end.h[0]);

  u64 res = 0;
  switch (unit) {
    case OS_TimerGranularity_min: {
      res = (u64)((diff_nanos / 1e9) / 60);
    } break;
    case OS_TimerGranularity_sec: {
      res = (u64)(diff_nanos / 1e9);
    } break;
    case OS_TimerGranularity_ms: {
      res = (u64)(diff_nanos / 1e6);
    } break;
    case OS_TimerGranularity_ns: {
      res = diff_nanos;
    } break;
  }
  return res;
}

// =============================================================================
// Memory allocation
fn void* os_reserve(usize base_addr, usize size) {
  void *res = mmap((void *)base_addr, size, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (res == MAP_FAILED) {
    res = 0;
  }

  return res;
}

fn void* os_reserveHuge(usize base_addr, usize size) {
  // TODO(lb): MAP_HUGETLB vs MAP_HUGE_2MB/MAP_HUGE_1GB?
  void *res = mmap((void *)base_addr, size, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
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

  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Thread);
  prim->thread.func = thread_main;
  prim->thread.args = args;

  if (pthread_create(&prim->thread.handle, 0, lnx_threadEntry, prim) != 0) {
    lnx_primitiveFree(prim);
    prim = 0;
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_thread_kill(OS_Handle thd_handle) {
  LNX_Primitive *prim = (LNX_Primitive *)thd_handle.h[0];
  if (!prim || prim->type != LNX_Primitive_Thread) { return; }
  (void)pthread_kill(prim->thread.handle, 0);
  lnx_primitiveFree(prim);
}

fn bool os_thread_join(OS_Handle thd_handle) {
  LNX_Primitive *prim = (LNX_Primitive *)thd_handle.h[0];
  if (!prim || prim->type != LNX_Primitive_Thread) { return false; }
  bool res = pthread_join(prim->thread.handle, 0) == 0;
  lnx_primitiveFree(prim);
  return res;
}

fn OS_ProcHandle os_proc_spawn(void) {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Process);
  prim->proc = fork();

  OS_ProcHandle res = {prim->proc == 0, {{(u64)prim}}};
  return res;
}

fn void os_proc_kill(OS_ProcHandle proc) {
  Assert(!proc.is_child);
  LNX_Primitive *prim = (LNX_Primitive *)proc.handle.h[0];
  (void)kill(prim->proc, SIGKILL);
  lnx_primitiveFree(prim);
}

fn OS_ProcStatus os_proc_wait(OS_ProcHandle proc) {
  Assert(!proc.is_child);
  LNX_Primitive *prim = (LNX_Primitive *)proc.handle.h[0];
  i32 child_res;
  (void)waitpid(prim->proc, &child_res, 0);
  lnx_primitiveFree(prim);

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
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Mutex);
  pthread_mutexattr_t attr;
  if (pthread_mutexattr_init(&attr) != 0 ||
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
    lnx_primitiveFree(prim);
    prim = 0;
  } else {
    (void)pthread_mutex_init(&prim->mutex, &attr);
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_mutex_lock(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_mutex_lock(&prim->mutex);
}

fn bool os_mutex_trylock(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  return pthread_mutex_trylock(&prim->mutex) == 0;
}

fn void os_mutex_unlock(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_mutex_unlock(&prim->mutex);
}

fn void os_mutex_free(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  pthread_mutex_destroy(&prim->mutex);
  lnx_primitiveFree(prim);
}

fn OS_Handle os_rwlock_alloc(void) {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Rwlock);
  pthread_rwlock_init(&prim->rwlock, 0);

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_rwlock_read_lock(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_rwlock_rdlock(&prim->rwlock);
}

fn bool os_rwlock_read_trylock(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  return pthread_rwlock_tryrdlock(&prim->rwlock) == 0;
}

fn void os_rwlock_read_unlock(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_rwlock_unlock(&prim->rwlock);
}

fn void os_rwlock_write_lock(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_rwlock_wrlock(&prim->rwlock);
}

fn bool os_rwlock_write_trylock(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  return pthread_rwlock_trywrlock(&prim->rwlock) == 0;
}

fn void os_rwlock_write_unlock(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_rwlock_unlock(&prim->rwlock);
}

fn void os_rwlock_free(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  pthread_rwlock_destroy(&prim->rwlock);
  lnx_primitiveFree(prim);
}

fn OS_Handle os_cond_alloc(void) {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_CondVar);
  if (pthread_cond_init(&prim->condvar.cond, 0)) {
    lnx_primitiveFree(prim);
    prim = 0;
  }

  OS_Handle res = {{(u64)prim}};
  return res;
}

fn void os_cond_signal(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_cond_signal(&prim->condvar.cond);
}

fn void os_cond_broadcast(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_cond_broadcast(&prim->condvar.cond);
}

fn bool os_cond_wait(OS_Handle cond_handle, OS_Handle mutex_handle,
                     u32 wait_at_most_microsec) {
  LNX_Primitive *cond_prim = (LNX_Primitive *)cond_handle.h[0];
  LNX_Primitive *mutex_prim = (LNX_Primitive *)mutex_handle.h[0];

  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += wait_at_most_microsec/1e6;
    abstime.tv_nsec += 1e3 * (wait_at_most_microsec % (u32)1e6);
    if (abstime.tv_nsec >= 1e9) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= 1e9;
    }

    return pthread_cond_timedwait(&cond_prim->condvar.cond, &mutex_prim->mutex, &abstime) == 0;
  } else {
    (void)pthread_cond_wait(&cond_prim->condvar.cond, &mutex_prim->mutex);
    return true;
  }
}

fn bool os_cond_waitrw_read(OS_Handle cond_handle, OS_Handle rwlock_handle,
                            u32 wait_at_most_microsec) {
  LNX_Primitive *cond_prim = (LNX_Primitive *)cond_handle.h[0];
  LNX_Primitive *rwlock_prim = (LNX_Primitive *)rwlock_handle.h[0];

  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += wait_at_most_microsec/1e6;
    abstime.tv_nsec += 1e3 * (wait_at_most_microsec % (u32)1e6);
    if (abstime.tv_nsec >= 1e9) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= 1e9;
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
  LNX_Primitive *cond_prim = (LNX_Primitive *)cond_handle.h[0];
  LNX_Primitive *rwlock_prim = (LNX_Primitive *)rwlock_handle.h[0];

  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += wait_at_most_microsec/1e6;
    abstime.tv_nsec += 1e3 * (wait_at_most_microsec % (u32)1e6);
    if (abstime.tv_nsec >= 1e9) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= 1e9;
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
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  i32 res = pthread_cond_destroy(&prim->condvar.cond);
  lnx_primitiveFree(prim);
  return res == 0;
}

fn OS_Handle os_semaphore_alloc(OS_SemaphoreKind kind, u32 init_count,
                                u32 max_count, String8 name) {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Semaphore);
  prim->semaphore.kind = kind;
  prim->semaphore.max_count = max_count;
  switch (kind) {
    case OS_SemaphoreKind_Thread: {
      prim->semaphore.sem = (sem_t *)os_reserve(0, sizeof(sem_t));
      os_commit(prim->semaphore.sem, sizeof(sem_t));
      if (sem_init(prim->semaphore.sem, 0, init_count)) {
        os_release(prim->semaphore.sem, sizeof(sem_t));
        prim->semaphore.sem = 0;
      }
    } break;
    case OS_SemaphoreKind_Process: {
      AssertMsg(name.size > 0,
                Strlit("Semaphores sharable between processes must be named."));

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
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  if (prim->semaphore.count + 1 >= prim->semaphore.max_count ||
      sem_post(prim->semaphore.sem)) {
    return false;
  }
  return ++prim->semaphore.count;
}

fn bool os_semaphore_wait(OS_Handle handle, u32 wait_at_most_microsec) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += wait_at_most_microsec/1e6;
    abstime.tv_nsec += 1e3 * (wait_at_most_microsec % (u32)1e6);
    if (abstime.tv_nsec >= 1e9) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= 1e9;
    }

    return sem_timedwait(prim->semaphore.sem, &abstime) == 0;
  } else {
    return sem_wait(prim->semaphore.sem) == 0;
  }
}

fn bool os_semaphore_trywait(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  return sem_trywait(prim->semaphore.sem) == 0;
}

fn void os_semaphore_free(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  switch (prim->semaphore.kind) {
    case OS_SemaphoreKind_Thread: {
      (void)sem_destroy(prim->semaphore.sem);
      os_release(prim->semaphore.sem, sizeof(sem_t));
    } break;
    case OS_SemaphoreKind_Process: {
      (void)sem_close(prim->semaphore.sem);
    } break;
  }
  lnx_primitiveFree(prim);
}

fn SharedMem os_sharedmem_open(String8 name, usize size, OS_AccessFlags flags) {
  SharedMem res = {};
  res.path = name;

  i32 access_flags = 0;
  if((flags & OS_acfRead) && (flags & OS_acfWrite)) {
    access_flags |= O_RDWR;
  } else if(flags & OS_acfRead) {
    access_flags |= O_RDONLY;
  } else if(flags & OS_acfWrite) {
    access_flags |= O_WRONLY | O_CREAT | O_TRUNC;
  }
  if(flags & OS_acfAppend) { access_flags |= O_APPEND | O_CREAT; }

  Scratch scratch = ScratchBegin(0, 0);
  res.file_handle.h[0] = shm_open(cstr_from_str8(scratch.arena, name),
                                  access_flags,
                                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  ScratchEnd(scratch);

  (void)ftruncate(res.file_handle.h[0], size);
  res.prop = fs_getProp(res.file_handle);
  res.content = (u8*)mmap(0, ClampBot(res.prop.size, 1), PROT_READ | PROT_WRITE,
                           MAP_SHARED, res.file_handle.h[0], 0);

  return res;
}

fn bool os_sharedmem_close(SharedMem *shm) {
  Scratch scratch = ScratchBegin(0, 0);
  bool res = munmap(shm->content, shm->prop.size) == 0 &&
             shm_unlink(cstr_from_str8(scratch.arena, shm->path)) == 0;
  ScratchEnd(scratch);
  return res;
}

// =============================================================================
// Dynamic libraries
fn OS_Handle os_lib_open(String8 path) {
  OS_Handle result = {0};
  Scratch scratch = ScratchBegin(0, 0);
  void *handle = dlopen(cstr_from_str8(scratch.arena, path),
                        RTLD_LAZY);
  if (handle) { result.h[0] = (u64)handle; }
  ScratchEnd(scratch);
  return result;
}

fn VoidFunc *os_lib_lookup(OS_Handle lib, String8 symbol) {
  Scratch scratch = ScratchBegin(0, 0);
  void *handle = (void*)lib.h[0];
  char *symbol_cstr = cstr_from_str8(scratch.arena, symbol);
  VoidFunc *result = (VoidFunc*)(u64)dlsym(handle, symbol_cstr);
  ScratchEnd(scratch);
  return result;
}

fn i32 os_lib_close(OS_Handle lib) {
  void *handle = (void*)lib.h[0];
  return dlclose(handle);
}

// =============================================================================
// Misc
fn String8 os_currentDir(Arena *arena) {
  char *wd = getcwd(0, 0);
  isize size = str8_len(wd);
  u8 *copy = New(arena, u8, size);
  memCopy(copy, wd, size);
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
    memCopy(inter->name.str, interface.str, interface.size);

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
      memCopy(inter->strip.str, strip.str, strip.size);
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
      memCopy(inter->strip.str, strip.str, strip.size);
    }

    DLLPushBack(res.first, res.last, inter);
  }

  freeifaddrs(ifaddr);
  return res;
}

fn NetInterface os_net_interface_from_str8(String8 strip) {
  NetInterface res = {};

  Scratch scratch = ScratchBegin(0, 0);
  NetInterfaceList inters = os_net_interfaces(scratch.arena);
  for (NetInterface *curr = inters.first; curr; curr = curr->next) {
    if (str8_eq(curr->strip, strip)) {
      memCopy(&res, curr, sizeof(NetInterface));
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
    memCopy(res.v4.bytes,
            &((struct sockaddr_in*)info->ai_addr)->sin_addr,
            4 * sizeof(u8));
  } else if (info->ai_family == AF_INET6) {
    res.version = OS_Net_Network_IPv6;
    memCopy(res.v6.words,
            &((struct sockaddr_in6*)info->ai_addr)->sin6_addr,
            8 * sizeof(u16));
  }

  freeaddrinfo(info);
  return res;
}

fn OS_Socket os_socket_open(String8 name, u16 port,
                                OS_Net_Transport protocol) {
  OS_Socket res = {0};
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Socket);
  IP server = os_net_ip_from_str8(name, 0);
  i32 ctype, cdomain;
  switch (server.version) {
    case OS_Net_Network_IPv4: {
      cdomain = AF_INET;
      struct sockaddr_in *addr = (struct sockaddr_in*)&prim->socket.addr;
      addr->sin_family = cdomain;
      addr->sin_port = htons(port);
      memCopy(&addr->sin_addr, server.v4.bytes, 4 * sizeof(u8));
      prim->socket.size = sizeof(struct sockaddr_in);
    } break;
    case OS_Net_Network_IPv6: {
      cdomain = AF_INET6;
      struct sockaddr_in6 *addr = (struct sockaddr_in6*)&prim->socket.addr;
      addr->sin6_family = cdomain;
      addr->sin6_port = htons(port);
      memCopy(&addr->sin6_addr, server.v4.bytes, 8 * sizeof(u16));
      prim->socket.size = sizeof(struct sockaddr_in6);
    } break;
    default: {
      AssertMsg(false, Strlit("Invalid server address."));
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
      AssertMsg(false, Strlit("Invalid transport protocol."));
    }
  }

  prim->socket.fd = socket(cdomain, ctype, 0);
  if (prim->socket.fd == -1) {
    lnx_primitiveFree(prim);
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
  LNX_Primitive *prim = (LNX_Primitive*)socket->handle.h[0];

  i32 optval = 1;
  (void)setsockopt(prim->socket.fd, SOL_SOCKET,
                   SO_REUSEPORT | SO_REUSEADDR, &optval,
                   sizeof(optval));
  (void)bind(prim->socket.fd, &prim->socket.addr, prim->socket.size);
  (void)listen(prim->socket.fd, max_backlog);
}

fn OS_Socket os_socket_accept(OS_Socket *socket) {
  OS_Socket res = {0};

  LNX_Primitive *server_prim = (LNX_Primitive*)socket->handle.h[0];
  LNX_Primitive *client_prim = lnx_primitiveAlloc(LNX_Primitive_Socket);
  client_prim->socket.size = sizeof(client_prim->socket.addr);
  client_prim->socket.fd = accept(server_prim->socket.fd,
                                  &client_prim->socket.addr,
                                  &client_prim->socket.size);
  if (client_prim->socket.fd == -1) {
    perror("accept");
    return res;
  }

  memCopy(&res, socket, sizeof(OS_Socket));
  res.handle.h[0] = (u64)client_prim;
  switch (client_prim->socket.addr.sa_family) {
  case AF_INET: {
    struct sockaddr_in *client = (struct sockaddr_in *)&client_prim->socket.addr;
    memCopy(res.client.addr.v4.bytes, &client->sin_addr, 4 * sizeof(u8));
    res.client.port = client->sin_port;
    res.client.addr.version = OS_Net_Network_IPv4;
  } break;
  case AF_INET6: {
    struct sockaddr_in6 *client = (struct sockaddr_in6 *)&client_prim->socket.addr;
    memCopy(res.client.addr.v6.words, &client->sin6_addr, 8 * sizeof(u16));
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
  LNX_Primitive *prim = (LNX_Primitive*)client->handle.h[0];
  read(prim->socket.fd, res, buffer_size);
  return res;
}

fn void os_socket_connect(OS_Socket *server) {
  LNX_Primitive *prim = (LNX_Primitive*)server->handle.h[0];
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
    memCopy(server->client.addr.v4.bytes, &clientv4->sin_addr, 4 * sizeof(u8));
    server->client.port = clientv4->sin_port;
    server->client.addr.version = OS_Net_Network_IPv4;
  } break;
  case AF_INET6: {
    struct sockaddr_in6 *clientv6 = (struct sockaddr_in6 *)&client;
    memCopy(server->client.addr.v6.words, &clientv6->sin6_addr, 8 * sizeof(u16));
    server->client.port = clientv6->sin6_port;
    server->client.addr.version = OS_Net_Network_IPv6;
  } break;
  }
}

fn void os_socket_send_str8(OS_Socket *socket, String8 msg) {
  LNX_Primitive *prim = (LNX_Primitive*)socket->handle.h[0];
  Scratch scratch = ScratchBegin(0, 0);
  send(prim->socket.fd, cstr_from_str8(scratch.arena, msg), msg.size, 0);
  ScratchEnd(scratch);
}

fn void os_socket_close(OS_Socket *socket) {
  LNX_Primitive *prim = (LNX_Primitive*)socket->handle.h[0];
  shutdown(prim->socket.fd, SHUT_RDWR);
  close(prim->socket.fd);
  lnx_primitiveFree(prim);
}

// =============================================================================
// File reading and writing/appending
fn OS_Handle fs_open(String8 filepath, OS_AccessFlags flags) {
  i32 access_flags = O_CREAT;

  if((flags & OS_acfRead) && (flags & OS_acfWrite)) {
    access_flags |= O_RDWR;
  } else if(flags & OS_acfRead) {
    access_flags |= O_RDONLY;
  } else if(flags & OS_acfWrite) {
    access_flags |= O_WRONLY | O_CREAT | O_TRUNC;
  }
  if(flags & OS_acfAppend) { access_flags |= O_APPEND; }

  Scratch scratch = ScratchBegin(0, 0);
  i32 fd = open(cstr_from_str8(scratch.arena, filepath), access_flags,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  ScratchEnd(scratch);
  if(fd < 0) {
    fd = 0;
  }

  OS_Handle res = {{(u64)fd}};
  return res;
}

fn bool fs_close(OS_Handle fd) {
  return close(fd.h[0]) == 0;
}

fn String8 fs_readVirtual(Arena *arena, OS_Handle file, usize size) {
  int fd = file.h[0];
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
  int fd = file.h[0];
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
  return write(file.h[0], content.str, content.size) == (isize)content.size;
}

fn FS_Properties fs_getProp(OS_Handle file) {
  FS_Properties result = {0};
  if(!file.h[0]) { return result; }

  struct stat file_stat;
  if (fstat((i32)file.h[0], &file_stat) == 0) {
    result = lnx_propertiesFromStat(&file_stat);
  }
  return result;
}

fn String8 fs_pathFromHandle(Arena *arena, OS_Handle fd) {
  char path[PATH_MAX];
  isize len = snprintf(path, sizeof(path), "/proc/self/fd/%ld", fd.h[0]);
  return fs_readlink(arena, str8((u8 *)path, len));
}

fn String8 fs_readlink(Arena *arena, String8 path) {
  String8 res = {};
  res.str = New(arena, u8, PATH_MAX);
  res.size = readlink((char *)path.str, (char *)res.str, PATH_MAX);
  if (res.size <= 0) {
    res.str = 0;
    res.size = 0;
  }

  arenaPop(arena, PATH_MAX - res.size);
  return res;
}

// =============================================================================
// Memory mapping files for easier and faster handling
fn File fs_fopen(Arena *arena, OS_Handle fd) {
  File file = {};
  i32 flags = fcntl(fd.h[0], F_GETFL);
  if (flags < 0) { return file; }
  flags &= O_ACCMODE;
  if (!flags) {
    flags = PROT_READ;
  } else {
    flags = PROT_READ | PROT_WRITE;
  }

  file.file_handle = fd;
  file.path = fs_pathFromHandle(arena, fd);
  file.prop = fs_getProp(file.file_handle);
  file.content = (u8 *)mmap(0, ClampBot(file.prop.size, 1),
                            flags, MAP_SHARED, fd.h[0], 0);
  file.mmap_handle.h[0] = (u64)file.content;

  return file;
}

fn File fs_fopenTmp(Arena *arena) {
  char path[] = "/tmp/base-XXXXXX";
  i32 fd = mkstemp(path);

  String8 pathstr = str8(New(arena, u8, Arrsize(path)), Arrsize(path));
  memCopy(pathstr.str, path, Arrsize(path));

  File file = {};
  file.file_handle.h[0] = fd;
  file.path = pathstr;
  file.prop = fs_getProp(file.file_handle);
  file.content = (u8*)mmap(0, ClampBot(file.prop.size, 1),
                           PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  file.mmap_handle.h[0] = (u64)file.content;
  return file;
}

inline fn bool fs_fclose(File *file) {
  return munmap((void *)file->mmap_handle.h[0], file->prop.size) == 0 &&
         close(file->file_handle.h[0]) >= 0;
}

inline fn bool fs_fresize(File *file, usize size) {
  if (ftruncate(file->file_handle.h[0], size) < 0) {
    return false;
  }

  file->prop.size = size;
  (void)munmap(file->content, file->prop.size);
  file->content = (u8*)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                            file->file_handle.h[0], 0);
  return (isize)file->content > 0;
}

inline fn bool fs_fileHasChanged(File *file) {
  FS_Properties prop = fs_getProp(file->file_handle);
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
  return unlink((char *)filepath.str) >= 0;
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

fn OS_FileIter* fs_iter_begin(Arena *arena, String8 path) {
  OS_FileIter *os_iter = New(arena, OS_FileIter);
  LNX_FileIter *iter = (LNX_FileIter *)os_iter->memory;
  iter->path = path;
  Scratch scratch = ScratchBegin(&arena, 1);
  iter->dir = opendir(cstr_from_str8(scratch.arena, path));
  ScratchEnd(scratch);

  return os_iter;
}

fn OS_FileIter* fs_iter_beginFiltered(Arena *arena, String8 path, OS_FileType allowed) {
  OS_FileIter *os_iter = New(arena, OS_FileIter);
  LNX_FileIter *iter = (LNX_FileIter *)os_iter->memory;
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
  LNX_FileIter *iter = (LNX_FileIter *)os_iter->memory;
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

    info_out->properties = lnx_propertiesFromStat(&file_stat);
  } while (os_iter->filter_allowed && !(info_out->properties.type & os_iter->filter_allowed));

  info_out->name.size = str.size;
  info_out->name.str = New(arena, u8, str.size);
  memCopy(info_out->name.str, str.str, str.size);
  ScratchEnd(scratch);
  return true;
}

fn void fs_iter_end(OS_FileIter *os_iter) {
  LNX_FileIter *iter = (LNX_FileIter *)os_iter->memory;
  if (iter->dir) { closedir(iter->dir); }
}

// =============================================================================
i32 main(i32 argc, char **argv) {
  lnx_state.info.core_count = get_nprocs();
  lnx_state.info.page_size = getpagesize();
  lnx_state.info.hostname = lnx_gethostname();

  lnx_state.arena = ArenaBuild();
  pthread_mutex_init(&lnx_state.primitive_lock, 0);
  lnx_parseMeminfo();

  CmdLine cli = {0};
  cli.count = argc - 1;
  cli.exe = str8_from_cstr(argv[0]);
  cli.args = New(lnx_state.arena, String8, argc - 1);
  for (isize i = 1; i < argc; ++i) {
    cli.args[i - 1] = str8_from_cstr(argv[i]);
  }

  struct timespec tms;
  struct tm lt = {0};
  (void)clock_gettime(CLOCK_REALTIME, &tms);
  (void)localtime_r(&tms.tv_sec, &lt);
  lnx_state.unix_utc_offset = lt.tm_gmtoff;

#if OS_GUI
  lnx_gfx_init();
#endif

  start(&cli);
}
