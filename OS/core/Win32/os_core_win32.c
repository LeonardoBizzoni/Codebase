#pragma comment(lib, "ws2_32.lib")

global W32_State w32_state = {};

fn OS_SystemInfo* os_sysinfo(void) {
  return &w32_state.info;
}

// =============================================================================
// DateTime stuff
fn time64 os_local_now(void) {
  SYSTEMTIME systime;
  GetLocalTime(&systime);
  return w32_time64_from_system_time(&systime);
}

fn DateTime os_local_dateTimeNow(void) {
  SYSTEMTIME system_time;
  GetLocalTime(&system_time);
  DateTime result = w32_date_time_from_system_time(&system_time);
  return result;
}

fn time64 os_local_fromUTCTime64(time64 in) {
  SYSTEMTIME utc = {0};
  SYSTEMTIME local_time = w32_system_time_from_time64(in);
  SystemTimeToTzSpecificLocalTime(0, &local_time, &utc);
  return w32_time64_from_system_time(&utc);
}

fn DateTime os_local_fromUTCDateTime(DateTime *in) {
  SYSTEMTIME systime = w32_system_time_from_date_time(in);
  FILETIME ftime;
  SystemTimeToFileTime(&systime, &ftime);
  FILETIME ftime_local;
  FileTimeToLocalFileTime(&ftime, &ftime_local);
  FileTimeToSystemTime(&ftime_local, &systime);
  DateTime result = w32_date_time_from_system_time(&systime);
  return result;
}

fn time64 os_utc_now(void) {
  SYSTEMTIME system_time;
  GetSystemTime(&system_time);
  return w32_time64_from_system_time(&system_time);
}

fn DateTime os_utc_dateTimeNow(void) {
  SYSTEMTIME system_time;
  GetSystemTime(&system_time);
  DateTime result = w32_date_time_from_system_time(&system_time);
  return result;
}

fn time64 os_utc_localizedTime64(i8 utc_offset) {
  time64 now = os_utc_now();
  i32 year = ((now >> 36) & ~(1 << 27));

  i8 month = (i8)((now >> 32) & bitmask4);
  i8 day = (i8)((now >> 27) & bitmask5);
  i8 hour = (i8)(((now >> 22) & bitmask5) + utc_offset);
  if (hour < 0) {
    day -= 1;
    hour += 24;
  } else if (hour > 23) {
    day += 1;
    hour -= 24;
  }

  if (day < 1) {
    month -= 1;
    day = daysXmonth[month - 1];
  } else if (day > daysXmonth[month - 1]) {
    month += 1;
    day = 1;
  }

  if (month < 1) {
    year -= 1;
    month = 12;
  } else if (month > 12) {
    year += 1;
    month = 1;
  }

  time64 res = now;
  res = (res & (~((u64)bitmask27 << 36))) | ((u64)year << 36);
  res = (res & (~((u64)bitmask4 << 32)))  | ((u64)month << 32);
  res = (res & (~((u64)bitmask5 << 27)))  | ((u64)day << 27);
  res = (res & (~((u64)bitmask5 << 22)))  | ((u64)hour << 22);
  return res;
}

fn DateTime os_utc_localizedDateTime(i8 utc_offset) {
  DateTime res = os_utc_dateTimeNow();
  res.hour += utc_offset;
  if (res.hour < 0) {
    res.day -= 1;
    res.hour += 24;
  } else if (res.hour > 23) {
    res.day += 1;
    res.hour -= 24;
  }

  if (res.day < 1) {
    res.month -= 1;
    res.day = daysXmonth[res.month - 1];
  } else if (res.day > daysXmonth[res.month - 1]) {
    res.month += 1;
    res.day = 1;
  }

  if (res.month < 1) {
    res.year -= 1;
    res.month = 12;
  } else if (res.month > 12) {
    res.year += 1;
    res.month = 1;
  }

  return res;
}

fn time64 os_utc_fromLocalTime64(time64 in) {
  SYSTEMTIME utctime = {0};
  SYSTEMTIME localtime = w32_system_time_from_time64(in);
  TzSpecificLocalTimeToSystemTime(NULL, &localtime, &utctime);
  return w32_time64_from_system_time(&utctime);
}

fn DateTime os_utc_fromLocalDateTime(DateTime *in) {
  SYSTEMTIME utctime = {0};
  SYSTEMTIME localtime = w32_system_time_from_date_time(in);
  TzSpecificLocalTimeToSystemTime(NULL, &localtime, &utctime);
  return w32_date_time_from_system_time(&utctime);
}

fn void os_sleep_milliseconds(u32 ms) {
  Sleep(ms);
}

fn OS_Handle os_timer_start(void) {
  W32_Primitive *primitive = w32_primitive_alloc(W32_Primitive_Timer);
  QueryPerformanceCounter(&primitive->timer);

  OS_Handle res = {{(u64)primitive}};
  return res;
}

fn u64 os_timer_elapsed_between(OS_Handle start, OS_Handle end,
                                OS_TimerGranularity unit) {
  W32_Primitive *start_prim = (W32_Primitive*)start.h[0];
  W32_Primitive *end_prim = (W32_Primitive*)end.h[0];

  u64 micros = (end_prim->timer.QuadPart - start_prim->timer.QuadPart) * Million(1)
               / w32_state.perf_freq.QuadPart;

  switch (unit) {
    case OS_TimerGranularity_min: {
      return micros / (60 * Million(1));
    } break;
    case OS_TimerGranularity_sec: {
      return micros / Million(1);
    } break;
    case OS_TimerGranularity_ms: {
      return micros / Thousand(1);
    } break;
    case OS_TimerGranularity_ns: {
      return micros * Thousand(1);
    } break;
  }
  return 0;
}

fn void os_timer_free(OS_Handle handle) {
  w32_primitive_release((W32_Primitive*)handle.h[0]);
}

////////////////////////////////
//- km: memory
fn void* os_reserve(isize size) {
  Assert(size > 0);
  void *result = VirtualAlloc(0, (usize)size, MEM_RESERVE,
                              PAGE_READWRITE);
  return result;
}

fn void* os_reserve_huge(isize size) {
  Assert(size > 0);
  void *result = VirtualAlloc(0, (usize)size,
                              MEM_RESERVE | MEM_LARGE_PAGES,
                              PAGE_READWRITE);
  return result;
}

