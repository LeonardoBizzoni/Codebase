#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>

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

fn String8 lnx_gethostname() {
  char name[HOST_NAME_MAX];
  (void)gethostname(name, HOST_NAME_MAX);

  String8 namestr = {
    .str = (u8 *)name,
    .size = str8len(name),
  };
  return namestr;
}

fn void lnx_parseMeminfo() {
  OS_Handle meminfo = fs_open(Strlit("/proc/meminfo"), OS_acfRead);
  StringStream lines = strSplit(lnx_state.arena, fs_readVirtual(lnx_state.arena, meminfo, 4096), '\n');
  for (StringNode *curr_line = lines.first; curr_line; curr_line = curr_line->next) {
    StringStream ss = strSplit(lnx_state.arena, curr_line->value, ':');
    for (StringNode *curr = ss.first; curr; curr = curr->next) {
      if (strEq(curr->value, Strlit("MemTotal"))) {
	curr = curr->next;
	lnx_state.info.total_memory = KiB(1) *
				u64FromStr(strSplit(lnx_state.arena, strTrim(curr->value),
						    ' ').first->value);
      } else if (strEq(curr->value, Strlit("Hugepagesize"))) {
	curr = curr->next;
	lnx_state.info.hugepage_size = KiB(1) *
				 u64FromStr(strSplit(lnx_state.arena, strTrim(curr->value),
						     ' ').first->value);
	return;
      }
    }
  }
}

fn OS_SystemInfo *os_getSystemInfo() {
  return &lnx_state.info;
}

fn time64 os_local_now() {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  time64 res = time64FromUnix(tms.tv_sec + lnx_state.unix_utc_offset);
  res |= (u64)(tms.tv_nsec / 1e6);
  return res;
}

fn DateTime os_local_dateTimeNow() {
  struct timespec tms;
  (void)clock_gettime(CLOCK_REALTIME, &tms);

  DateTime res = dateTimeFromUnix(tms.tv_sec + lnx_state.unix_utc_offset);
  res.ms = tms.tv_nsec / 1e6;
  return res;
}

fn time64 os_local_fromUTCTime64(time64 t) {
  u64 utc_time = unixFromTime64(t);
  time64 res = time64FromUnix(utc_time + lnx_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_local_fromUTCDateTime(DateTime *dt) {
  u64 utc_time = unixFromDateTime(dt);
  DateTime res = dateTimeFromUnix(utc_time + lnx_state.unix_utc_offset);
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
  time64 res = time64FromUnix(local_time - lnx_state.unix_utc_offset);
  return res | (t & bitmask10);
}

fn DateTime os_utc_fromLocalDateTime(DateTime *dt) {
  u64 local_time = unixFromDateTime(dt);
  DateTime res = dateTimeFromUnix(local_time - lnx_state.unix_utc_offset);
  res.ms = dt->ms;
  return res;
}

fn void os_sleep_milliseconds(u32 ms) {
  usleep(ms * 1e3);
}

fn OS_Handle os_timer_start() {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Timer);
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &prim->timer) != 0) {
    lnx_primitiveFree(prim);
    prim = 0;
  }

  OS_Handle res = {(u64)prim};
  return res;
}

fn u64 os_timer_elapsed(OS_TimerGranularity unit, OS_Handle start, OS_Handle end) {
  struct timespec tstart = ((LNX_Primitive *)start.h[0])->timer;
  struct timespec tend = ((LNX_Primitive *)end.h[0])->timer;
  struct timespec diff = {
    .tv_sec = tend.tv_sec - tstart.tv_sec,
    .tv_nsec = tend.tv_nsec - tstart.tv_nsec,
  };

  lnx_primitiveFree((LNX_Primitive *)start.h[0]);
  lnx_primitiveFree((LNX_Primitive *)end.h[0]);

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
  (void)mprotect(base, size, PROT_READ | PROT_WRITE);
}

fn void os_decommit(void *base, usize size) {
  (void)mprotect(base, size, PROT_NONE);
  (void)madvise(base, size, MADV_DONTNEED);
}

fn OS_Handle os_thread_start(ThreadFunc *thread_main, void *args) {
  Assert(thread_main);

  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Thread);
  prim->thread.func = thread_main;
  prim->thread.args = args;

  if (pthread_create(&prim->thread.handle, 0, lnx_threadEntry, prim) != 0) {
    lnx_primitiveFree(prim);
    prim = 0;
  }

  OS_Handle res = {(u64)prim};
  return res;
}

fn void os_thread_kill(OS_Handle thd_handle) {
  LNX_Primitive *prim = (LNX_Primitive *)thd_handle.h[0];
  (void)pthread_kill(prim->thread.handle, 0);
  lnx_primitiveFree(prim);
}

fn bool os_thread_join(OS_Handle thd_handle) {
  LNX_Primitive *prim = (LNX_Primitive *)thd_handle.h[0];
  i32 res = pthread_join(prim->thread.handle, 0);
  lnx_primitiveFree(prim);
  return res;
}

fn OS_ProcHandle os_proc_spawn() {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Process);
  prim->proc = fork();

  OS_ProcHandle res = {prim->proc == 0, {(u64)prim}};
  return res;
}

