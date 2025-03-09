#include <aclapi.h>
#pragma comment(lib, "Advapi32.lib")

fn OS_SystemInfo*
os_getSystemInfo(void) {
  return &w32_state.info;
}

fn DateTime
os_w32_date_time_from_system_time(SYSTEMTIME* in)
{
  DateTime result = {0};
  result.year = in->wYear;
  result.month = in->wMonth - 1;
  result.day = in->wDay - 1;
  result.hour = (u8)in->wHour;
  result.minute = (u8)in->wMinute;
  result.second = (u8)in->wSecond;
  result.ms = (u16)in->wMilliseconds;
  return result;
}

fn time64
os_w32_time64_from_system_time(SYSTEMTIME* in) {
  i16 year = in->wYear;
  time64 res = (year >= 0 ? 1ULL << 63 : 0);
  res |= (u64)((year >= 0 ? year : -year) & ~(1 << 27)) << 36;
  res |= (u64)(in->wMonth) << 32;
  res |= (u64)(in->wDay) << 27;
  res |= (in->wHour) << 22;
  res |= (in->wMinute) << 16;
  res |= (in->wSecond) << 10;
  res |= in->wMilliseconds;
  return res;
}

fn SYSTEMTIME
os_w32_system_time_from_date_time(DateTime *in)
{
  SYSTEMTIME result = {0};
  result.wYear = in->year;
  result.wMonth = in->month + 1;
  result.wDay = in->day + 1;
  result.wHour = in->hour;
  result.wMinute = in->minute;
  result.wSecond = in->second;
  result.wMilliseconds = in->ms;
  return result;
}

fn SYSTEMTIME
os_w32_system_time_from_time64(time64 in) {
  SYSTEMTIME res = {0};
  i16 year = (in >> 36) & ~(1 << 27)  * (in >> 63 ? 1 : -1);
  if (year < 1601 || year > 30827) { return res; }

  res.wYear = year;
  res.wMonth = (in >> 32) & bitmask4;
  res.wDay = (in >> 27) & bitmask5;
  res.wHour = (in >> 22) & bitmask5;
  res.wMinute = (in >> 16) & bitmask6;
  res.wSecond = (in >> 10) & bitmask6;
  res.wMilliseconds = in & bitmask10;
  return res;
}

fn time64 os_local_now() {
  SYSTEMTIME systime;
  GetLocalTime(&systime);
  return os_w32_time64_from_system_time(&systime);
}

fn DateTime os_local_dateTimeNow() {
  SYSTEMTIME system_time;
  GetLocalTime(&system_time);
  DateTime result = os_w32_date_time_from_system_time(&system_time);
  return result;
}

fn time64 os_local_fromUTCTime64(time64 in) {
  SYSTEMTIME utc = {0};
  SYSTEMTIME local_time = os_w32_system_time_from_time64(in);
  SystemTimeToTzSpecificLocalTime(0, &local_time, &utc);
  return os_w32_time64_from_system_time(&utc);
}

fn DateTime
os_local_fromUTCDateTime(DateTime *in)
{
  SYSTEMTIME systime = os_w32_system_time_from_date_time(in);
  FILETIME ftime;
  SystemTimeToFileTime(&systime, &ftime);
  FILETIME ftime_local;
  FileTimeToLocalFileTime(&ftime, &ftime_local);
  FileTimeToSystemTime(&ftime_local, &systime);
  DateTime result = os_w32_date_time_from_system_time(&systime);
  return result;
}


fn time64
os_utc_now() {
  SYSTEMTIME system_time;
  GetSystemTime(&system_time);
  return os_w32_time64_from_system_time(&system_time);
}

fn DateTime
os_utc_dateTimeNow()
{
  SYSTEMTIME system_time;
  GetSystemTime(&system_time);
  DateTime result = os_w32_date_time_from_system_time(&system_time);
  return result;
}

