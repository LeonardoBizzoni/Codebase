#ifndef OS_LINUX_CORE_H
#define OS_LINUX_CORE_H

#include <dirent.h>
#include <semaphore.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>

#if OS_SOUND
#  include <pulse/pulseaudio.h>
#endif

typedef u64 LNX_PrimitiveType;
enum {
  LNX_Primitive_Process,
  LNX_Primitive_Mutex,
  LNX_Primitive_Rwlock,
  LNX_Primitive_CondVar,
  LNX_Primitive_Timer,
  LNX_Primitive_Thread,
  LNX_Primitive_Semaphore,
  LNX_Primitive_Socket,
  LNX_Primitive_Sound,
};

typedef struct LNX_Primitive {
  struct LNX_Primitive *next;
  LNX_PrimitiveType type;

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
      bool paused, should_exit;
      pa_stream *stream;
      pa_sample_spec sample_info;
      OS_Handle pause_condvar;
      OS_Handle pause_mutex;
      OS_Handle player;
    } sound;
#endif
  };
} LNX_Primitive;

typedef struct {
  String8 path;
  DIR *dir;
  struct dirent *dir_entry;
} LNX_FileIter;

typedef struct {
  Arena *arena;
  OS_SystemInfo info;
  pthread_mutex_t primitive_lock;
  LNX_Primitive *primitive_freelist;

  u64 unix_utc_offset;
} LNX_State;

// Missing in glibc
struct sched_attr {
  u32 size;              /* Size of this structure */
  u32 sched_policy;      /* Policy (SCHED_*) */
  u64 sched_flags;       /* Flags */
  i32 sched_nice;        /* Nice value (SCHED_OTHER, SCHED_BATCH) */
  u32 sched_priority;    /* Static priority (SCHED_FIFO, SCHED_RR) */
  /* For SCHED_DEADLINE */
  u64 sched_runtime;
  u64 sched_deadline;
  u64 sched_period;
  /* Utilization hints */
  u32 sched_util_min;
  u32 sched_util_max;
};

fn LNX_Primitive* lnx_primitiveAlloc(LNX_PrimitiveType type);
fn void lnx_primitiveFree(LNX_Primitive *ptr);

fn void* lnx_threadEntry(void *args);

fn FS_Properties lnx_propertiesFromStat(struct stat *stat);
fn String8 lnx_getHostname(void);
fn void lnx_parseMeminfo(void);

fn i32 lnx_sched_setattr(u32 policy, u64 runtime_ns, u64 deadline_ns, u64 period_ns);
fn void lnx_sched_set_deadline(u64 runtime_ns, u64 deadline_ns, u64 period_ns,
                               SignalFunc *deadline_miss_handler);
fn void lnx_sched_yield(void);

#endif
