#include <sys/sysinfo.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>

global BSD_State bsd_state = {0};

fn BSD_Primitive* bsd_primitiveAlloc(BSD_PrimitiveType type) {
  pthread_mutex_lock(&bsd_state.primitive_lock);
  BSD_Primitive *res = bsd_state.primitive_freelist;
  if (res) {
    StackPop(bsd_state.primitive_freelist);
  } else {
    res = New(bsd_state.arena, BSD_Primitive);
  }
  pthread_mutex_unlock(&bsd_state.primitive_lock);

  res->type = type;
  return res;
}

fn void bsd_primitiveFree(BSD_Primitive *ptr) {
  pthread_mutex_lock(&bsd_state.primitive_lock);
  StackPush(bsd_state.primitive_freelist, ptr);
  pthread_mutex_unlock(&bsd_state.primitive_lock);
}

fn void* bsd_threadEntry(void *args) {
  BSD_Primitive *wrap_args = (BSD_Primitive *)args;
  ThreadFunc *func = (ThreadFunc *)wrap_args->thread.func;
  func(wrap_args->thread.args);
  return 0;
}

fn FS_Properties bsd_propertiesFromStat(struct stat *stat) {
  FS_Properties result = {0};
  result.ownerID = stat->st_uid;
  result.groupID = stat->st_gid;
  result.size = (usize)stat->st_size;
  result.last_access_time = (u64)stat->st_atime;
  result.last_modification_time = (u64)stat->st_mtime;
  result.last_status_change_time = (u64)stat->st_ctime;

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

fn String8 bsd_gethostname() {
  char name[MAXHOSTNAMELEN];
  (void)gethostname(name, MAXHOSTNAMELEN);

  String8 namestr = {
    .str = (u8 *)name,
    .size = str8len(name),
  };
  return namestr;
}

// =============================================================================
// System information retrieval
fn OS_SystemInfo *os_getSystemInfo() {
  return &bsd_state.info;
}

// =============================================================================
// DateTime
fn time64 os_local_now() {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64FromUnix(tms.tv_sec + bsd_state.unix_utc_offset);
  res |= (u64)(tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_local_dateTimeNow() {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = dateTimeFromUnix(tms.tv_sec + bsd_state.unix_utc_offset);
  res.ms = tms.tv_nsec / 1e6;
  return res;
}

fn time64 os_local_fromUTCTime64(time64 t) {
  u64 utc_time = unixFromTime64(t);
  time64 res = time64FromUnix(utc_time + bsd_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_local_fromUTCDateTime(DateTime *dt) {
  u64 utc_time = unixFromDateTime(dt);
  DateTime res = dateTimeFromUnix(utc_time + bsd_state.unix_utc_offset);
  res.ms = dt->ms;
  return res;
}

fn time64 os_utc_now() {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64FromUnix(tms.tv_sec);
  res |= (u64)(tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_utc_dateTimeNow() {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = dateTimeFromUnix(tms.tv_sec);
  res.ms = tms.tv_nsec / 1e6;
  return res;
}

fn time64 os_utc_localizedTime64(i8 utc_offset) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64FromUnix(tms.tv_sec + utc_offset * UNIX_HOUR);
  res |= (u64)(tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_utc_localizedDateTime(i8 utc_offset) {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = dateTimeFromUnix(tms.tv_sec + utc_offset * UNIX_HOUR);
  res.ms = tms.tv_nsec / 1e6;
  return res;
}

fn time64 os_utc_fromLocalTime64(time64 t) {
  u64 local_time = unixFromTime64(t);
  time64 res = time64FromUnix(local_time - bsd_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_utc_fromLocalDateTime(DateTime *dt) {
  u64 local_time = unixFromDateTime(dt);
  DateTime res = dateTimeFromUnix(local_time - bsd_state.unix_utc_offset);
  res.ms = dt->ms;
  return res;
}

fn void os_sleep_milliseconds(u32 ms) {
  usleep(ms * 1e3);
}

fn OS_Handle os_timer_start() {
  BSD_Primitive *prim = bsd_primitiveAlloc(BSD_Primitive_Timer);
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &prim->timer) != 0) {
    bsd_primitiveFree(prim);
    prim = 0;
  }

  OS_Handle res = {(u64)prim};
  return res;
}

fn u64 os_timer_elapsed(OS_TimerGranularity unit, OS_Handle start, OS_Handle end) {
  struct timespec tstart = ((BSD_Primitive *)start.h[0])->timer;
  struct timespec tend = ((BSD_Primitive *)end.h[0])->timer;
  struct timespec diff = {
    .tv_sec = tend.tv_sec - tstart.tv_sec,
    .tv_nsec = tend.tv_nsec - tstart.tv_nsec,
  };

  bsd_primitiveFree((BSD_Primitive *)start.h[0]);
  bsd_primitiveFree((BSD_Primitive *)end.h[0]);

  u64 res = 0;
  switch (unit) {
    case OS_TimerGranularity_min: {
      res = diff.tv_sec / 60;
    } break;
    case OS_TimerGranularity_sec: {
      res = diff.tv_sec;
    } break;
    case OS_TimerGranularity_ms: {
      res = (u64)(diff.tv_nsec / 1e6);
    } break;
    case OS_TimerGranularity_ns: {
      res = diff.tv_nsec;
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
  void *res = mmap((void *)base_addr, size, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED_SUPER, -1, 0);
  if (res == MAP_FAILED) {
    res = 0;
  }

  return res;
}

fn void os_release(void *base, usize size) {
  (void)munmap(base, size);
}

fn void os_commit(void *base, usize size) {
  (void)mprotect(base, size, PROT_READ | PROT_WRITE);
}

fn void os_decommit(void *base, usize size) {
  (void)mprotect(base, size, PROT_NONE);
  (void)madvise(base, size, MADV_DONTNEED);
}

// =============================================================================
// Threads & Processes stuff
fn OS_Handle os_thread_start(ThreadFunc *thread_main, void *args) {
  Assert(thread_main);

  BSD_Primitive *prim = bsd_primitiveAlloc(BSD_Primitive_Thread);
  prim->thread.func = thread_main;
  prim->thread.args = args;

  if (pthread_create(&prim->thread.handle, 0, bsd_threadEntry, prim) != 0) {
    bsd_primitiveFree(prim);
    prim = 0;
  }

  OS_Handle res = {(u64)prim};
  return res;
}

fn void os_thread_kill(OS_Handle thd_handle) {
  BSD_Primitive *prim = (BSD_Primitive *)thd_handle.h[0];
  (void)pthread_kill(prim->thread.handle, 0);
  bsd_primitiveFree(prim);
}

fn bool os_thread_join(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  i32 res = pthread_join(prim->thread.handle, 0);
  bsd_primitiveFree(prim);
  return res;
}

fn OS_ProcHandle os_proc_spawn() {
  BSD_Primitive *prim = bsd_primitiveAlloc(BSD_Primitive_Process);
  prim->proc = fork();

  OS_ProcHandle res = {prim->proc == 0, {(u64)prim}};
  return res;
}

fn void os_proc_kill(OS_ProcHandle proc) {
  Assert(!proc.is_child);
  BSD_Primitive *prim = (BSD_Primitive *)proc.handle.h[0];
  (void)kill(prim->proc, SIGKILL);
  bsd_primitiveFree(prim);
}

fn OS_ProcStatus os_proc_wait(OS_ProcHandle proc) {
  Assert(!proc.is_child);
  BSD_Primitive *prim = (BSD_Primitive *)proc.handle.h[0];
  i32 child_res = 0;
  (void)waitpid(prim->proc, &child_res, 0);
  bsd_primitiveFree(prim);

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

fn OS_Handle os_mutex_alloc() {
  BSD_Primitive *prim = bsd_primitiveAlloc(BSD_Primitive_Mutex);
  pthread_mutexattr_t attr;
  if (pthread_mutexattr_init(&attr) != 0 ||
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
    bsd_primitiveFree(prim);
    prim = 0;
  } else {
    (void)pthread_mutex_init(&prim->mutex, &attr);
  }

  OS_Handle res = {(u64)prim};
  return res;
}

fn void os_mutex_lock(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  (void)pthread_mutex_lock(&prim->mutex);
}

fn bool os_mutex_trylock(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  return pthread_mutex_trylock(&prim->mutex) == 0;
}

fn void os_mutex_unlock(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  (void)pthread_mutex_unlock(&prim->mutex);
}

fn void os_mutex_free(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  (void)pthread_mutex_destroy(&prim->mutex);
  bsd_primitiveFree(prim);
}

fn OS_Handle os_rwlock_alloc() {
  BSD_Primitive *prim = bsd_primitiveAlloc(BSD_Primitive_Rwlock);
  pthread_rwlock_init(&prim->rwlock, 0);

  OS_Handle res = {(u64)prim};
  return res;
}

fn void os_rwlock_read_lock(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  (void)pthread_rwlock_rdlock(&prim->rwlock);
}

fn bool os_rwlock_read_trylock(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  return pthread_rwlock_tryrdlock(&prim->rwlock) == 0;
}

fn void os_rwlock_read_unlock(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  (void)pthread_rwlock_unlock(&prim->rwlock);
}

fn void os_rwlock_write_lock(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  (void)pthread_rwlock_wrlock(&prim->rwlock);
}

fn bool os_rwlock_write_trylock(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  return pthread_rwlock_trywrlock(&prim->rwlock) == 0;
}

fn void os_rwlock_write_unlock(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  (void)pthread_rwlock_unlock(&prim->rwlock);
}

fn void os_rwlock_free(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  pthread_rwlock_destroy(&prim->rwlock);
  bsd_primitiveFree(prim);
}


fn OS_Handle os_cond_alloc() {
  BSD_Primitive *prim = bsd_primitiveAlloc(BSD_Primitive_CondVar);
  if (pthread_cond_init(&prim->condvar.cond, 0)) {
    bsd_primitiveFree(prim);
    prim = 0;
  } else {
    (void)pthread_mutex_init(&prim->condvar.mutex, 0);
  }

  OS_Handle res = {(u64)prim};
  return res;
}

fn void os_cond_signal(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  (void)pthread_cond_signal(&prim->condvar.cond);
}

fn void os_cond_broadcast(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  (void)pthread_cond_broadcast(&prim->condvar.cond);
}

fn bool os_cond_wait(OS_Handle cond_handle, OS_Handle mutex_handle,
		     u32 wait_at_most_microsec) {
  BSD_Primitive *cond_prim = (BSD_Primitive *)cond_handle.h[0];
  BSD_Primitive *mutex_prim = (BSD_Primitive *)mutex_handle.h[0];

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
  BSD_Primitive *cond_prim = (BSD_Primitive *)cond_handle.h[0];
  BSD_Primitive *rwlock_prim = (BSD_Primitive *)rwlock_handle.h[0];

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
  BSD_Primitive *cond_prim = (BSD_Primitive *)cond_handle.h[0];
  BSD_Primitive *rwlock_prim = (BSD_Primitive *)rwlock_handle.h[0];

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
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  i32 res = pthread_cond_destroy(&prim->condvar.cond) &
	    pthread_mutex_destroy(&prim->condvar.mutex);
  bsd_primitiveFree(prim);
  return res == 0;
}

fn OS_Handle os_semaphore_alloc(OS_SemaphoreKind kind, u32 init_count,
				u32 max_count, String8 name) {
  BSD_Primitive *prim = bsd_primitiveAlloc(BSD_Primitive_Semaphore);
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
      char *path = cstrFromStr8(scratch.arena, name);
      (void)sem_unlink(path);
      prim->semaphore.sem = sem_open(path, O_CREAT,
				     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, init_count);
      if (prim->semaphore.sem == SEM_FAILED) {
	prim->semaphore.sem = 0;
      }
      ScratchEnd(scratch);
    } break;
  }

  OS_Handle res = {(u64)prim};
  return res;
}

fn bool os_semaphore_signal(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  if (prim->semaphore.count + 1 >= prim->semaphore.max_count ||
      sem_post(prim->semaphore.sem)) {
    return false;
  }
  return ++prim->semaphore.count;
}

fn bool os_semaphore_wait(OS_Handle handle, u32 wait_at_most_microsec) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += wait_at_most_microsec/1e6;
    abstime.tv_nsec += 1e3 * (wait_at_most_microsec % (u32)1e6);
    if (abstime.tv_nsec >= 1e9) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= 1e9;
    }

    return sem_timedwait(prim->semaphore.sem, &abstime);
  } else {
    return sem_wait(prim->semaphore.sem);
  }
}

fn bool os_semaphore_trywait(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  return sem_trywait(prim->semaphore.sem);
}

fn void os_semaphore_free(OS_Handle handle) {
  BSD_Primitive *prim = (BSD_Primitive *)handle.h[0];
  switch (prim->semaphore.kind) {
    case OS_SemaphoreKind_Thread: {
      (void)sem_destroy(prim->semaphore.sem);
      os_release(prim->semaphore.sem, sizeof(sem_t));
    } break;
    case OS_SemaphoreKind_Process: {
      (void)sem_close(prim->semaphore.sem);
    } break;
  }
  bsd_primitiveFree(prim);
}

fn SharedMem os_sharedmem_open(String8 name, usize size, OS_AccessFlags flags) {
  SharedMem res = {0};
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
  res.file_handle.h[0] = shm_open(cstrFromStr8(scratch.arena, name), access_flags,
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
	     shm_unlink(cstrFromStr8(scratch.arena, shm->path)) == 0;
  ScratchEnd(scratch);
  return res;
}


// =============================================================================
// Dynamic libraries
fn OS_Handle os_lib_open(String8 path) {
  OS_Handle result = {0};
  Scratch scratch = ScratchBegin(0, 0);
  char *path_cstr = cstrFromStr8(scratch.arena, path);
  void *handle = dlopen(path_cstr, RTLD_LAZY);
  if(handle){
    result.h[0] = (u64)handle;
  }
  ScratchEnd(scratch);
  return result;
}

fn VoidFunc *os_lib_lookup(OS_Handle lib, String8 symbol) {
  Scratch scratch = ScratchBegin(0, 0);
  void *handle = (void*)lib.h[0];
  char *symbol_cstr = cstrFromStr8(scratch.arena, symbol);
  VoidFunc *result = (VoidFunc*)(usize)dlsym(handle, symbol_cstr);
  ScratchEnd(scratch);
  return result;
}

fn i32 os_lib_close(OS_Handle lib) {
  void *handle = (void*)lib.h[0];
  return dlclose(handle);
}

i32 main(i32 argc, char **argv) {
  i32 mib[2] = { CTL_HW, HW_NCPU };
  usize len = sizeof(bsd_state.info.core_count);
  sysctl(mib, 2, &bsd_state.info.core_count, &len, 0, 0);

  mib[1] = HW_PHYSMEM;
  len = sizeof(bsd_state.info.total_memory);
  sysctl(mib, 2, &bsd_state.info.total_memory, &len, 0, 0);

  usize page_sizes[3] = {0};
  getpagesizes(page_sizes, 3);
  bsd_state.info.page_size = page_sizes[0];
#ifdef USE_SUPERLARGE_PAGES
  bsd_state.info.hugepage_size = page_sizes[2];
#else
  bsd_state.info.hugepage_size = page_sizes[1];
#endif

  bsd_state.info.hostname = bsd_gethostname();
  bsd_state.arena = ArenaBuild();

  CmdLine cli = {0};
  cli.count = argc - 1;
  cli.exe = strFromCstr(argv[0]);
  cli.args = New(bsd_state.arena, String8, argc - 1);
  for (isize i = 1; i < argc; ++i) {
    cli.args[i - 1] = strFromCstr(argv[i]);
  }

  struct timespec tms;
  struct tm lt = {0};

  (void)clock_gettime(CLOCK_REALTIME, &tms);
  (void)localtime_r(&tms.tv_sec, &lt);

  bsd_state.unix_utc_offset = lt.tm_gmtoff;

  start(&cli);
}