fn time64
os_utc_localizedTime64(i8 utc_offset) {
  time64 now = os_utc_now();
  i32 year = ((now >> 36) & ~(1 << 27));
  i8 month = (now >> 32) & bitmask4;
  i8 day = (now >> 27) & bitmask5;
  i8 hour = ((now >> 22) & bitmask5) + utc_offset;

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

fn time64
os_utc_fromLocalTime64(time64 in) {
  SYSTEMTIME utctime = {0};
  SYSTEMTIME localtime = os_w32_system_time_from_time64(in);
  TzSpecificLocalTimeToSystemTime(NULL, &localtime, &utctime);
  return os_w32_time64_from_system_time(&utctime);
}

fn DateTime
os_utc_fromLocalDateTime(DateTime *in) {
  SYSTEMTIME utctime = {0};
  SYSTEMTIME localtime = os_w32_system_time_from_date_time(in);
  TzSpecificLocalTimeToSystemTime(NULL, &localtime, &utctime);
  return os_w32_date_time_from_system_time(&utctime);
}

fn void os_sleep_milliseconds(u32 ms) {
  Sleep(ms);
}

////////////////////////////////
//- km: memory

fn void*
os_reserve(usize base_addr, usize size)
{
  void *result = VirtualAlloc((void*)base_addr, size, MEM_RESERVE, PAGE_READWRITE);
  return result;
}

fn void*
os_reserveHuge(usize base_addr, usize size)
{
  void *result = VirtualAlloc((void*)base_addr, size,
			      MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE);
  return result;
}

fn void
os_release(void *base, usize size)
{
  BOOL result = VirtualFree(base, size, MEM_RELEASE);
  (void)result;
}

fn void
os_commit(void *base, usize size)
{
  void *result = VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE);
  (void)result;
}

fn void
os_decommit(void *base, usize size)
{
  BOOL result = VirtualFree(base, size, MEM_DECOMMIT);
  (void)result;
}

////////////////////////////////
//- km: Thread functions
fn OS_W32_Primitive*
os_w32_primitive_alloc(OS_W32_PrimitiveType kind)
{
  EnterCriticalSection(&w32_state.mutex);
  OS_W32_Primitive *result = w32_state.free_list;
  if(result)
  {
    StackPop(w32_state.free_list);
  }
  else
  {
    result = New(w32_state.arena, OS_W32_Primitive);

  }
  memset(result, 0, sizeof(*result));
  result->kind = kind;
  LeaveCriticalSection(&w32_state.mutex);
  return result;
}

fn void
os_w32_primitive_release(OS_W32_Primitive *primitive)
{
  primitive->kind = OS_W32_Primitive_Nil;
  EnterCriticalSection(&w32_state.mutex);
  StackPush(w32_state.free_list, primitive);
  LeaveCriticalSection(&w32_state.mutex);
}

fn OS_Handle
os_thread_start(ThreadFunc *func, void *arg)
{
  OS_W32_Primitive *primitive = os_w32_primitive_alloc(OS_W32_Primitive_Thread);
  HANDLE handle = CreateThread(0, 0, os_w32_thread_entry_point, primitive, 0,
			       &primitive->thread.tid);
  primitive->thread.func = func;
  primitive->thread.arg = arg;
  primitive->thread.handle = handle;
  OS_Handle result = {(u64)primitive};
  return result;
}

fn bool
os_thread_join(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  DWORD wait = WAIT_OBJECT_0;
  if(primitive)
  {
    wait = WaitForSingleObject(primitive->thread.handle, INFINITE);
    CloseHandle(primitive->thread.handle);
    os_w32_primitive_release(primitive);
  }
  return wait == WAIT_OBJECT_0;
}

fn void
os_thread_kill(OS_Handle thd)
{
  (void)0;
}

fn DWORD
os_w32_thread_entry_point(void *ptr)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)ptr;
  ThreadFunc *func = primitive->thread.func;
  func(primitive->thread.arg);
  return 0;
}

////////////////////////////////
//- km: critical section mutex

