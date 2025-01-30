#ifndef OS_CORE_H
#define OS_CORE_H

#include <stdio.h>

#if OS_LINUX || OS_BSD
#  include <pthread.h>
#endif

typedef struct {
  u64 h[1];
} OS_Handle;

typedef struct {
  bool is_child;
  OS_Handle handle;
} OS_ProcHandle;

typedef struct {
  u64 page_size;
  u64 hugepage_size;

  u8 core_count;
  u64 total_memory;
  String8 hostname;
} OS_SystemInfo;

typedef struct{
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
  usize size;
  OS_FileType type;

  u64 last_access_time;
  u64 last_modification_time;
  u64 last_status_change_time;

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
} File;

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

typedef void VoidFunc(void);
typedef void ThreadFunc(void*);

// =============================================================================
// Main entry point
fn void start(CmdLine *cmdln);

// =============================================================================
// System information retrieval
fn OS_SystemInfo *os_getSystemInfo();

// =============================================================================
// Misc
fn void os_sleep(usize ms);
fn DateTime os_currentDateTime();

// =============================================================================
// Memory allocation
fn void* os_reserve(usize base_addr, usize size);
fn void* os_reserveHuge(usize base_addr, usize size);
fn void os_release(void *base, usize size);

fn void os_commit(void *base, usize size);
fn void os_decommit(void *base, usize size);

// =============================================================================
// Threads & Processes stuff
fn OS_Handle os_thread_start(ThreadFunc *thread_main, void *args);
fn void os_thread_kill(OS_Handle thd);
fn bool os_thread_join(OS_Handle thd);

fn OS_ProcHandle os_proc_spawn();
fn void os_proc_kill(OS_ProcHandle proc);
fn void os_proc_wait(OS_ProcHandle proc);
// TODO(lb): Handle status code returned by child process

fn OS_Handle os_mutex_alloc();
fn void os_mutex_lock(OS_Handle handle);
fn bool os_mutex_trylock(OS_Handle handle);
fn void os_mutex_unlock(OS_Handle handle);
fn void os_mutex_free(OS_Handle handle);

fn OS_Handle os_rwlock_alloc();
fn void os_rwlock_read_lock(OS_Handle handle);
fn bool os_rwlock_read_trylock(OS_Handle handle);
fn void os_rwlock_read_unlock(OS_Handle handle);
fn void os_rwlock_write_lock(OS_Handle handle);
fn bool os_rwlock_write_trylock(OS_Handle handle);
fn void os_rwlock_write_unlock(OS_Handle handle);
fn void os_rwlock_free(OS_Handle handle);

// =============================================================================
// Dynamic libraries
fn OS_Handle os_lib_open(String8 path);
fn VoidFunc *os_lib_lookup(OS_Handle lib, String8 symbol);
fn i32 os_lib_close(OS_Handle lib);

// =============================================================================
// File reading and writing/appending
fn OS_Handle fs_open(String8 filepath, OS_AccessFlags flags);
fn bool fs_close(OS_Handle fd);
fn String8 fs_readVirtual(Arena *arena, OS_Handle file, usize size);
fn String8 fs_read(Arena *arena, OS_Handle file);
fn bool fs_write(OS_Handle file, String8 content);

fn FS_Properties fs_getProp(OS_Handle file);
fn String8 fs_pathFromHandle(Arena *arena, OS_Handle file);
fn String8 fs_readlink(Arena *arena, String8 path);

// =============================================================================
// Memory mapping files
       fn File fs_fopen(Arena* arena, OS_Handle file);
       fn File fs_fopenTmp(Arena *arena);
inline fn bool fs_fclose(File *file);
inline fn bool fs_fresize(File *file, usize size);
inline fn void fs_fwrite(File *file, String8 str);

inline fn bool fs_fileHasChanged(File *file);
inline fn bool fs_fdelete(File *file);
inline fn bool fs_frename(File *file, String8 to);

// =============================================================================
// Misc operation on the filesystem
inline fn bool fs_delete(String8 filepath);
inline fn bool fs_rename(String8 filepath, String8 to);

inline fn bool fs_mkdir(String8 path);
inline fn bool fs_rmdir(String8 path);

// =============================================================================
// File iteration
fn OS_FileIter* fs_iter_begin(Arena *arena, String8 path);
fn bool fs_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out);
fn void fs_iter_end(OS_FileIter *iter);

#endif
