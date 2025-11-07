OS_Web_State os_web_state = {0};

// =============================================================================
// System information retrieval
fn OS_SystemInfo *os_sysinfo(void) {
  return &os_web_state.info;
}

// =============================================================================
// Date&Time stuff: implemented per OS
fn time64 os_local_now(void) { return (time64) {0}; }
fn DateTime os_local_dateTimeNow(void) { return (DateTime) {0}; }
fn time64 os_local_fromUTCTime64(time64 t) { return (time64) {0}; }
fn DateTime os_local_fromUTCDateTime(DateTime *dt) { return (DateTime) {0}; }
fn time64 os_utc_now(void) { return (time64) {0}; }
fn DateTime os_utc_dateTimeNow(void) { return (DateTime) {0}; }
fn time64 os_utc_localizedTime64(i8 utc_offset) { return (time64) {0}; }
fn DateTime os_utc_localizedDateTime(i8 utc_offset) { return (DateTime) {0}; }
fn time64 os_utc_fromLocalTime64(time64 t) { return (time64) {0}; }
fn DateTime os_utc_fromLocalDateTime(DateTime *dt) { return (DateTime) {0}; }
fn f64 os_time_now(void) { return (f64) {0}; }
fn u64 os_time_update_frequency(void) { return (u64) {0}; }
fn void os_sleep_milliseconds(u32 ms) {}
fn OS_Handle os_timer_start(void) { return (OS_Handle) {0}; }
fn void os_timer_free(OS_Handle handle) {}
fn u64 os_timer_elapsed_between(OS_Handle start, OS_Handle end, OS_TimerGranularity unit) { return (u64) {0}; }

// =============================================================================
// Memory allocation
fn void* os_reserve(i64 bytes) {
  return malloc(bytes);
}

fn void* os_reserve_huge(i64 bytes) {
  Warn("Huge page allocation are not supported.");
  return 0;
}

fn void os_release(void *base, i64 bytes) {
  Unused(bytes);
  free(base);
}

fn void os_commit(void *base, i64 bytes) {}
fn void os_decommit(void *base, i64 bytes) {}

// =============================================================================
// Threads & Processes stuff: implemented per OS
fn OS_Handle os_thread_start(Func_Thread *thread_main, void *args) { return (OS_Handle) {0}; }
fn void os_thread_kill(OS_Handle thd) {}
fn void os_thread_cancel(OS_Handle thd) {}
fn bool os_thread_join(OS_Handle thd) { return (bool) {0}; }
fn void os_thread_cancelpoint(void) {}
fn OS_Handle os_proc_spawn(String8 command) { return (OS_Handle) {0}; }
fn void os_proc_kill(OS_Handle proc) {}
fn i32 os_proc_wait(OS_Handle proc) { return (i32) {0}; }
fn void os_exit(u8 status_code) {}
fn void os_atexit(Func_Void *callback) {}
fn OS_Handle os_mutex_alloc(void) { return (OS_Handle) {0}; }
fn void os_mutex_lock(OS_Handle handle) {}
fn bool os_mutex_trylock(OS_Handle handle) { return (bool) {0}; }
fn void os_mutex_unlock(OS_Handle handle) {}
fn void os_mutex_free(OS_Handle handle) {}
fn OS_Handle os_rwlock_alloc(void) { return (OS_Handle) {0}; }
fn void os_rwlock_read_lock(OS_Handle handle) {}
fn bool os_rwlock_read_trylock(OS_Handle handle) { return (bool) {0}; }
fn void os_rwlock_read_unlock(OS_Handle handle) {}
fn void os_rwlock_write_lock(OS_Handle handle) {}
fn bool os_rwlock_write_trylock(OS_Handle handle) { return (bool) {0}; }
fn void os_rwlock_write_unlock(OS_Handle handle) {}
fn void os_rwlock_free(OS_Handle handle) {}
fn OS_Handle os_cond_alloc(void) { return (OS_Handle) {0}; }
fn void os_cond_signal(OS_Handle handle) {}
fn void os_cond_broadcast(OS_Handle handle) {}
fn bool os_cond_wait(OS_Handle cond_handle, OS_Handle mutex_handle, u32 wait_at_most_microsec) { return (bool) {0}; }
fn bool os_cond_waitrw_read(OS_Handle cond_handle, OS_Handle rwlock_handle, u32 wait_at_most_microsec) { return (bool) {0}; }
fn bool os_cond_waitrw_write(OS_Handle cond_handle, OS_Handle rwlock_handle, u32 wait_at_most_microsec) { return (bool) {0}; }
fn bool os_cond_free(OS_Handle handle) { return (bool) {0}; }
fn OS_Handle os_semaphore_alloc(OS_SemaphoreKind kind, u32 init_count, u32 max_count, String8 name) { return (OS_Handle) {0}; }
fn bool os_semaphore_signal(OS_Handle sem) { return (bool) {0}; }
fn bool os_semaphore_wait(OS_Handle sem, u32 wait_at_most_microsec) { return (bool) {0}; }
fn bool os_semaphore_trywait(OS_Handle sem) { return (bool) {0}; }
fn void os_semaphore_free(OS_Handle sem) {}
fn SharedMem os_sharedmem_open(String8 name, i64 bytes, OS_AccessFlag flags) { return (SharedMem) {0}; }
fn void os_sharedmem_close(SharedMem *shm) {}

