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
  u64 page_size;
  u64 hugepage_size;

  u8 core_count;
  u64 total_memory;

  u8 ipv4[4];
  u16 ipv6[4];
  String8 hostname;
} OS_SystemInfo;

typedef u8 OS_Permissions;
enum {
  OS_Permissions_Unknown = 0,
  OS_Permissions_Execute = 1 << 0,
  OS_Permissions_Write = 1 << 1,
  OS_Permissions_Read = 1 << 2,
};

typedef u32 OS_AccessFlags;
enum {
  OS_acfRead = 1 << 0,
  OS_acfWrite = 1 << 1,
  OS_acfExecute = 1 << 2,
  OS_acfAppend = 1 << 3,
  OS_acfShareRead = 1 << 4,
  OS_acfShareWrite = 1 << 5,
};

typedef struct {
  u32 ownerID;
  u32 groupID;
  usize size;

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
  OS_Handle handle;
  String8 path;
  FS_Properties prop;
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

typedef void VoidFunc(void);
typedef void ThreadFunc(void*);

// =============================================================================
// Main entry point
fn void start(u32 argc, String8 *argv);

// =============================================================================
// System information retrieval
fn OS_SystemInfo *os_getSystemInfo();

// =============================================================================
// Memory allocation
fn void* os_reserve(usize base_addr, usize size);
fn void* os_reserveHuge(usize base_addr, usize size);
fn void os_release(void *base, usize size);

fn void os_commit(void *base, usize size);
fn void os_decommit(void *base, usize size);

// =============================================================================
// Threading
fn OS_Handle os_thread_start(ThreadFunc *thread_main, void *args);
fn bool os_thread_join(OS_Handle ids);

// =============================================================================
// Dynamic libraries
fn OS_Handle os_lib_open(String8 path);
fn VoidFunc *os_lib_lookup(OS_Handle lib, String8 symbol);
fn i32 os_lib_close(OS_Handle lib);

// =============================================================================
// File reading and writing/appending
       fn OS_Handle fs_open(String8 filepath, OS_AccessFlags flags);
       fn String8 fs_read(Arena *arena, OS_Handle file);
inline fn bool fs_write(OS_Handle file, String8 content);

fn FS_Properties fs_getProp(OS_Handle file);

// =============================================================================
// Memory mapping files
       fn File fs_fopen(String8 filepath);
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
fn FilenameList fs_iterFiles(Arena *arena, String8 dirname);
fn bool fs_rmIter(String8 dirname);

#endif
