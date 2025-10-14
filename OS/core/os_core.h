#ifndef OS_CORE_H
#define OS_CORE_H

typedef struct {
  u64 h[1];
} OS_Handle;

typedef u8 OS_SemaphoreKind;
enum {
  OS_SemaphoreKind_Thread,
  OS_SemaphoreKind_Process,
};

typedef u8 OS_TimerGranularity;
enum {
  OS_TimerGranularity_min,
  OS_TimerGranularity_sec,
  OS_TimerGranularity_ms,
  OS_TimerGranularity_ns,
};

typedef struct {
  i64 page_size;
  i64 hugepage_size;

  u8 core_count;
  u64 total_memory;
  String8 hostname;
} OS_SystemInfo;

typedef u8 OS_LogLevel;
enum {
  OS_LogLevel_Log,
  OS_LogLevel_Info,
  OS_LogLevel_Warn,
  OS_LogLevel_Error,
};

typedef struct {
  i64 count;
  String8 exe;
  String8 *args;
} CmdLine;

typedef u8 OS_Permissions;
enum {
  OS_Permissions_Unknown = 0,
  OS_Permissions_Execute = 1 << 0,
  OS_Permissions_Write   = 1 << 1,
  OS_Permissions_Read    = 1 << 2,
};

typedef u32 OS_AccessFlags;
enum {
  OS_acfRead       = 1 << 0,
  OS_acfWrite      = 1 << 1,
  OS_acfExecute    = 1 << 2,
  OS_acfAppend     = 1 << 3,
  OS_acfShareRead  = 1 << 4,
  OS_acfShareWrite = 1 << 5,
};

typedef u64 OS_FileType;
enum {
  OS_FileType_BlkDevice  = 1 << 0,
  OS_FileType_CharDevice = 1 << 1,
  OS_FileType_Dir        = 1 << 2,
  OS_FileType_Pipe       = 1 << 3,
  OS_FileType_Link       = 1 << 4,
  OS_FileType_Socket     = 1 << 5,
  OS_FileType_Regular    = 1 << 6,
};

typedef struct {
  u32 ownerID;
  u32 groupID;
  isize size;
  OS_FileType type;

  time64 last_access_time;
  time64 last_modification_time;
  time64 last_status_change_time;

  union {
    OS_Permissions permissions[3];

    struct {
      OS_Permissions user;
      OS_Permissions group;
      OS_Permissions other;
    };
  };
} FS_Properties;

typedef struct {
  OS_Handle mmap_handle;
  OS_Handle file_handle;
  FS_Properties prop;
  String8 path;
  u8 *content;
} File, SharedMem;

typedef struct FilenameNode {
  String8 value;
  struct FilenameNode *next;
  struct FilenameNode *prev;
} FilenameNode;

typedef struct {
  FilenameNode *first;
  FilenameNode *last;
} FilenameList;

typedef struct{
  String8 name;
  FS_Properties properties;
} OS_FileInfo;

typedef struct{
  OS_FileType filter_allowed;
  u8 memory[640];
} OS_FileIter;

typedef i32 OS_Net_Transport;
enum {
  OS_Net_Transport_Invalid,
  OS_Net_Transport_TCP,
  OS_Net_Transport_UDP,
};

typedef i32 OS_Net_Network;
enum {
  OS_Net_Network_Invalid,
  OS_Net_Network_IPv4 = 4,
  OS_Net_Network_IPv6 = 6,
};

typedef struct {
  u8 bytes[4];
} IPv4;

typedef struct {
  u16 words[8];
} IPv6;

typedef struct {
  OS_Net_Network version;
  union {
    IPv4 v4;
    IPv6 v6;
  };
} IP;

typedef struct NetInterface {
  String8 name;
  String8 strip;

  OS_Net_Network version;
  union {
    struct {
      IPv4 addr;
      IPv4 netmask;
    } ipv4;
    struct {
      IPv6 addr;
      IPv6 netmask;
    } ipv6;
  };

  struct NetInterface *next;
  struct NetInterface *prev;
} NetInterface;

typedef struct {
  NetInterface *first;
  NetInterface *last;
} NetInterfaceList;

typedef struct {
  OS_Net_Transport protocol_transport;
  OS_Handle handle;
  struct {
    IP addr;
    u16 port;
  } client, server;
} OS_Socket;