fn OS_Handle
os_mutex_alloc()
{
  OS_W32_Primitive *primitive = os_w32_primitive_alloc(OS_W32_Primitive_Mutex);
  InitializeCriticalSection(&primitive->mutex);
  OS_Handle result = {(u64)primitive};
  return result;
}

fn void
os_mutex_lock(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  EnterCriticalSection(&primitive->mutex);
}

fn bool
os_mutex_trylock(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  BOOL result = TryEnterCriticalSection(&primitive->mutex);
  return result;
}

fn void
os_mutex_unlock(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  LeaveCriticalSection(&primitive->mutex);
}

fn void
os_mutex_free(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  DeleteCriticalSection(&primitive->mutex);
  os_w32_primitive_release(primitive);
}

////////////////////////////////
//- km read/write mutexes

fn OS_Handle os_rwlock_alloc()
{
  OS_W32_Primitive *primitive = os_w32_primitive_alloc(OS_W32_Primitive_RWLock);
  InitializeSRWLock(&primitive->rw_mutex);
  OS_Handle result = {(u64)primitive};
  return result;
}

fn void
os_rwlock_read_lock(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  AcquireSRWLockShared(&primitive->rw_mutex);
}

fn bool
os_rwlock_read_trylock(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  BOOLEAN result = TryAcquireSRWLockShared(&primitive->rw_mutex);
  return result;
}

fn void
os_rwlock_read_unlock(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  ReleaseSRWLockShared(&primitive->rw_mutex);
}

fn void
os_rwlock_write_lock(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  AcquireSRWLockExclusive(&primitive->rw_mutex);
}

fn bool
os_rwlock_write_trylock(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  BOOLEAN result = TryAcquireSRWLockExclusive(&primitive->rw_mutex);
  return result;
}

fn void
os_rwlock_write_unlock(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  ReleaseSRWLockExclusive(&primitive->rw_mutex);
}

fn void
os_rwlock_free(OS_Handle handle)
{
  OS_W32_Primitive *primitive = (OS_W32_Primitive*)handle.h[0];
  os_w32_primitive_release(primitive);
}

fn OS_Handle os_cond_alloc() {
  OS_W32_Primitive *prim = os_w32_primitive_alloc(OS_W32_Primitive_CondVar);
  InitializeConditionVariable(&prim->condvar);
  OS_Handle res = {(u64)prim};
  return res;
}

fn void os_cond_signal(OS_Handle handle) {
  OS_W32_Primitive *prim = (OS_W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != OS_W32_Primitive_CondVar) { return; }
  WakeConditionVariable(&prim->condvar);
}

fn void os_cond_broadcast(OS_Handle handle) {
  OS_W32_Primitive *prim = (OS_W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != OS_W32_Primitive_CondVar) { return; }
  WakeAllConditionVariable(&prim->condvar);
}

fn bool os_cond_wait(OS_Handle cond_handle, OS_Handle mutex_handle,
		     u32 wait_at_most_microsec) {
  OS_W32_Primitive *condprim = (OS_W32_Primitive*)cond_handle.h[0];
  OS_W32_Primitive *mutexprim = (OS_W32_Primitive*)mutex_handle.h[0];
  if (!condprim || condprim->kind != OS_W32_Primitive_CondVar) { return false; }
  if (!mutexprim || mutexprim->kind != OS_W32_Primitive_Mutex) { return false; }
  return SleepConditionVariableCS(&condprim->condvar, &mutexprim->mutex,
				  (wait_at_most_microsec
				   ? wait_at_most_microsec / 1e3
				   : INFINITE)) != 0;
}

fn bool os_cond_waitrw_read(OS_Handle cond_handle, OS_Handle rwlock_handle,
			    u32 wait_at_most_microsec) {
  OS_W32_Primitive *condprim = (OS_W32_Primitive*)cond_handle.h[0];
  OS_W32_Primitive *rwlockprim = (OS_W32_Primitive*)rwlock_handle.h[0];
  if (!condprim || condprim->kind != OS_W32_Primitive_CondVar) { return false; }
  if (!rwlockprim || rwlockprim->kind != OS_W32_Primitive_RWLock)
    { return false; }
  return SleepConditionVariableSRW(&condprim->condvar, &rwlockprim->rw_mutex,
				   (wait_at_most_microsec
				    ? wait_at_most_microsec / 1e3
				    : INFINITE),
				   CONDITION_VARIABLE_LOCKMODE_SHARED) != 0;
}