fn void os_release(void *base, isize size) {
  Assert(size > 0);
  BOOL result = VirtualFree(base, (usize)size, MEM_RELEASE);
  (void)result;
}

fn void os_commit(void *base, isize size) {
  Assert(size > 0);
  void *result = VirtualAlloc(base, (usize)size, MEM_COMMIT,
                              PAGE_READWRITE);
  (void)result;
}

fn void os_decommit(void *base, isize size) {
  Assert(size > 0);
  BOOL result = VirtualFree(base, (usize)size, MEM_DECOMMIT);
  (void)result;
}

// =============================================================================
// Threads & Processes stuff
fn OS_Handle os_thread_start(Func_Thread *func, void *arg) {
  W32_Primitive *primitive = w32_primitive_alloc(W32_Primitive_Thread);
  HANDLE handle = CreateThread(0, 0, w32_thread_entry_point, primitive, 0,
                               &primitive->thread.tid);
  primitive->thread.func = func;
  primitive->thread.arg = arg;
  primitive->thread.handle = handle;
  os_mutex_scope(w32_state.thread_list.mutex) {
    DLLPushBack(w32_state.thread_list.first,
                w32_state.thread_list.last, primitive);
  }
  OS_Handle result = {(u64)primitive};
  return result;
}

fn void os_thread_kill(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  os_mutex_scope(w32_state.thread_list.mutex) {
    DLLDelete(w32_state.thread_list.first,
              w32_state.thread_list.last, primitive);
  }
  BOOL res = TerminateThread(primitive->thread.handle, 0);
  (void)res;
}

fn void os_thread_cancel(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  primitive->thread.should_quit = true;
  WaitForSingleObject(primitive->thread.handle, INFINITE);
  CloseHandle(primitive->thread.handle);
  os_mutex_scope(w32_state.thread_list.mutex) {
    DLLDelete(w32_state.thread_list.first,
              w32_state.thread_list.last, primitive);
  }
  w32_primitive_release(primitive);
}

fn bool os_thread_join(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  DWORD wait = WaitForSingleObject(primitive->thread.handle, INFINITE);
  CloseHandle(primitive->thread.handle);
  os_mutex_scope(w32_state.thread_list.mutex) {
    DLLDelete(w32_state.thread_list.first,
              w32_state.thread_list.last, primitive);
  }
  w32_primitive_release(primitive);
  return wait == WAIT_OBJECT_0;
}

fn void os_thread_cancelpoint(void) {
  HANDLE thd = GetCurrentThread();
  for (W32_Primitive *curr = w32_state.thread_list.first;
       curr;
       curr = curr->next) {
    if (curr->thread.handle == thd && curr->thread.should_quit) {
      ExitThread(0);
      return; // unreachable
    }
  }
}

fn OS_Handle os_proc_spawn(String8 command) {
  OS_Handle res = {};

  STARTUPINFO si = {};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

  PROCESS_INFORMATION pi = {};
  Scratch scratch = ScratchBegin(0, 0);
  if (CreateProcessA(0, cstr_from_str8(scratch.arena, command),
                      0, 0, TRUE, 0, 0, 0, &si, &pi)) {
    CloseHandle(pi.hThread);
    res.h[0] = (u64)pi.hProcess;
  }
  ScratchEnd(scratch);
  return res;
}

fn void os_proc_kill(OS_Handle handle) {
  (void)TerminateProcess((HANDLE)handle.h[0], 0);
}

fn i32 os_proc_wait(OS_Handle handle) {
  i32 res = 0;
  HANDLE proc = (HANDLE)handle.h[0];
  WaitForSingleObject(proc, INFINITE);
  GetExitCodeProcess(proc, &res);
  CloseHandle(proc);
  return res;
}

fn void os_exit(u8 status_code) {
  ExitProcess(status_code);
}

fn void os_atexit(Func_Void *callback) {
  _crt_atexit(callback);
}

fn OS_Handle os_mutex_alloc(void) {
  W32_Primitive *primitive = w32_primitive_alloc(W32_Primitive_Mutex);
  InitializeCriticalSection(&primitive->mutex);
  OS_Handle result = {(u64)primitive};
  return result;
}

fn void os_mutex_lock(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  EnterCriticalSection(&primitive->mutex);
}

fn bool os_mutex_trylock(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  BOOL result = TryEnterCriticalSection(&primitive->mutex);
  return result;
}

fn void os_mutex_unlock(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  LeaveCriticalSection(&primitive->mutex);
}

fn void os_mutex_free(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  DeleteCriticalSection(&primitive->mutex);
  w32_primitive_release(primitive);
}

fn OS_Handle os_rwlock_alloc(void) {
  W32_Primitive *primitive = w32_primitive_alloc(W32_Primitive_RWLock);
  InitializeSRWLock(&primitive->rw_mutex);
  OS_Handle result = {(u64)primitive};
  return result;
}

fn void os_rwlock_read_lock(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  AcquireSRWLockShared(&primitive->rw_mutex);
}

fn bool os_rwlock_read_trylock(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  BOOLEAN result = TryAcquireSRWLockShared(&primitive->rw_mutex);
  return result;
}

fn void os_rwlock_read_unlock(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  ReleaseSRWLockShared(&primitive->rw_mutex);
}

fn void os_rwlock_write_lock(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  AcquireSRWLockExclusive(&primitive->rw_mutex);
}

fn bool os_rwlock_write_trylock(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  BOOLEAN result = TryAcquireSRWLockExclusive(&primitive->rw_mutex);
  return result;
}

fn void os_rwlock_write_unlock(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  ReleaseSRWLockExclusive(&primitive->rw_mutex);
}

fn void os_rwlock_free(OS_Handle handle) {
  W32_Primitive *primitive = (W32_Primitive*)handle.h[0];
  w32_primitive_release(primitive);
}

fn OS_Handle os_cond_alloc(void) {
  W32_Primitive *prim = w32_primitive_alloc(W32_Primitive_CondVar);
  InitializeConditionVariable(&prim->condvar);
  OS_Handle res = {(u64)prim};
  return res;
}

fn void os_cond_signal(OS_Handle handle) {
  W32_Primitive *prim = (W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != W32_Primitive_CondVar) { return; }
  WakeConditionVariable(&prim->condvar);
}

fn void os_cond_broadcast(OS_Handle handle) {
  W32_Primitive *prim = (W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != W32_Primitive_CondVar) { return; }
  WakeAllConditionVariable(&prim->condvar);
}

