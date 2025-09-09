#ifndef OS_UNIX_CORE_H
#define OS_UNIX_CORE_H

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

#if OS_SOUND
#  include <pulse/pulseaudio.h>
#endif

#if NDEBUG
#  define dbg_println(fmt, ...)
#  define dbg_print(fmt, ...)
#else
#  define dbg_println(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#  define dbg_print(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

typedef u64 UNX_PrimitiveType;
enum {
  UNX_Primitive_Process,
  UNX_Primitive_Mutex,
  UNX_Primitive_Rwlock,
  UNX_Primitive_CondVar,
  UNX_Primitive_Timer,
  UNX_Primitive_Thread,
  UNX_Primitive_Semaphore,
  UNX_Primitive_Socket,
  UNX_Primitive_Sound,
};

typedef struct UNX_Primitive {
  struct UNX_Primitive *next;
  UNX_PrimitiveType type;

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
      ThreadFunc *func;
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
#if OS_SOUND
    struct {
      File file;
      pa_stream *stream;
      pa_sample_spec sample_info;

      OS_Handle pausexit_condvar;
      OS_Handle pausexit_mutex;
      bool paused, should_exit;

      OS_Handle player;
      usize player_offset;
      OS_Handle player_mutex;
    } sound;
#endif
  };
} UNX_Primitive;

typedef struct {
  String8 path;
  DIR *dir;
  struct dirent *dir_entry;
} UNX_FileIter;

typedef struct {
  Arena *arena;
  OS_SystemInfo info;
  pthread_mutex_t primitive_lock;
  UNX_Primitive *primitive_freelist;

  u64 unix_utc_offset;
} UNX_State;

fn void unx_sharedmem_resize(SharedMem *shm, usize size);
fn void unx_sharedmem_unlink_name(SharedMem *shm);

#endif