fn bool os_cond_waitrw_write(OS_Handle cond_handle, OS_Handle rwlock_handle,
			     u32 wait_at_most_microsec) {
  OS_W32_Primitive *condprim = (OS_W32_Primitive*)cond_handle.h[0];
  OS_W32_Primitive *rwlockprim = (OS_W32_Primitive*)rwlock_handle.h[0];
  if (!condprim || condprim->kind != OS_W32_Primitive_CondVar) { return false; }
  if (!rwlockprim || rwlockprim->kind != OS_W32_Primitive_RWLock)
    { return false; }
  return SleepConditionVariableSRW(&condprim->condvar, &rwlockprim->rw_mutex,
				   (wait_at_most_microsec
				    ? wait_at_most_microsec / 1e3
				    : INFINITE),
				   CONDITION_VARIABLE_LOCKMODE_EXCLUSIVE) != 0;
}

fn bool os_cond_free(OS_Handle handle) {
  OS_W32_Primitive *prim = (OS_W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != OS_W32_Primitive_CondVar) { return false; }
  os_w32_primitive_release(prim);
  return true;
}

fn OS_Handle os_semaphore_alloc(OS_SemaphoreKind kind, u32 init_count,
				u32 max_count, String8 name) {
  OS_W32_Primitive *prim = os_w32_primitive_alloc(OS_W32_Primitive_Semaphore);

  Scratch scratch = ScratchBegin(0, 0);
  StringStream ss = {0};
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
				     cstrFromStr8(scratch.arena,
						  str8FromStream(scratch.arena,
								 ss)));
 skip_semname:
  ScratchEnd(scratch);
  OS_Handle res = {(u64)prim};
  return res;
}

fn bool os_semaphore_signal(OS_Handle handle) {
  OS_W32_Primitive *prim = (OS_W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != OS_W32_Primitive_Semaphore) { return false; }
  return ReleaseSemaphore(prim->semaphore, 1, 0);
}

fn bool os_semaphore_wait(OS_Handle handle, u32 wait_at_most_microsec) {
  OS_W32_Primitive *prim = (OS_W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != OS_W32_Primitive_Semaphore) { return false; }
  return WaitForSingleObject(prim->semaphore,
			     (wait_at_most_microsec
			      ? wait_at_most_microsec / 1e3
			      : INFINITE)) == WAIT_OBJECT_0;
}

fn bool os_semaphore_trywait(OS_Handle handle) {
  OS_W32_Primitive *prim = (OS_W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != OS_W32_Primitive_Semaphore) { return false; }
  return WaitForSingleObject(prim->semaphore, 0) == WAIT_OBJECT_0;
}

fn void os_semaphore_free(OS_Handle handle) {
  OS_W32_Primitive *prim = (OS_W32_Primitive*)handle.h[0];
  if (!prim || prim->kind != OS_W32_Primitive_Semaphore) { return; }
  CloseHandle(prim->semaphore);
  os_w32_primitive_release(prim);
}