fn bool os_cond_wait(OS_Handle cond_handle, OS_Handle mutex_handle,
                     u32 wait_at_most_microsec) {
  W32_Primitive *condprim = (W32_Primitive*)cond_handle.h[0];
  W32_Primitive *mutexprim = (W32_Primitive*)mutex_handle.h[0];
  if (!condprim || condprim->kind != W32_Primitive_CondVar) { return false; }
  if (!mutexprim || mutexprim->kind != W32_Primitive_Mutex) { return false; }
  return SleepConditionVariableCS(&condprim->condvar, &mutexprim->mutex,
                                  (wait_at_most_microsec
                                   ? wait_at_most_microsec / Thousand(1)
                                   : INFINITE)) != 0;
}

fn bool os_cond_waitrw_read(OS_Handle cond_handle, OS_Handle rwlock_handle,
                            u32 wait_at_most_microsec) {
  W32_Primitive *condprim = (W32_Primitive*)cond_handle.h[0];
  W32_Primitive *rwlockprim = (W32_Primitive*)rwlock_handle.h[0];
  if (!condprim || condprim->kind != W32_Primitive_CondVar) { return false; }
  if (!rwlockprim || rwlockprim->kind != W32_Primitive_RWLock)
    { return false; }
  return SleepConditionVariableSRW(&condprim->condvar, &rwlockprim->rw_mutex,
                                   (wait_at_most_microsec
                                    ? wait_at_most_microsec / Thousand(1)
                                    : INFINITE),
                                   CONDITION_VARIABLE_LOCKMODE_SHARED) != 0;
}

fn bool os_cond_waitrw_write(OS_Handle cond_handle, OS_Handle rwlock_handle,
                             u32 wait_at_most_microsec) {
  W32_Primitive *condprim = (W32_Primitive*)cond_handle.h[0];
  W32_Primitive *rwlockprim = (W32_Primitive*)rwlock_handle.h[0];
  if (!condprim || condprim->kind != W32_Primitive_CondVar) { return false; }
  if (!rwlockprim || rwlockprim->kind != W32_Primitive_RWLock)
    { return false; }
  return SleepConditionVariableSRW(&condprim->condvar, &rwlockprim->rw_mutex,
                                   (wait_at_most_microsec
                                    ? wait_at_most_microsec / Thousand(1)
                                    : INFINITE),
                                   CONDITION_VARIABLE_LOCKMODE_EXCLUSIVE) != 0;
}

fn bool os_cond_free(OS_Handle handle) {
  W32_Primitive *prim = (W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != W32_Primitive_CondVar) { return false; }
  w32_primitive_release(prim);
  return true;
}

fn OS_Handle os_semaphore_alloc(OS_SemaphoreKind kind, u32 init_count,
                                u32 max_count, String8 name) {
  W32_Primitive *prim = w32_primitive_alloc(W32_Primitive_Semaphore);

  Scratch scratch = ScratchBegin(0, 0);
  StringBuilder ss = {0};
  switch (kind) {
  case OS_SemaphoreKind_Thread: {
    if (name.size == 0) { goto skip_semname; }
    strstream_append_str(scratch.arena, &ss, Strlit("Local\\\\"));
  } break;
  case OS_SemaphoreKind_Process: {
    strstream_append_str(scratch.arena, &ss, Strlit("Global\\\\"));
  } break;
  }
  strstream_append_str(scratch.arena, &ss, name);
  prim->semaphore = CreateSemaphoreA(0, init_count, max_count,
                                     cstr_from_str8(scratch.arena,
                                                    str8_from_stream(scratch.arena,
                                                                     ss)));
 skip_semname:
  ScratchEnd(scratch);
  OS_Handle res = {(u64)prim};
  return res;
}

fn bool os_semaphore_signal(OS_Handle handle) {
  W32_Primitive *prim = (W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != W32_Primitive_Semaphore) { return false; }
  return ReleaseSemaphore(prim->semaphore, 1, 0);
}

fn bool os_semaphore_wait(OS_Handle handle, u32 wait_at_most_microsec) {
  W32_Primitive *prim = (W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != W32_Primitive_Semaphore) { return false; }
  return WaitForSingleObject(prim->semaphore,
                             (wait_at_most_microsec
                              ? wait_at_most_microsec / Thousand(1)
                              : INFINITE)) == WAIT_OBJECT_0;
}

fn bool os_semaphore_trywait(OS_Handle handle) {
  W32_Primitive *prim = (W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != W32_Primitive_Semaphore) { return false; }
  return WaitForSingleObject(prim->semaphore, 0) == WAIT_OBJECT_0;
}

fn void os_semaphore_free(OS_Handle handle) {
  W32_Primitive *prim = (W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != W32_Primitive_Semaphore) { return; }
  CloseHandle(prim->semaphore);
  w32_primitive_release(prim);
}

fn SharedMem os_sharedmem_open(String8 name, isize size,
                               OS_AccessFlags flags) {
  Assert(size > 0);
  SharedMem res = {0};
  DWORD access_flags = 0;
  if (flags & OS_acfRead)    { access_flags |= GENERIC_READ;}
  if (flags & OS_acfWrite)   { access_flags |= GENERIC_WRITE; }
  if (flags & OS_acfExecute) { access_flags |= GENERIC_EXECUTE; }
  if (flags & OS_acfAppend)  { access_flags |= FILE_APPEND_DATA; }

  Scratch scratch = ScratchBegin(0, 0);
  StringBuilder ss = {0};
  strstream_append_str(scratch.arena, &ss, Strlit("Global\\\\"));
  strstream_append_str(scratch.arena, &ss, name);
  res.path = str8_from_stream(scratch.arena, ss);
  res.mmap_handle.h[0] = (u64)
    CreateFileMapping(INVALID_HANDLE_VALUE, 0, access_flags,
                      (u32)((size >> 32) & bitmask32),
                      (u32)(size & bitmask32),
                      !name.size ? (char *)0 : cstr_from_str8(scratch.arena,
                                                              res.path));
  ScratchEnd(scratch);
  if (!res.mmap_handle.h[0]) { return res; }

  access_flags = 0;
  if (flags & OS_acfRead)    { access_flags |= FILE_MAP_READ;}
  if (flags & OS_acfWrite)   { access_flags |= FILE_MAP_WRITE; }
  if (flags & OS_acfExecute) { access_flags |= FILE_MAP_EXECUTE; }
  if (flags & OS_acfAppend)  { access_flags |= FILE_MAP_ALL_ACCESS; }
  res.content = (u8*)MapViewOfFile((HANDLE)res.mmap_handle.h[0], access_flags,
                                   0, 0, size);
  if (!res.content) {
    CloseHandle((HANDLE)res.mmap_handle.h[0]);
    SharedMem _ = {0};
    return _;
  }

  return res;
}

