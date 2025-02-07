#ifndef OS_LINUX_CORE_H
#define OS_LINUX_CORE_H

#include <dirent.h>
#include <semaphore.h>
#include <sys/stat.h>

#define LNX_MUTEX_UNLOCKED 0UL
#define LNX_MUTEX_LOCKED 1UL

typedef struct {
  atomic(u32) futex;
  pthread_t owner;
  u32 count;
} LNX_Mutex;

#define LNX_RWLOCK_OPEN   0UL
#define LNX_RWLOCK_CLOSED 1UL

typedef u64 LNX_PrimitiveType;
enum {
  LNX_Primitive_Process,
  LNX_Primitive_Mutex,
  LNX_Primitive_Rwlock,
  LNX_Primitive_CondVar,
  LNX_Primitive_Timer,
  LNX_Primitive_Thread,
  LNX_Primitive_Semaphore,
};

typedef struct LNX_Primitive {
  struct LNX_Primitive *next;
  LNX_PrimitiveType type;

  union {
    pid_t proc;
    LNX_Mutex mutex;
    pthread_cond_t cond;
    struct timespec timer;
    struct {
      atomic(u32) futex;
      LNX_Mutex critical;
      u32 readers;
    } rwlock;
    struct {
      pthread_t handle;
      ThreadFunc *func;
      void *args;
    } thread;
    struct {
      OS_SemaphoreKind kind;
      u32 max_count;
      atomic(u32) *sem;
    } semaphore;
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
  LNX_Mutex primitive_lock;
  LNX_Primitive *primitive_freelist;

  u64 unix_utc_offset;
} LNX_State;

#define FUTEX_OP_SET        0  /* uaddr2 = oparg; */
#define FUTEX_OP_ADD        1  /* uaddr2 += oparg; */
#define FUTEX_OP_OR         2  /* uaddr2 |= oparg; */
#define FUTEX_OP_ANDN       3  /* uaddr2 &= ~oparg; */
#define FUTEX_OP_XOR        4  /* uaddr2 ^= oparg; */
#define FUTEX_OP_ARG_SHIFT  8  /* Use (1 << oparg) as operand */

#define FUTEX_OP_CMP_EQ     0  /* if (oldval_uaddr2 == cmparg) wake */
#define FUTEX_OP_CMP_NE     1  /* if (oldval_uaddr2 != cmparg) wake */
#define FUTEX_OP_CMP_LT     2  /* if (oldval_uaddr2 < cmparg) wake */
#define FUTEX_OP_CMP_LE     3  /* if (oldval_uaddr2 <= cmparg) wake */
#define FUTEX_OP_CMP_GT     4  /* if (oldval_uaddr2 > cmparg) wake */
#define FUTEX_OP_CMP_GE     5  /* if (oldval_uaddr2 >= cmparg) wake */

#define FUTEX_OP(op, oparg, cmp, cmparg) \
  (((op & 0xf) << 28) | \
   ((cmp & 0xf) << 24) | \
   ((oparg & 0xfff) << 12) | \
   (cmparg & 0xfff))

/*
       i32 syscall(SYS_futex, uint32_t *uaddr, int futex_op, uint32_t val,
                  (struct timespec *timeout | uint32_t val2),
                  uint32_t *uaddr2, uint32_t val3);
*/
#define lnx_futex(UADDR, OP, VAL, TIMEOUT, UADDR2, VAL3) \
  syscall(SYS_futex, UADDR, OP, VAL, TIMEOUT, UADDR2, VAL3)

inline fn i32 lnx_futex_wait(atomic(u32) *futex_addr, u32 expected_val, bool isPrivate);
inline fn i32 lnx_futex_timedwait(atomic(u32) *futex_addr, u32 expected_val,
				  struct timespec *timeout, bool isPrivate);
inline fn i32 lnx_futex_wake(atomic(u32) *futex_addr, u32 waiters2wake_count, bool isPrivate);
inline fn i32 lnx_futex_signal(atomic(u32) *futex_addr, bool isPrivate);
inline fn i32 lnx_futex_broadcast(atomic(u32) *futex_addr, bool isPrivate);
inline fn i32 lnx_futex_wake_op(atomic(u32) *futex_addr, u32 waiters2wake_count,
				atomic(u32) *futex_addr2, u32 waiters2wake_count2,
				u32 op, bool isPrivate);
inline fn i32 lnx_futex_cmp_requeue(atomic(u32) *futex_from, atomic(u32) *futex_to,
				    u32 expected_value, u32 waiters2wake_count,
				    u32 max_requeued_waiters, bool isPrivate);
inline fn i32 lnx_futex_requeue(atomic(u32) *futex_from, atomic(u32) *futex_to,
				u32 waiters2wake_count, u32 max_requeued_waiters,
				bool isPrivate);

fn void lnx_futeximpl_mutex_init(LNX_Mutex *m);
fn void lnx_futeximpl_mutex_lock(LNX_Mutex *m);
fn void lnx_futeximpl_mutex_unlock(LNX_Mutex *m);

fn LNX_Primitive* lnx_primitiveAlloc(LNX_PrimitiveType type);
fn void lnx_primitiveFree(LNX_Primitive *ptr);

fn void* lnx_threadEntry(void *args);

fn FS_Properties lnx_propertiesFromStat(struct stat *stat);
fn String8 lnx_getHostname();
fn void lnx_parseMeminfo();

#endif