fn SharedMem os_sharedmem_open(String8 name, usize size, OS_AccessFlags flags) {
  SharedMem res = {0};
  DWORD access_flags = 0;
  if(flags & OS_acfRead)    { access_flags |= GENERIC_READ;}
  if(flags & OS_acfWrite)   { access_flags |= GENERIC_WRITE; }
  if(flags & OS_acfExecute) { access_flags |= GENERIC_EXECUTE; }
  if(flags & OS_acfAppend)  { access_flags |= FILE_APPEND_DATA; }

  Scratch scratch = ScratchBegin(0, 0);
  StringStream ss = {0};
  strstream_append_str(scratch.arena, &ss, Strlit("Global\\\\"));
  strstream_append_str(scratch.arena, &ss, name);
  res.path = str8FromStream(scratch.arena, ss);
  res.mmap_handle.h[0] = (u64)
    CreateFileMapping(INVALID_HANDLE_VALUE, 0, access_flags,
		      (u32)((size >> 32) & bitmask32),
		      (u32)(size & bitmask32),
		      !name.size ? (char *)0 : cstrFromStr8(scratch.arena,
							    res.path));
  ScratchEnd(scratch);
  if (!res.mmap_handle.h[0]) { return res; }

  access_flags = 0;
  if(flags & OS_acfRead)    { access_flags |= FILE_MAP_READ;}
  if(flags & OS_acfWrite)   { access_flags |= FILE_MAP_WRITE; }
  if(flags & OS_acfExecute) { access_flags |= FILE_MAP_EXECUTE; }
  if(flags & OS_acfAppend)  { access_flags |= FILE_MAP_ALL_ACCESS; }
  res.content = (u8*)MapViewOfFile((HANDLE)res.mmap_handle.h[0], access_flags,
				   0, 0, size);
  if (!res.content) {
    CloseHandle((HANDLE)res.mmap_handle.h[0]);
    SharedMem _ = {0};
    return _;
  }

  return res;
}

fn bool os_sharedmem_close(SharedMem *shm) {
  return UnmapViewOfFile(shm->content) &&
	 CloseHandle((HANDLE)shm->mmap_handle.h[0]);
}

////////////////////////////////
//- km: Dynamic libraries

fn OS_Handle os_lib_open(String8 path){
  OS_Handle result = {0};
  Scratch scratch = ScratchBegin(0,0);
  String16 path16 = UTF16From8(scratch.arena, path);
  HMODULE module = LoadLibraryExW((WCHAR*)path16.str, 0, 0);
  if(module != 0){
    result.h[0] = (u64)module;
  }
  ScratchEnd(scratch);
  return result;
}

fn VoidFunc* os_lib_lookup(OS_Handle lib, String8 symbol){
  Scratch scratch = ScratchBegin(0,0);
  char *symbol_cstr = cstrFromStr8(scratch.arena, symbol);
  HMODULE module = (HMODULE)lib.h[0];
  VoidFunc *result = (VoidFunc*)GetProcAddress(module, symbol_cstr);
  ScratchEnd(scratch);
  return result;
}

fn i32 os_lib_close(OS_Handle lib){
  HMODULE module = (HMODULE)lib.h[0];
  BOOL result = FreeLibrary(module);
  return result;
}

// =============================================================================
// Misc
fn String8 os_currentDir(Arena *arena) {
  String8 res = {0};
  res.str = New(arena, u8, MAX_PATH);
  res.size = GetCurrentDirectoryA(MAX_PATH, (LPSTR)res.str);
  arenaPop(arena, MAX_PATH - res.size);
  return res;
}

// =============================================================================
// Networking
fn NetInterfaceList os_net_getInterfaces(Arena *arena) {
  NetInterfaceList res = {0};
  return res;
}

fn NetInterface os_net_interfaceFromStr8(String8 strip) {
  NetInterface res = {0};
  return res;
}

fn IP os_net_ipFromStr8(String8 strip) {
  IP res = {0};
  return res;
}

fn Socket os_net_socket_open(OS_Net_Transport protocol,
			     IP client, u16 client_port,
			     IP server, u16 server_port) {
  Socket res = {0};
  return res;
}

fn bool os_net_socket_close(Socket sock) { return false; }

////////////////////////////////
//- km: File operations