fn void os_sharedmem_close(SharedMem *shm) {
  (void)UnmapViewOfFile(shm->content);
  (void)CloseHandle((HANDLE)shm->mmap_handle.h[0]);
}

////////////////////////////////
//- km: Dynamic libraries
fn OS_Handle os_lib_open(String8 path) {
  Scratch scratch = ScratchBegin(0,0);
  HMODULE module = LoadLibraryA((LPCSTR)cstr_from_str8(scratch.arena,
                                                       path));
  ScratchEnd(scratch);
  OS_Handle result = {{(u64)module}};
  return result;
}

fn Func_Void* os_lib_lookup(OS_Handle lib, String8 symbol) {
  if (!lib.h[0]) { return 0; }
  HMODULE module = (HMODULE)lib.h[0];
  Scratch scratch = ScratchBegin(0,0);
  char *symbol_cstr = cstr_from_str8(scratch.arena, symbol);
  Func_Void *result = (Func_Void*)GetProcAddress(module, symbol_cstr);
  ScratchEnd(scratch);
  return result;
}

fn i32 os_lib_close(OS_Handle lib) {
  HMODULE module = (HMODULE)lib.h[0];
  BOOL result = FreeLibrary(module);
  return result;
}

// =============================================================================
// Misc
fn String8 os_currentDir(Arena *arena) {
  String8 res = {};
  res.str = arena_push_many(arena, u8, MAX_PATH);
  res.size = GetCurrentDirectoryA(MAX_PATH, (LPSTR)res.str);
  arena_pop(arena, MAX_PATH - res.size);
  return res;
}

// =============================================================================
// Networking
fn IP os_net_ip_from_str8(String8 name, OS_Net_Network hint) {
  IP res = {0};
  ADDRINFOA *info = 0;
  ADDRINFOA hints = {0};
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
  int err = getaddrinfo(cstr_from_str8(scratch.arena, name), 0,
                        0, &info);
  ScratchEnd(scratch);
  Assert(!err);
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
  return res;
}

// TODO(lb): implement os_net_interfaces
fn NetInterfaceList os_net_interfaces(Arena *arena) {
  NetInterfaceList res = {};
  return res;
}

