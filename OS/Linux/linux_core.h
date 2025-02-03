#ifndef OS_LINUX_CORE_H
#define OS_LINUX_CORE_H

#include <dirent.h>
#include <sys/stat.h>

typedef u64 LNX_PrimitiveType;
enum {
  LNX_Primitive_Process,
  LNX_Primitive_Mutex,
  LNX_Primitive_Rwlock,
  LNX_Primitive_CondVar,
  LNX_Primitive_Timer,
  LNX_Primitive_Thread,
};

typedef struct LNX_Primitive {
  struct LNX_Primitive *next;
  LNX_PrimitiveType type;

  union {
    pid_t proc;
    pthread_mutex_t mutex;
    pthread_rwlock_t rwlock;
    pthread_cond_t cond;
    struct timespec timer;
    struct {
      pthread_t handle;
      ThreadFunc *func;
      void *args;
    } thread;
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

fn LNX_Primitive* lnx_primitiveAlloc(LNX_PrimitiveType type);
fn void lnx_primitiveFree(LNX_Primitive *ptr);

fn void* lnx_threadEntry(void *args);

fn FS_Properties lnx_propertiesFromStat(struct stat *stat);
fn String8 lnx_getHostname();
fn void lnx_parseMeminfo();

#endif
