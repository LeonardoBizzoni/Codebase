#ifndef OS_BSD_CORE_H
#define OS_BSD_CORE_H

#include <dirent.h>
#include <semaphore.h>
#include <sys/stat.h>

#ifndef MEMFILES_ALLOWED
#  define MEMFILES_ALLOWED 234414
#endif

typedef u64 BSD_PrimitiveType;
enum {
  BSD_Primitive_Process,
  BSD_Primitive_Mutex,
  BSD_Primitive_Rwlock,
  BSD_Primitive_Timer,
  BSD_Primitive_CondVar,
  BSD_Primitive_Thread,
  BSD_Primitive_Semaphore,
};

typedef struct {
  ThreadFunc *func;
  void *args;
} bsd_thdData;

typedef struct BSD_Primitive {
  struct BSD_Primitive *next;
  BSD_PrimitiveType type;

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
  };
} BSD_Primitive;

typedef struct {
  String8 path;
  DIR *dir;
  struct dirent *dir_entry;
} BSD_FileIter;

typedef struct {
  Arena *arena;
  OS_SystemInfo info;
  String8 filemap[MEMFILES_ALLOWED];
  pthread_mutex_t primitive_lock;
  BSD_Primitive *primitive_freelist;

  u64 unix_utc_offset;
} BSD_State;

fn BSD_Primitive* bsd_primitiveAlloc(BSD_PrimitiveType type);
fn void bsd_primitiveFree(BSD_Primitive *ptr);

fn void* bsd_thdEntry(void *args);

fn FS_Properties bsd_propertiesFromStat(struct stat *stat);
fn String8 bsd_gethostname();

#endif