fn OS_Socket os_socket_open(String8 name, u16 port, OS_Net_Transport protocol) {
  OS_Socket res = {0};
  IP ip = os_net_ip_from_str8(name, 0);
  W32_Primitive *prim = w32_primitive_alloc(W32_Primitive_Socket);
  i32 ctype, cdomain;
  switch (ip.version) {
    case OS_Net_Network_IPv4: {
      cdomain = AF_INET;
      struct sockaddr_in *addr = (struct sockaddr_in*)&prim->socket.addr;
      addr->sin_family = (ADDRESS_FAMILY)cdomain;
      addr->sin_port = htons(port);
      memcopy(&addr->sin_addr, ip.v4.bytes, 4 * sizeof(u8));
      prim->socket.size = sizeof(struct sockaddr_in);
    } break;
    case OS_Net_Network_IPv6: {
      cdomain = AF_INET6;
      struct sockaddr_in6 *addr = (struct sockaddr_in6*)&prim->socket.addr;
      addr->sin6_family = (ADDRESS_FAMILY)cdomain;
      addr->sin6_port = htons(port);
      memcopy(&addr->sin6_addr, ip.v4.bytes, 8 * sizeof(u16));
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

  prim->socket.handle = socket(cdomain, ctype, 0);
  if (prim->socket.handle == INVALID_SOCKET) {
    w32_primitive_release(prim);
    return res;
  }

  res.protocol_transport = protocol;
  res.server.addr = ip;
  res.server.port = port;
  res.handle.h[0] = (u64)prim;
  return res;
}

fn void os_socket_listen(OS_Socket *socket, u8 max_backlog) {
  W32_Primitive *prim = (W32_Primitive*)socket->handle.h[0];

  i32 optval = 1;
  (void)setsockopt(prim->socket.handle, SOL_SOCKET,
                   SO_REUSEADDR, (char*)&optval, sizeof(optval));
  (void)bind(prim->socket.handle, &prim->socket.addr, prim->socket.size);
  (void)listen(prim->socket.handle, max_backlog);
}

fn OS_Socket os_socket_accept(OS_Socket *socket) {
  OS_Socket res = {0};

  W32_Primitive *server_prim = (W32_Primitive*)socket->handle.h[0];
  W32_Primitive *client_prim = w32_primitive_alloc(W32_Primitive_Socket);
  client_prim->socket.size = sizeof(client_prim->socket.addr);
  client_prim->socket.handle = accept(server_prim->socket.handle,
                                      &client_prim->socket.addr,
                                      &client_prim->socket.size);
  if (client_prim->socket.handle == INVALID_SOCKET) {
    w32_primitive_release(client_prim);
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

fn void os_socket_connect(OS_Socket *server) {
  W32_Primitive *prim = (W32_Primitive*)server->handle.h[0];
  connect(prim->socket.handle, &prim->socket.addr, prim->socket.size);

  struct sockaddr client = {0};
  socklen_t client_len = sizeof(struct sockaddr);
  (void)getsockname(prim->socket.handle, &client, &client_len);
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

fn u8* os_socket_recv(Arena *arena, OS_Socket *client, usize buffer_size) {
  u8 *res = arena_push_many(arena, u8, buffer_size);
  W32_Primitive *prim = (W32_Primitive*)client->handle.h[0];
  recv(prim->socket.handle, (char*)res, (int)buffer_size, MSG_WAITALL);
  return res;
}

fn void os_socket_send_str8(OS_Socket *socket, String8 msg) {
  W32_Primitive *prim = (W32_Primitive*)socket->handle.h[0];
  Scratch scratch = ScratchBegin(0, 0);
  send(prim->socket.handle, cstr_from_str8(scratch.arena, msg), (int)msg.size, 0);
  ScratchEnd(scratch);
}

fn void os_socket_close(OS_Socket *socket) {
  W32_Primitive *prim = (W32_Primitive*)socket->handle.h[0];
  shutdown(prim->socket.handle, SD_BOTH);
  closesocket(prim->socket.handle);
}

////////////////////////////////
//- km: File operations
fn OS_Handle os_fs_open(String8 filepath, OS_AccessFlags flags) {
  W32_Primitive *prim = w32_primitive_alloc(W32_Primitive_File);
  prim->file.flags = flags;

  Scratch scratch = ScratchBegin(0, 0);
  String16 path = str16_from_str8(scratch.arena, filepath);
  SECURITY_ATTRIBUTES security_attributes = {sizeof(SECURITY_ATTRIBUTES), 0, 0};
  DWORD access_flags = 0;
  DWORD share_mode = 0;
  DWORD creation_disposition = OPEN_EXISTING;

  if (flags & OS_acfRead) {
    access_flags |= GENERIC_READ;
    share_mode |= FILE_SHARE_READ;
  }
  if (flags & OS_acfWrite) {
    access_flags |= GENERIC_WRITE;
    creation_disposition = OPEN_ALWAYS;
  }
  if (flags & OS_acfAppend) {
    access_flags |= FILE_APPEND_DATA;
    creation_disposition = OPEN_ALWAYS;
  }
  if (flags & OS_acfExecute) { access_flags |= GENERIC_EXECUTE; }
  if (flags & OS_acfShareRead) { share_mode |= FILE_SHARE_READ; }
  if (flags & OS_acfShareWrite) { share_mode |= FILE_SHARE_WRITE; }

  prim->file.handle = CreateFileW((WCHAR*)path.str, access_flags,
                                  share_mode, &security_attributes,
                                  creation_disposition,
                                  FILE_ATTRIBUTE_NORMAL, 0);
  ScratchEnd(scratch);

  OS_Handle result = {0};
  if (prim->file.handle != INVALID_HANDLE_VALUE) {
    result.h[0] = (u64)prim;
  }
  return result;
}

fn bool os_fs_close(OS_Handle fd) {
  W32_Primitive *prim = (W32_Primitive*)fd.h[0];
  if (!prim) { return false; }
  BOOL res = CloseHandle(prim->file.handle);
  return res;
}

fn String8 os_fs_read(Arena *arena, OS_Handle handle) {
  String8 result = {};

  W32_Primitive *prim = (W32_Primitive*)handle.h[0];
  Assert(prim);
  HANDLE file = prim->file.handle;
  if (file == 0) { return result; }

  LARGE_INTEGER file_size = {0};
  GetFileSizeEx(file, &file_size);

  u64 to_read = file_size.QuadPart;
  u64 total_read = 0;
  u8 *ptr = arena_push_many(arena, u8, to_read);

  for (;total_read < to_read;) {
    u64 amount64 = to_read - total_read;
    u32 amount32 = amount64 > U32_MAX ? U32_MAX : (u32)amount64;
    OVERLAPPED overlapped = {0};
    DWORD bytes_read = 0;
    (void)ReadFile(file, ptr + total_read, amount32, &bytes_read, &overlapped);
    total_read += bytes_read;
    if (bytes_read != amount32) { break; }
  }

  if (total_read == to_read) {
    result.str = ptr;
    result.size = to_read;
  }
  return result;
}

// TODO(lb): i don't know if there are some location on the Windows
//           FS that contain files with size=0.
fn String8 os_fs_read_virtual(Arena *arena, OS_Handle file, usize size) {
  return os_fs_read(arena, file);
}

fn bool os_fs_write(OS_Handle handle, String8 content) {
  bool result = false;

  HANDLE file = ((W32_Primitive*)handle.h[0])->file.handle;
  if (file == 0) { return result; }

  u64 to_write = content.size;
  u64 total_write = 0;
  u8 *ptr = content.str;
  for (;total_write < to_write;) {
    u64 amount64 = to_write - total_write;;
    u32 amount32 = amount64 > U32_MAX ? U32_MAX : (u32)amount64;
    DWORD bytes_written = 0;
    WriteFile(file, ptr + total_write, amount32, &bytes_written, 0);
    total_write += bytes_written;
    if (bytes_written != amount32) { break; }
  }

  if (total_write == to_write) {
    result = true;
  }
  return result;
}

fn bool os_fs_copy(String8 src, String8 dest) {
  Scratch scratch = ScratchBegin(0, 0);
  BOOL res = CopyFile(cstr_from_str8(scratch.arena, src),
                      cstr_from_str8(scratch.arena, dest),
                      FALSE);
  if (!res) {
    DWORD err = GetLastError();
  }
  ScratchEnd(scratch);
  return res;
}

fn FS_Properties os_fs_get_properties(OS_Handle file) {
  FS_Properties properties = {0};
  if (!file.h[0]) { return properties; }

  SECURITY_DESCRIPTOR *security = 0;
  SID *owner = 0;
  SID *group = 0;

  AssertMsg(GetSecurityInfo(((W32_Primitive*)file.h[0])->file.handle,
                            SE_FILE_OBJECT,
                            OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION,
                            (PSID*)&properties.ownerID,
                            (PSID*)&properties.groupID, 0, 0,
                            (PSECURITY_DESCRIPTOR *)&security) == ERROR_SUCCESS,
            "GetSecurityInfo");

  BY_HANDLE_FILE_INFORMATION file_info;
  Assert(GetFileInformationByHandle(((W32_Primitive*)file.h[0])->file.handle, &file_info));

  properties.size = (u64)(((u64)file_info.nFileSizeHigh << 32) |
                          file_info.nFileSizeLow);

  switch (file_info.dwFileAttributes) {
  case FILE_ATTRIBUTE_DIRECTORY: {
    properties.type = OS_FileType_Dir;
  } break;
  case FILE_ATTRIBUTE_DEVICE: {
    properties.type = OS_FileType_BlkDevice;
  } break;
  case FILE_ATTRIBUTE_REPARSE_POINT: {
    properties.type = OS_FileType_Link;
  } break;
  case FILE_ATTRIBUTE_NORMAL: {
    properties.type = OS_FileType_Regular;
  } break;
  default: {
    properties.type = OS_FileType_Regular;
  }
  }

  SYSTEMTIME lastAccessTime, lastWriteTime;
  Assert(FileTimeToSystemTime(&file_info.ftLastAccessTime, &lastAccessTime));
  Assert(FileTimeToSystemTime(&file_info.ftLastWriteTime, &lastWriteTime));

  properties.last_access_time =
    w32_time64_from_system_time(&lastAccessTime);
  properties.last_modification_time = properties.last_status_change_time =
      w32_time64_from_system_time(&lastWriteTime);


  return properties;
}

fn String8 os_fs_path_from_handle(Arena *arena, OS_Handle handle) {
  String8 res = {};
  res.str = arena_push_many(arena, u8, MAX_PATH);
  res.size = GetFinalPathNameByHandleA(((W32_Primitive*)handle.h[0])->file.handle,
                                       (LPSTR)res.str,
                                       MAX_PATH, FILE_NAME_NORMALIZED);
  arena_pop(arena, MAX_PATH - res.size);
  if (res.str[0] == '\\') { // TODO(lb): not sure if its guaranteed to be here
    res.str += 4; // skips the `\\?\`
    res.size -= 4;
  }
  return res;
}

fn String8 os_fs_readlink(Arena *arena, String8 path) {
  String8 res = {};
  res.str = arena_push_many(arena, u8, MAX_PATH);
  Scratch scratch = ScratchBegin(&arena, 1);
  HANDLE pathfd = CreateFileA(cstr_from_str8(scratch.arena, path),
                              GENERIC_READ, FILE_SHARE_READ, 0,
                              OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
  res.size = GetFinalPathNameByHandleA(pathfd, (LPSTR)res.str,
                                       MAX_PATH, FILE_NAME_NORMALIZED);
  CloseHandle(pathfd);
  ScratchEnd(scratch);
  arena_pop(arena, MAX_PATH - res.size);
  if (res.str[0] == '\\') { // TODO(lb): not sure if its guaranteed to be here
    res.str += 4; // skips the `\\?\`
    res.size -= 4;
  }
  return res;
}

// =============================================================================
// Memory mapping files
fn File os_fs_fopen(Arena *arena, OS_Handle file) {
  File result = {0};
  W32_Primitive *prim = (W32_Primitive*)file.h[0];
  Assert(prim);
  result.file_handle.h[0] = (u64)prim;

  BY_HANDLE_FILE_INFORMATION file_info;
  Assert(GetFileInformationByHandle((HANDLE)prim->file.handle, &file_info));

  SECURITY_DESCRIPTOR *security = 0;
  Assert(GetSecurityInfo((HANDLE)prim->file.handle, SE_FILE_OBJECT,
                         DACL_SECURITY_INFORMATION,
                         0, 0, 0, 0, (PSECURITY_DESCRIPTOR *)&security)
         == ERROR_SUCCESS);

  char path[MAX_PATH];
  result.path.size = GetFinalPathNameByHandleA((HANDLE)prim->file.handle, path,
                                               MAX_PATH, FILE_NAME_NORMALIZED);
  result.path.str = arena_push_many(arena, u8, result.path.size);
  memcopy(result.path.str, path, result.path.size);
  if (result.path.str[0] == '\\') {
    result.path.str += 4;
    result.path.size -= 4;
  }

  DWORD access_flags = 0;
  switch (prim->file.flags) {
  case OS_acfRead: {
    access_flags = PAGE_READONLY;
  } break;
  case OS_acfWrite:
  case OS_acfRead | OS_acfWrite: {
    access_flags = PAGE_READWRITE;
  } break;
  case OS_acfExecute:
  case OS_acfRead | OS_acfExecute: {
    access_flags = PAGE_EXECUTE_READ;
  } break;
  case OS_acfWrite | OS_acfExecute:
  case OS_acfRead | OS_acfWrite | OS_acfExecute: {
    access_flags = PAGE_EXECUTE_READWRITE;
  } break;
  }
  result.mmap_handle.h[0] = (u64)CreateFileMappingA((HANDLE)prim->file.handle, 0,
                                                    access_flags, 0, 0, 0);

  access_flags = 0;
  switch (prim->file.flags) {
  case OS_acfRead: {
    access_flags = FILE_MAP_READ;
  } break;
  case OS_acfWrite:
  case OS_acfRead | OS_acfWrite: {
    access_flags = FILE_MAP_WRITE;
  } break;
  case OS_acfExecute:
  case OS_acfRead | OS_acfExecute: {
    access_flags = FILE_MAP_READ | FILE_MAP_EXECUTE;
  } break;
  case OS_acfWrite | OS_acfExecute:
  case OS_acfRead | OS_acfWrite | OS_acfExecute: {
    access_flags = FILE_MAP_ALL_ACCESS;
  } break;
  }
  result.content = (u8*)MapViewOfFile((HANDLE)result.mmap_handle.h[0],
                                      access_flags, 0, 0, 0);

  result.prop = os_fs_get_properties(file);
  return result;
}

fn File os_fs_fopen_tmp(Arena *arena) {
  char tmpdir_path[MAX_PATH] = {};
  isize tmpdir_path_size = GetTempPath2A(MAX_PATH, tmpdir_path);
  Assert(tmpdir_path_size > 0 && tmpdir_path_size < MAX_PATH);

  char tmpfile_name[MAX_PATH] = {};
  Assert(!GetTempFileNameA(tmpdir_path, APPLICATION_NAME, true, tmpfile_name));

  String8 tmpfile = {};
  tmpfile.size = strlen(tmpfile_name);
  tmpfile.str = arena_push_many(arena, u8, tmpfile.size);
  memcopy(tmpfile.str, tmpfile_name, tmpfile.size);
  return os_fs_fopen(arena, os_fs_open(tmpfile, OS_acfRead | OS_acfWrite));
}

fn bool os_fs_fclose(File *file) {
  return UnmapViewOfFile(file->content) &&
         CloseHandle((HANDLE)((W32_Primitive*)file->file_handle.h[0])->file.handle) &&
         CloseHandle((HANDLE)file->mmap_handle.h[0]);
}

fn bool os_fs_fresize(File *file, isize size) {
  Assert(size >= 0);
  W32_Primitive *prim = (W32_Primitive*)file->file_handle.h[0];
  LARGE_INTEGER lsize = { .QuadPart = (LONGLONG)size };
  if (!SetFilePointerEx(prim->file.handle, lsize, 0, FILE_BEGIN)) {
    return false;
  }

  file->prop.size = size;
  if (!SetEndOfFile(prim->file.handle)) {
    return false;
  }

  UnmapViewOfFile(file->content);
  CloseHandle((HANDLE)file->mmap_handle.h[0]);

  DWORD access_flags = 0;
  switch (prim->file.flags) {
  case OS_acfRead: {
    access_flags = PAGE_READONLY;
  } break;
  case OS_acfWrite:
  case OS_acfRead | OS_acfWrite: {
    access_flags = PAGE_READWRITE;
  } break;
  case OS_acfExecute:
  case OS_acfRead | OS_acfExecute: {
    access_flags = PAGE_EXECUTE_READ;
  } break;
  case OS_acfWrite | OS_acfExecute:
  case OS_acfRead | OS_acfWrite | OS_acfExecute: {
    access_flags = PAGE_EXECUTE_READWRITE;
  } break;
  }
  file->mmap_handle.h[0] = (u64)CreateFileMappingA((HANDLE)prim->file.handle, 0,
                                                   access_flags, 0, 0, 0);
  access_flags = 0;
  switch (prim->file.flags) {
  case OS_acfRead: {
    access_flags = FILE_MAP_READ;
  } break;
  case OS_acfWrite:
  case OS_acfRead | OS_acfWrite: {
    access_flags = FILE_MAP_WRITE;
  } break;
  case OS_acfExecute:
  case OS_acfRead | OS_acfExecute: {
    access_flags = FILE_MAP_READ | FILE_MAP_EXECUTE;
  } break;
  case OS_acfWrite | OS_acfExecute:
  case OS_acfRead | OS_acfWrite | OS_acfExecute: {
    access_flags = FILE_MAP_ALL_ACCESS;
  } break;
  }
  return (file->content = (u8*)MapViewOfFile((HANDLE)file->mmap_handle.h[0],
                                             access_flags, 0, 0, size)) != 0;
}

fn bool os_fs_file_has_changed(File *file) {
  FILETIME new_mtime = {};
  Scratch scratch = ScratchBegin(0, 0);
  HANDLE new_handle = CreateFileA(cstr_from_str8(scratch.arena, file->path),
                                  GENERIC_READ, FILE_SHARE_READ,
                                  0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  ScratchEnd(scratch);
  Assert(new_handle != INVALID_HANDLE_VALUE &&
         GetFileTime(new_handle, 0, 0, &new_mtime));
  CloseHandle(new_handle);

  SYSTEMTIME new_mtime_systime = {};
  FileTimeToSystemTime(&new_mtime, &new_mtime_systime);
  time64 mtime = w32_time64_from_system_time(&new_mtime_systime);
  return mtime > file->prop.last_modification_time;
}

fn bool os_fs_fdelete(File *file) {
  Scratch scratch = ScratchBegin(0, 0);
  bool res = os_fs_fclose(file) &&
    !_unlink(cstr_from_str8(scratch.arena, file->path));
  ScratchEnd(scratch);
  return res;
}

fn bool os_fs_frename(File *file, String8 to) {
  Scratch scratch = ScratchBegin(0, 0);
  bool res = MoveFileEx(cstr_from_str8(scratch.arena, file->path),
                        cstr_from_str8(scratch.arena, to),
                        MOVEFILE_REPLACE_EXISTING |
                        MOVEFILE_WRITE_THROUGH);
  ScratchEnd(scratch);
  return res;
}

// =============================================================================
// Misc operation on the filesystem
fn bool os_fs_delete(String8 filepath) {
  Scratch scratch = ScratchBegin(0, 0);
  bool res = DeleteFileA(cstr_from_str8(scratch.arena, filepath));
  ScratchEnd(scratch);
  return res;
}

fn bool os_fs_rename(String8 filepath, String8 to) {
  Scratch scratch = ScratchBegin(0, 0);
  bool res = MoveFileEx(cstr_from_str8(scratch.arena, filepath),
                        cstr_from_str8(scratch.arena, to),
                        MOVEFILE_REPLACE_EXISTING |
                        MOVEFILE_WRITE_THROUGH);
  ScratchEnd(scratch);
  return res;
}

fn bool os_fs_mkdir(String8 path) {
  Scratch scratch = ScratchBegin(0, 0);
  bool res = _mkdir(cstr_from_str8(scratch.arena, path));
  ScratchEnd(scratch);
  return res;
}

fn bool os_fs_rmdir(String8 path) {
  Scratch scratch = ScratchBegin(0, 0);
  bool res = _rmdir(cstr_from_str8(scratch.arena, path));
  ScratchEnd(scratch);
  return res;
}

fn String8 os_fs_filename_from_path(Arena *arena, String8 path) {
  String8 res = {};
  Scratch scratch = ScratchBegin(0, 0);
  StringBuilder ss = str8_split(scratch.arena, path, '/');

  usize last_dot = 0;
  for (usize i = 0; i < ss.last->value.size; ++i) {
    if (ss.last->value.str[i] == '.') { last_dot = i; }
  }
  res.size = last_dot;
  res.str = arena_push_many(arena, u8, res.size);
  memcopy(res.str, ss.last->value.str, res.size);
  ScratchEnd(scratch);
  return res;
}

////////////////////////////////
//- km: File iterator
fn OS_FileIter* os_fs_iter_begin(Arena *arena, String8 path) {
  Scratch scratch = ScratchBegin(&arena, 1);
  StringBuilder list = {0};

  strstream_append_str(scratch.arena, &list, path);
  strstream_append_str(scratch.arena, &list, Strlit("\\*"));
  path = str8_from_stream(scratch.arena, list);

  String16 path16 = str16_from_str8(scratch.arena, path);
  WIN32_FIND_DATAW file_data = {0};

  W32_FileIter *iter = arena_push(arena, W32_FileIter);
  iter->handle = FindFirstFileW((WCHAR*)path16.str, &file_data);
  iter->file_data = file_data;

  OS_FileIter *result = arena_push(arena, OS_FileIter);
  memcopy(result->memory, iter, sizeof(W32_FileIter));
  return result;
}

fn bool os_fs_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out) {
  W32_FileIter *w32_iter = (W32_FileIter*)iter->memory;
  for (;!w32_iter->done;) {
    WCHAR *file_name = w32_iter->file_data.cFileName;
    bool is_dot = file_name[0] == '.';
    bool is_dotdot = file_name[0] == '.' && file_name[1] == '.';

    bool valid = (!is_dot && !is_dotdot);
    WIN32_FIND_DATAW data;
    if (valid) {
      memcopy(&data, &w32_iter->file_data, sizeof(data));
    }
    if (!FindNextFileW(w32_iter->handle, &w32_iter->file_data)) {
      w32_iter->done = 1;
    }

    if (!valid) { continue; }
    info_out->name = str8_from_str16(arena, str16_from_cstr16((u16*)data.cFileName));

    OS_Handle file = os_fs_open(info_out->name, OS_acfRead);
    info_out->properties = os_fs_get_properties(file);
    os_fs_close(file);
    if (iter->filter_allowed &&
        !(info_out->properties.type & iter->filter_allowed)) {
      continue;
    }
    return true;
  }

  return false;
}

fn void os_fs_iter_end(OS_FileIter *iter) {
  W32_FileIter *w32_iter = (W32_FileIter*)iter->memory;
  FindClose(w32_iter->handle);
}

// =============================================================================
// Debugger communication func
fn void dbg_print(const char *fmt, ...) {
  va_list args;
  Scratch scratch = ScratchBegin(0, 0);
  va_start(args, fmt);
  String8 msg = _str8_format(scratch.arena, fmt, args);
  OutputDebugStringA(cstr_from_str8(scratch.arena, msg));
  OutputDebugStringA("\n");
  va_end(args);
  ScratchEnd(scratch);
}

// =============================================================================
// Win32 specific
fn DWORD w32_thread_entry_point(void *ptr) {
  W32_Primitive *primitive = (W32_Primitive*)ptr;
  Func_Thread *func = primitive->thread.func;
  func(primitive->thread.arg);
  return 0;
}

fn W32_Primitive* w32_primitive_alloc(W32_PrimitiveType kind) {
  EnterCriticalSection(&w32_state.mutex);
  W32_Primitive *result = w32_state.free_list;
  if (result) {
    StackPop(w32_state.free_list);
    memzero(result, sizeof(*result));
  } else {
    result = arena_push(w32_state.arena, W32_Primitive);
  }
  result->kind = kind;
  LeaveCriticalSection(&w32_state.mutex);
  return result;
}

fn void w32_primitive_release(W32_Primitive *primitive) {
  primitive->kind = W32_Primitive_Nil;
  EnterCriticalSection(&w32_state.mutex);
  StackPush(w32_state.free_list, primitive);
  LeaveCriticalSection(&w32_state.mutex);
}

fn DateTime w32_date_time_from_system_time(SYSTEMTIME* in) {
  DateTime result = {0};
  result.year = in->wYear;
  result.month  = (u8)in->wMonth - 1;
  result.day    = (u8)in->wDay - 1;
  result.hour = (u8)in->wHour;
  result.minute = (u8)in->wMinute;
  result.second = (u8)in->wSecond;
  result.ms     = (u16)in->wMilliseconds;
  return result;
}

fn time64 w32_time64_from_system_time(SYSTEMTIME* in) {
  i16 year = in->wYear;
  time64 res = (year >= 0 ? 1ULL << 63 : 0);
  res |= (u64)((year >= 0 ? year : -year) & ~(1 << 27)) << 36;
  res |= (u64)(in->wMonth) << 32;
  res |= (u64)(in->wDay) << 27;
  res |= (u64)(in->wHour) << 22;
  res |= (u64)(in->wMinute) << 16;
  res |= (u64)(in->wSecond) << 10;
  res |= (u64)in->wMilliseconds;
  return res;
}

fn SYSTEMTIME w32_system_time_from_date_time(DateTime *in) {
  SYSTEMTIME result = {0};
  result.wYear = (WORD)in->year;
  result.wMonth = in->month + 1;
  result.wDay = in->day + 1;
  result.wHour = in->hour;
  result.wMinute = in->minute;
  result.wSecond = in->second;
  result.wMilliseconds = in->ms;
  return result;
}

fn SYSTEMTIME w32_system_time_from_time64(time64 in) {
  SYSTEMTIME res = {0};
  i16 year = (i16)((in >> 36) & ~(1 << 27)  * (in >> 63 ? 1 : -1));
  if (year < 1601 || year > 30827) { return res; }

  res.wYear         = year;
  res.wMonth        = (WORD)((in >> 32) & bitmask4);
  res.wDay          = (WORD)((in >> 27) & bitmask5);
  res.wHour         = (WORD)((in >> 22) & bitmask5);
  res.wMinute       = (WORD)((in >> 16) & bitmask6);
  res.wSecond       = (WORD)((in >> 10) & bitmask6);
  res.wMilliseconds = (WORD)(in & bitmask10);
  return res;
}

fn void os_env_setup(void) {
  SYSTEM_INFO sys_info = {0};
  GetSystemInfo(&sys_info);

  w32_state.info.core_count = (u8)sys_info.dwNumberOfProcessors;
  w32_state.info.page_size = sys_info.dwPageSize;
  w32_state.info.hugepage_size = GetLargePageMinimum();

  w32_state.arena = arena_build(.reserve_size = GB(1));
  InitializeCriticalSection(&w32_state.mutex);
  QueryPerformanceFrequency(&w32_state.perf_freq);

  w32_state.thread_list.mutex = os_mutex_alloc();

  WSADATA wsa_data;
  WSAStartup(MAKEWORD(2, 2), &wsa_data);
}

#ifndef CBUILD_H
fn void w32_call_entrypoint(int argc, WCHAR **wargv) {
  Arena *args_arena = arena_build();
  CmdLine *cmdln = arena_push(args_arena, CmdLine);
  cmdln->count = argc - 1;
  if (cmdln->count) {
    cmdln->exe = str8_from_str16(args_arena, str16_from_cstr16((u16*)wargv[0]));
  }
  for (int i = 1; i < argc; ++i) {
    cmdln->args[i - 1] = str8_from_str16(args_arena,
                                         str16_from_cstr16((u16*)wargv[i]));
  }
  start(cmdln);
  DeleteCriticalSection(&w32_state.mutex);
}

#  if OS_GUI
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance,
                    PWSTR cmdln, int cmd_show) {
  os_env_setup();
  w32_gfx_init(instance);
  w32_call_entrypoint(__argc, (WCHAR **)__argv);
  return 0;
}
#  else
int wmain(int argc, WCHAR **argv) {
  os_env_setup();
  w32_call_entrypoint(argc, argv);
  return 0;
}
#  endif
#endif
