fn OS_SystemInfo*
os_getSystemInfo(void)
{
  return &w32_state.info;
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
  void *result = VirtualAlloc((void*)base_addr, size, MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE);
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
  HANDLE handle = CreateThread(0, 0, os_w32_thread_entry_point, primitive, 0, &primitive->thread.tid);
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
  char *symbol_cstr = strToCstr(scratch.arena, symbol);
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
  if(flags & OS_acfWrite) { access_flags |= GENERIC_WRITE; creation_disposition = CREATE_ALWAYS; }
  if(flags & OS_acfExecute) { access_flags |= GENERIC_EXECUTE; }
  if(flags & OS_acfAppend) { creation_disposition = OPEN_ALWAYS; access_flags |= FILE_APPEND_DATA; }
  if(flags & OS_acfShareRead) { share_mode |= FILE_SHARE_READ; }
  if(flags & OS_acfShareWrite) { share_mode |= FILE_SHARE_WRITE; }

  HANDLE file = CreateFileW((WCHAR*)path.str, access_flags, share_mode,
                            &security_attributes, creation_disposition, FILE_ATTRIBUTE_NORMAL, 0);
  if(file != INVALID_HANDLE_VALUE) {
    result.h[0] = (u64)file;
  }

  ScratchEnd(scratch);
  return result;
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
  // TODO(km): need to fill owner,group, and permission flags
  FS_Properties properties = {0};

  return properties;
}

fn File fs_fileOpen(Arena *arena, OS_Handle file) {
  File result = {0};

  return result;
}

////////////////////////////////
//- km: File iterator

fn OS_FileIter*
fs_iter_begin(Arena *arena, String8 path)
{
  Scratch scratch = ScratchBegin(&arena, 1);
  StringStream list = {0};

  stringstreamAppend(scratch.arena, &list, path);
  stringstreamAppend(scratch.arena, &list, Strlit("\\*"));
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