fn OS_Handle fs_open(String8 filepath, OS_AccessFlags flags) {
  OS_Handle result = {0};
  Scratch scratch = ScratchBegin(0, 0);
  String16 path = UTF16From8(scratch.arena, filepath);
  SECURITY_ATTRIBUTES security_attributes = {sizeof(SECURITY_ATTRIBUTES), 0, 0};
  DWORD access_flags = 0;
  DWORD share_mode = 0;
  DWORD creation_disposition = OPEN_EXISTING;

  if(flags & OS_acfRead) { access_flags |= GENERIC_READ;}
  if(flags & OS_acfWrite) {
    access_flags |= GENERIC_WRITE;
    creation_disposition = CREATE_ALWAYS;
  }
  if(flags & OS_acfExecute) { access_flags |= GENERIC_EXECUTE; }
  // TODO(lb): appending doesn't work
  if(flags & OS_acfAppend) {
    creation_disposition = OPEN_ALWAYS;
    access_flags |= FILE_APPEND_DATA;
  }
  if(flags & OS_acfShareRead) { share_mode |= FILE_SHARE_READ; }
  if(flags & OS_acfShareWrite) { share_mode |= FILE_SHARE_WRITE; }

  HANDLE file = CreateFileW((WCHAR*)path.str, access_flags, share_mode,
                            &security_attributes, creation_disposition,
			    FILE_ATTRIBUTE_NORMAL, 0);
  if(file != INVALID_HANDLE_VALUE) {
    result.h[0] = (u64)file;
  }

  ScratchEnd(scratch);
  return result;
}

fn bool fs_close(OS_Handle fd) { return false; }

// TODO(lb): i don't know if there are some location on the Windows
//           FS that contain files with size=0.
fn String8 fs_readVirtual(Arena *arena, OS_Handle file, usize size) {
  return fs_read(arena, file);
}

fn String8 fs_read(Arena *arena, OS_Handle file) {
  String8 result = {0};

  if(file.h[0] == 0) { return result; }

  LARGE_INTEGER file_size = {0};
  HANDLE handle = (HANDLE)file.h[0];

  GetFileSizeEx(handle, &file_size);
  u64 to_read = file_size.QuadPart;
  u64 total_read = 0;
  u8 *ptr = New(arena, u8, to_read);

  for(;total_read < to_read;)
  {
    u64 amount64 = to_read - total_read;
    u32 amount32 = amount64 > U32_MAX ? U32_MAX : (u32)amount64;
    OVERLAPPED overlapped = {0};
    DWORD bytes_read = 0;
    ReadFile(handle, ptr + total_read, amount32, &bytes_read, &overlapped);
    total_read += bytes_read;
    if(bytes_read != amount32)
    {
      break;
    }
  }

  if(total_read == to_read)
  {
    result.str = ptr;
    result.size = to_read;
  }

  return result;
}

fn bool fs_write(OS_Handle file, String8 content) {
  bool result = false;

  if(file.h[0] == 0) { return result; }
  HANDLE handle = (HANDLE)file.h[0];

  u64 to_write = content.size;
  u64 total_write = 0;
  u8 *ptr = content.str;
  for(;total_write < to_write;)
  {
    u64 amount64 = to_write - total_write;;
    u32 amount32 = amount64 > U32_MAX ? U32_MAX : (u32)amount64;
    DWORD bytes_written = 0;
    WriteFile(handle, ptr + total_write, amount32, &bytes_written, 0);
    total_write += bytes_written;
    if(bytes_written != amount32)
    {
      break;
    }
  }

  if(total_write == to_write)
  {
    result = true;
  }

  return result;
}

fn FS_Properties fs_getProp(OS_Handle file) {
  FS_Properties properties = {0};

  SECURITY_DESCRIPTOR *security = 0;
  SID *owner = 0;
  SID *group = 0;

  AssertMsg(GetSecurityInfo((HANDLE)file.h[0], SE_FILE_OBJECT,
			    OWNER_SECURITY_INFORMATION |
			    GROUP_SECURITY_INFORMATION,
			    (SID**)&properties.ownerID,
			    (SID**)&properties.groupID, 0, 0,
			    &security) == ERROR_SUCCESS,
	    Strlit("GetSecurityInfo"));

  BY_HANDLE_FILE_INFORMATION file_info;
  Assert(GetFileInformationByHandle((HANDLE)file.h[0], &file_info));

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
    os_w32_time64_from_system_time(&lastAccessTime);
  properties.last_modification_time = properties.last_status_change_time =
      os_w32_time64_from_system_time(&lastWriteTime);


  return properties;
}

