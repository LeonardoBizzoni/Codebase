#ifndef OS_POSIX_CORE_H
#define OS_POSIX_CORE_H

#include <pthread.h>
#include <semaphore.h>
#include <netdb.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <sched.h>
#include <dirent.h>
#include <ifaddrs.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#if DEBUG
#  define dbg_println(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#  define dbg_print(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#  define dbg_println(fmt, ...)
#  define dbg_print(fmt, ...)
#endif

#define OS_POSIX_TIME_FREQ 1000000000

typedef u64 OS_Posix_PrimitiveType;
enum {
  OS_Posix_Primitive_Process,
  OS_Posix_Primitive_Mutex,
  OS_Posix_Primitive_Rwlock,
  OS_Posix_Primitive_CondVar,
  OS_Posix_Primitive_Timer,
  OS_Posix_Primitive_Thread,
  OS_Posix_Primitive_Semaphore,
  OS_Posix_Primitive_Socket,
  OS_Posix_Primitive_Sound,
};

typedef struct OS_Posix_Primitive {
  struct OS_Posix_Primitive *next;
  OS_Posix_PrimitiveType type;

  union {
    pid_t proc;
    pthread_mutex_t mutex;
    pthread_rwlock_t rwlock;
    struct timespec timer;
    struct {
      pthread_cond_t cond;
      pthread_mutex_t mutex;
    } condvar;
    struct {
      pthread_t handle;
      Func_Thread *func;
      void *args;
    } thread;
    struct {
      OS_SemaphoreKind kind;
      u32 max_count;
      u32 count;
      sem_t *sem;
    } semaphore;
    struct {
      i32 fd;
      socklen_t size;
      struct sockaddr addr;
    } socket;
  };
} OS_Posix_Primitive;

typedef struct {
  String8 path;
  DIR *dir;
  struct dirent *dir_entry;
} OS_Posix_FileIter;

typedef struct {
  Arena *arena;
  OS_SystemInfo info;
  pthread_mutex_t primitive_lock;
  OS_Posix_Primitive *primitive_freelist;

  u64 unix_utc_offset;
  i64 time_offset;
} OS_Posix_State;

internal OS_Posix_Primitive* os_posix_primitive_alloc(OS_Posix_PrimitiveType type);
internal void os_posix_primitive_release(OS_Posix_Primitive *ptr);

internal void* os_posix_thread_entry(void *args);

internal FS_Properties os_posix_properties_from_stat(struct stat *stat);
internal i32 os_posix_flags_from_acf(OS_AccessFlag acf);

internal String8 os_posix_gethostname(void);

internal void os_posix_sharedmem_resize(SharedMem *shm, isize size);
internal void os_posix_sharedmem_unlink_name(SharedMem *shm);

internal void os_posix_sleep_nanoseconds(u32 ns);

#endif