fn void os_proc_kill(OS_ProcHandle proc) {
  Assert(!proc.is_child);
  LNX_Primitive *prim = (LNX_Primitive *)proc.handle.h[0];
  (void)kill(prim->proc, SIGKILL);
  lnx_primitiveFree(prim);
}

fn void os_exit(u8 status_code) {
  exit(status_code);
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

fn OS_Handle os_mutex_alloc() {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Mutex);
  pthread_mutexattr_t attr;
  if (pthread_mutexattr_init(&attr) != 0 ||
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
    lnx_primitiveFree(prim);
    prim = 0;
  } else {
    (void)pthread_mutex_init(&prim->mutex, &attr);
  }

  OS_Handle res = {(u64)prim};
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

fn OS_Handle os_rwlock_alloc() {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_Rwlock);
  pthread_rwlock_init(&prim->rwlock, 0);

  OS_Handle res = {(u64)prim};
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

fn OS_Handle os_cond_alloc() {
  LNX_Primitive *prim = lnx_primitiveAlloc(LNX_Primitive_CondVar);
  if (pthread_cond_init(&prim->cond, 0)) {
    lnx_primitiveFree(prim);
    prim = 0;
  }

  OS_Handle res = {(u64)prim};
  return res;
}

fn void os_cond_signal(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_cond_signal(&prim->cond);
}

fn void os_cond_broadcast(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  (void)pthread_cond_broadcast(&prim->cond);
}

fn bool os_cond_wait(OS_Handle cond_handle, OS_Handle mutex_handle,
		     u32 wait_at_most_microsec) {
  LNX_Primitive *cond_prim = (LNX_Primitive *)cond_handle.h[0];
  LNX_Primitive *mutex_prim = (LNX_Primitive *)mutex_handle.h[0];

  if (wait_at_most_microsec) {
    struct timespec abstime;
    (void)clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += wait_at_most_microsec/1e6;
    abstime.tv_nsec += 1e3 * (wait_at_most_microsec - 1e6 * (wait_at_most_microsec/1e6));
    if (abstime.tv_nsec >= 1e6) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= 1e6;
    }

    return pthread_cond_timedwait(&cond_prim->cond, &mutex_prim->mutex, &abstime) == 0;
  } else {
    (void)pthread_cond_wait(&cond_prim->cond, &mutex_prim->mutex);
    return true;
  }
}

// TODO(lb): pthread doesn't provide a way to use rwlocks with condvars but i still want this
fn bool os_cond_waitrw_read(OS_Handle cond_handle, OS_Handle rwlock_handle,
			    u32 wait_at_most_microsec) {
  return false;
}

fn bool os_cond_waitrw_write(OS_Handle cond_handle, OS_Handle rwlock_handle,
			     u32 wait_at_most_microsec) {
  return false;
}

fn bool os_cond_free(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  i32 res = pthread_cond_destroy(&prim->cond);
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
      char *path = strToCstr(scratch.arena, name);
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
    abstime.tv_nsec += 1e3 * (wait_at_most_microsec - 1e6 * (wait_at_most_microsec/1e6));
    if (abstime.tv_nsec >= 1e6) {
      abstime.tv_sec += 1;
      abstime.tv_nsec -= 1e6;
    }

    return sem_timedwait(prim->semaphore.sem, &abstime);
  } else {
    return sem_wait(prim->semaphore.sem);
  }
}

fn bool os_semaphore_trywait(OS_Handle handle) {
  LNX_Primitive *prim = (LNX_Primitive *)handle.h[0];
  return sem_trywait(prim->semaphore.sem);
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

fn OS_Handle os_lib_open(String8 path) {
  OS_Handle result = {0};
  Scratch scratch = ScratchBegin(0, 0);
  char *path_cstr = strToCstr(scratch.arena, path);

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
  char *symbol_cstr = strToCstr(scratch.arena, symbol);
  VoidFunc *result = (VoidFunc*)(u64)dlsym(handle, symbol_cstr);
  ScratchEnd(scratch);
  return result;
}

fn i32 os_lib_close(OS_Handle lib) {
  void *handle = (void*)lib.h[0];
  return dlclose(handle);
}

i32 main(i32 argc, char **argv) {
  lnx_state.info.core_count = get_nprocs();
  lnx_state.info.page_size = getpagesize();
  lnx_state.info.hostname = lnx_gethostname();

  lnx_state.arena = ArenaBuild();
  pthread_mutex_init(&lnx_state.primitive_lock, 0);
  lnx_parseMeminfo();

  CmdLine cli = {0};
  cli.count = argc - 1;
  cli.exe = strFromCstr(argv[0]);
  cli.args = New(lnx_state.arena, String8, argc - 1);
  for (isize i = 1; i < argc; ++i) {
    cli.args[i - 1] = strFromCstr(argv[i]);
  }

  struct timespec tms;
  struct tm lt = {0};

  (void)clock_gettime(CLOCK_REALTIME, &tms);
  (void)localtime_r(&tms.tv_sec, &lt);

  lnx_state.unix_utc_offset = lt.tm_gmtoff;

  start(&cli);
}