fn String8 fs_pathFromHandle(Arena *arena, OS_Handle handle) {
  String8 res = {0};
  res.str = New(arena, u8, MAX_PATH);
  res.size = GetFinalPathNameByHandleA((HANDLE)handle.h[0], (LPSTR)res.str,
				       MAX_PATH, FILE_NAME_NORMALIZED);
  arenaPop(arena, MAX_PATH - res.size);
  if (res.str[0] == '\\') { // TODO(lb): not sure if its guaranteed to be here
    res.str += 4; // skips the `\\?\`
    res.size -= 4;
  }
  return res;
}

fn String8 fs_readlink(Arena *arena, String8 path) {
  String8 res = {0};
  res.str = New(arena, u8, MAX_PATH);
  Scratch scratch = ScratchBegin(&arena, 1);
  HANDLE pathfd = CreateFileA(cstrFromStr8(scratch.arena, path),
			      GENERIC_READ, FILE_SHARE_READ, 0,
			      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
  res.size = GetFinalPathNameByHandleA(pathfd, (LPSTR)res.str,
				       MAX_PATH, FILE_NAME_NORMALIZED);
  CloseHandle(pathfd);
  ScratchEnd(scratch);
  arenaPop(arena, MAX_PATH - res.size);
  if (res.str[0] == '\\') { // TODO(lb): not sure if its guaranteed to be here
    res.str += 4; // skips the `\\?\`
    res.size -= 4;
  }
  return res;
}

// =============================================================================
// Memory mapping files
fn File fs_fopen(Arena *arena, OS_Handle file) {
  File result = {0};
  result.file_handle = file;

  BY_HANDLE_FILE_INFORMATION file_info;
  Assert(GetFileInformationByHandle((HANDLE)file.h[0], &file_info));

  SECURITY_DESCRIPTOR *security = 0;
  Assert(GetSecurityInfo((HANDLE)file.h[0], SE_FILE_OBJECT,
			 DACL_SECURITY_INFORMATION,
			 0, 0, 0, 0, &security) == ERROR_SUCCESS);

  char path[MAX_PATH];
  result.path.size = GetFinalPathNameByHandleA((HANDLE)file.h[0], path,
					       MAX_PATH, FILE_NAME_NORMALIZED);
  result.path.str = New(arena, u8, result.path.size);
  memCopy(result.path.str, path, result.path.size);
  if (result.path.str[0] == '\\') {
    result.path.str += 4;
    result.path.size -= 4;
  }

  SECURITY_ATTRIBUTES security_attrib;
  security_attrib.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attrib.lpSecurityDescriptor = security;
  security_attrib.bInheritHandle = FALSE;

  // TODO(lb): change PAGE_READONLY to whatever mode `file` was opened with
  result.mmap_handle.h[0] = (u64)
    CreateFileMapping((HANDLE)file.h[0], 0,
		      PAGE_READONLY, file_info.nFileSizeHigh,
		      file_info.nFileSizeLow, 0);
  AssertMsg(result.mmap_handle.h[0], Strlit("CreateFileMapping"));

  // TODO(lb): change FILE_MAP_READ to whatever mode `file` was opened with
  result.content = (u8*)MapViewOfFile((HANDLE)result.mmap_handle.h[0],
				      FILE_MAP_READ, 0, 0, 0);
  AssertMsg(result.content, Strlit("MapViewOfFile"));

  result.prop = fs_getProp(file);

  return result;
}

fn File fs_fopenTmp(Arena *arena) {
  File result = {0};
  return result;
}

inline fn bool fs_fclose(File *file) {
  return false;
}

inline fn bool fs_fresize(File *file, usize size) {
  return false;
}

inline fn void fs_fwrite(File *file, String8 str) {}

inline fn bool fs_fileHasChanged(File *file) {
  return false;
}

inline fn bool fs_fdelete(File *file) {
  return false;
}

inline fn bool fs_frename(File *file, String8 to) {
  return false;
}

// =============================================================================
// Misc operation on the filesystem
inline fn bool fs_delete(String8 filepath) {
  return false;
}

inline fn bool fs_rename(String8 filepath, String8 to) {
  return false;
}

inline fn bool fs_mkdir(String8 path) {
  return false;
}

inline fn bool fs_rmdir(String8 path) {
  return false;
}

////////////////////////////////
//- km: File iterator

fn OS_FileIter*
fs_iter_begin(Arena *arena, String8 path)
{
  Scratch scratch = ScratchBegin(&arena, 1);
  StringStream list = {0};

  strstream_append_str(scratch.arena, &list, path);
  strstream_append_str(scratch.arena, &list, Strlit("\\*"));
  path = str8FromStream(scratch.arena, list);

  String16 path16 = UTF16From8(scratch.arena, path);
  WIN32_FIND_DATAW file_data = {0};

  OS_W32_FileIter *iter = New(arena, OS_W32_FileIter);
  iter->handle = FindFirstFileW((WCHAR*)path16.str, &file_data);
  iter->file_data = file_data;

  OS_FileIter *result = New(arena, OS_FileIter);
  memcpy(result->memory, iter, sizeof(OS_W32_FileIter));
  return result;
}

fn bool
fs_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out)
{
  bool result = false;
  OS_W32_FileIter *w32_iter = (OS_W32_FileIter*)iter->memory;
  for(;!w32_iter->done;)
  {
    WCHAR *file_name = w32_iter->file_data.cFileName;
    bool is_dot = file_name[0] == '.';
    bool is_dotdot = file_name[0] == '.' && file_name[1] == '.';

    bool valid = (!is_dot && !is_dotdot);
    WIN32_FIND_DATAW data;
    if(valid)
    {
      memcpy(&data, &w32_iter->file_data, sizeof(data));
    }

    if(!FindNextFileW(w32_iter->handle, &w32_iter->file_data))
    {
      w32_iter->done = 1;
    }

    if(valid)
    {
      info_out->name = UTF8From16(arena, str16_cstr((u16*)data.cFileName));
      result = true;
      break;
    }
  }

  return result;
}