typedef void Func_Void(void);
typedef void Func_Thread(void*);
typedef void Func_Signal(i32);

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define Log(STR, ...) os_print(OS_LogLevel_Log, __func__, __FILE__, __LINE__, STR, ##__VA_ARGS__)
#if DEBUG
#  define Info(STR, ...) os_print(OS_LogLevel_Info, __func__, __FILE__, __LINE__, STR, ##__VA_ARGS__)
#  define Warn(STR, ...) os_print(OS_LogLevel_Warn, __func__, __FILE__, __LINE__, STR, ##__VA_ARGS__)
#  define Err(STR, ...) os_print(OS_LogLevel_Error, __func__, __FILE__, __LINE__, STR, ##__VA_ARGS__)
#else
#  define Info(STR, ...)
#  define Warn(STR, ...)
#  define Err(STR, ...)
#endif

fn void os_print(OS_LogLevel level, const char *caller, const char *file,
                 i32 line, const char *fmt, ...);

// =============================================================================
// Main entry point
fn void os_env_setup(void);
fn void start(CmdLine *cmdln);

// =============================================================================
// System information retrieval
fn OS_SystemInfo *os_sysinfo(void);

// =============================================================================
// DateTime
fn time64 os_local_now(void);
fn DateTime os_local_dateTimeNow(void);
fn time64 os_local_fromUTCTime64(time64 t);
fn DateTime os_local_fromUTCDateTime(DateTime *dt);

fn time64 os_utc_now(void);
fn DateTime os_utc_dateTimeNow(void);
fn time64 os_utc_localizedTime64(i8 utc_offset);
fn DateTime os_utc_localizedDateTime(i8 utc_offset);
fn time64 os_utc_fromLocalTime64(time64 t);
fn DateTime os_utc_fromLocalDateTime(DateTime *dt);

fn void os_sleep_milliseconds(u32 ms);

fn OS_Handle os_timer_start(void);
fn void os_timer_free(OS_Handle handle);
fn bool os_timer_reached(OS_Handle timer, u64 how_much, OS_TimerGranularity unit);
fn u64 os_timer_elapsed(OS_Handle start, OS_TimerGranularity unit);
fn u64 os_timer_elapsed_between(OS_Handle start, OS_Handle end,
                                OS_TimerGranularity unit);

// =============================================================================
// Memory allocation
fn void* os_reserve(isize size);
fn void* os_reserve_huge(isize size);
fn void os_release(void *base, isize size);

fn void os_commit(void *base, isize size);
fn void os_decommit(void *base, isize size);

// =============================================================================
// Threads & Processes stuff
fn OS_Handle os_thread_start(Func_Thread *thread_main, void *args);
fn void os_thread_kill(OS_Handle thd);
fn void os_thread_cancel(OS_Handle thd);
fn bool os_thread_join(OS_Handle thd);
fn void os_thread_cancelpoint(void);

// TODO(lb): add stream redirection?
fn OS_Handle os_proc_spawn(String8 command);
fn void os_proc_kill(OS_Handle proc);
fn i32 os_proc_wait(OS_Handle proc);

fn void os_exit(u8 status_code);
fn void os_atexit(Func_Void *callback);

fn OS_Handle os_mutex_alloc(void);
fn void os_mutex_lock(OS_Handle handle);
fn bool os_mutex_trylock(OS_Handle handle);
fn void os_mutex_unlock(OS_Handle handle);
fn void os_mutex_free(OS_Handle handle);
#define os_mutex_scope(mutex) DeferLoop(os_mutex_lock(mutex), os_mutex_unlock(mutex))

fn OS_Handle os_rwlock_alloc(void);
fn void os_rwlock_read_lock(OS_Handle handle);
fn bool os_rwlock_read_trylock(OS_Handle handle);
fn void os_rwlock_read_unlock(OS_Handle handle);
fn void os_rwlock_write_lock(OS_Handle handle);
fn bool os_rwlock_write_trylock(OS_Handle handle);
fn void os_rwlock_write_unlock(OS_Handle handle);
fn void os_rwlock_free(OS_Handle handle);

fn OS_Handle os_cond_alloc(void);
fn void os_cond_signal(OS_Handle handle);
fn void os_cond_broadcast(OS_Handle handle);
fn bool os_cond_wait(OS_Handle cond_handle, OS_Handle mutex_handle,
                     u32 wait_at_most_microsec);
fn bool os_cond_waitrw_read(OS_Handle cond_handle, OS_Handle rwlock_handle,
                            u32 wait_at_most_microsec);
fn bool os_cond_waitrw_write(OS_Handle cond_handle, OS_Handle rwlock_handle,
                             u32 wait_at_most_microsec);
fn bool os_cond_free(OS_Handle handle);

fn OS_Handle os_semaphore_alloc(OS_SemaphoreKind kind, u32 init_count,
                                u32 max_count, String8 name);
fn bool os_semaphore_signal(OS_Handle sem);
fn bool os_semaphore_wait(OS_Handle sem, u32 wait_at_most_microsec);
fn bool os_semaphore_trywait(OS_Handle sem);
fn void os_semaphore_free(OS_Handle sem);

fn SharedMem os_sharedmem_open(String8 name, isize size, OS_AccessFlags flags);
fn void os_sharedmem_close(SharedMem *shm);

// =============================================================================
// Dynamic libraries
fn OS_Handle os_lib_open(String8 path);
fn Func_Void *os_lib_lookup(OS_Handle lib, String8 symbol);
fn i32 os_lib_close(OS_Handle lib);

// =============================================================================
// Misc
fn String8 os_currentDir(Arena *arena);

// =============================================================================
// Networking
fn IP os_net_ip_from_str8(String8 name, OS_Net_Network hint);
fn NetInterface os_net_interface_lookup(String8 interface);
fn NetInterfaceList os_net_interfaces(Arena *arena);

fn OS_Socket os_socket_open(String8 name, u16 port, OS_Net_Transport protocol);
fn void os_socket_listen(OS_Socket *socket, u8 max_backlog);
fn OS_Socket os_socket_accept(OS_Socket *socket);
fn void os_socket_connect(OS_Socket *server);
fn u8* os_socket_recv(Arena *arena, OS_Socket *client, usize buffer_size);
fn void os_socket_send(OS_Socket *socket, String8 msg);
fn void os_socket_close(OS_Socket *socket);

// =============================================================================
// File reading and writing/appending
fn OS_Handle os_fs_open(String8 filepath, OS_AccessFlags flags);
fn bool os_fs_close(OS_Handle fd);
fn String8 os_fs_read(Arena *arena, OS_Handle file);
fn String8 os_fs_read_virtual(Arena *arena, OS_Handle file, usize size);
fn bool os_fs_write(OS_Handle file, String8 content);
fn bool os_fs_copy(String8 src, String8 dest);

fn FS_Properties os_fs_get_properties(OS_Handle file);
fn String8 os_fs_path_from_handle(Arena *arena, OS_Handle file);
fn String8 os_fs_readlink(Arena *arena, String8 path);

// =============================================================================
// Memory mapping files
fn void* os_fs_map(OS_Handle fd, i32 offset, isize length);
fn bool os_fs_unmap(void *fmap, isize length);

fn File os_fs_fopen(Arena* arena, OS_Handle file);
fn File os_fs_fopen_tmp(Arena *arena);
fn bool os_fs_fclose(File *file);
fn bool os_fs_fresize(File *file, isize size);
fn void os_fs_fwrite(File *file, String8 str);

fn bool os_fs_file_has_changed(File *file);
fn bool os_fs_fdelete(File *file);
fn bool os_fs_frename(File *file, String8 to);

// =============================================================================
// Misc operation on the filesystem
fn bool os_fs_delete(String8 filepath);
fn bool os_fs_rename(String8 filepath, String8 to);

fn bool os_fs_mkdir(String8 path);
fn bool os_fs_rmdir(String8 path);

fn String8 os_fs_filename_from_path(Arena *arena, String8 path);

// =============================================================================
// File iteration
fn OS_FileIter* os_fs_iter_begin(Arena *arena, String8 path);
fn OS_FileIter* os_fs_iter_begin_filtered(Arena *arena, String8 path, OS_FileType allowed);
fn bool os_fs_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out);
fn void os_fs_iter_end(OS_FileIter *iter);

// =============================================================================
// Debugger communication func
fn void dbg_print(const char *fmt, ...);

#endif