// =============================================================================
// Dynamic libraries
fn OS_Handle os_lib_open(String8 path) { return (OS_Handle) {0}; }
fn Func_Void *os_lib_lookup(OS_Handle lib, String8 symbol) { return 0; }
fn i32 os_lib_close(OS_Handle lib) { return (i32) {0}; }

// =============================================================================
// Misc
fn String8 os_currentDir(Arena *arena) { return (String8) {0}; }

// =============================================================================
// Networking
fn IP os_net_ip_from_str8(String8 name, OS_Net_Network hint) { return (IP) {0}; }
fn NetInterfaceList os_net_interfaces(Arena *arena) { return (NetInterfaceList) {0}; }
fn OS_Socket os_socket_open(String8 name, u16 port, OS_Net_Transport protocol) { return (OS_Socket) {0}; }
fn void os_socket_listen(OS_Socket *socket, u8 max_backlog) {}
fn OS_Socket os_socket_accept(OS_Socket *socket) { return (OS_Socket) {0}; }
fn void os_socket_connect(OS_Socket *server) {}
fn u8* os_socket_recv(Arena *arena, OS_Socket *client, i64 buffer_size) { return (u8*) {0}; }
fn void os_socket_send(OS_Socket *socket, String8 msg) {}
fn void os_socket_close(OS_Socket *socket) {}

// =============================================================================
// File reading and writing/appending
fn OS_Handle os_fs_open(String8 filepath, OS_AccessFlag flags) { return (OS_Handle) {0}; }
fn bool os_fs_close(OS_Handle fd) { return (bool) {0}; }
fn String8 os_fs_read(Arena *arena, OS_Handle file) { return (String8) {0}; }
fn String8 os_fs_read_virtual(Arena *arena, OS_Handle file, i64 bytes) { return (String8) {0}; }
fn bool os_fs_write(OS_Handle file, String8 content) { return (bool) {0}; }
fn bool os_fs_copy(String8 src, String8 dest) { return (bool) {0}; }
fn FS_Properties os_fs_get_properties(OS_Handle file) { return (FS_Properties) {0}; }
fn String8 os_fs_path_from_handle(Arena *arena, OS_Handle file) { return (String8) {0}; }
fn String8 os_fs_readlink(Arena *arena, String8 path) { return (String8) {0}; }

// =============================================================================
// Memory mapping files
fn void* os_fs_map(OS_Handle fd, i32 offset, i64 bytes) { return 0; }
fn bool os_fs_unmap(void *fmap, i64 bytes) { return (bool) {0}; }
fn File os_fs_fopen(Arena* arena, OS_Handle file) { return (File) {0}; }
fn File os_fs_fopen_tmp(Arena *arena) { return (File) {0}; }
fn bool os_fs_fclose(File *file) { return (bool) {0}; }
fn bool os_fs_fresize(File *file, i64 bytes) { return (bool) {0}; }
fn bool os_fs_file_has_changed(File *file) { return (bool) {0}; }
fn bool os_fs_fdelete(File *file) { return (bool) {0}; }
fn bool os_fs_frename(File *file, String8 to) { return (bool) {0}; }

// =============================================================================
// Misc operation on the filesystem
fn bool os_fs_delete(String8 filepath) { return (bool) {0}; }
fn bool os_fs_rename(String8 filepath, String8 to) { return (bool) {0}; }
fn bool os_fs_mkdir(String8 path) { return (bool) {0}; }
fn bool os_fs_rmdir(String8 path) { return (bool) {0}; }
fn String8 os_fs_filename_from_path(Arena *arena, String8 path) { return (String8) {0}; }

// =============================================================================
// File iteration
fn OS_FileIter* os_fs_iter_begin(Arena *arena, String8 path) { return 0; }
fn bool os_fs_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out) { return (bool) {0}; }
fn void os_fs_iter_end(OS_FileIter *iter) {}

// =============================================================================
// Entry point
i32 main(i32 argc, char **argv) {
  os_web_state.info.core_count = 1;
  os_web_state.info.page_size = KiB(64);
  os_web_state.arena = arena_build();

  CmdLine cli = {0};
  cli.count = argc - 1;
  cli.exe = str8_from_cstr(argv[0]);
  cli.args = arena_push_many(os_web_state.arena, String8, argc - 1);
  for (isize i = 1; i < argc; ++i) {
    cli.args[i - 1] = str8_from_cstr(argv[i]);
  }

  start(&cli);
}