fn void
fs_iter_end(OS_FileIter *iter)
{
  OS_W32_FileIter *w32_iter = (OS_W32_FileIter*)iter->memory;
  FindClose(w32_iter->handle);
}

////////////////////////////////
//- km: Entry point

fn void
w32_entry_point_caller(int argc, WCHAR **wargv)
{
  SYSTEM_INFO sys_info = {0};
  GetSystemInfo(&sys_info);

  w32_state.info.core_count = (u8)sys_info.dwNumberOfProcessors;
  w32_state.info.page_size = sys_info.dwPageSize;
  w32_state.info.hugepage_size = GetLargePageMinimum();

  w32_state.arena = ArenaBuild(.reserve_size = GB(1));
  InitializeCriticalSection(&w32_state.mutex);

  Arena *args_arena = ArenaBuild();
  CmdLine *cmdln = New(args_arena, CmdLine);
  cmdln->count = argc - 1;
  cmdln->exe = UTF8From16(args_arena, str16_cstr((u16*)wargv[0]));
  for(int i = 1; i < argc; ++i)
  {
    cmdln->args[i - 1] = UTF8From16(args_arena, str16_cstr((u16*)wargv[i]));
  }

  start(cmdln);
  DeleteCriticalSection(&w32_state.mutex);
}

#if BUILD_CONSOLE_INTEFACE
int
wmain(int argc, WCHAR **argv)
{
  w32_entry_point_caller(argc, argv);
  return 0;
}
#else
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmdln, int cmd_show)
{
  w32_entry_point_caller(__argc, __wargv);
  return 0;
}
#endif
