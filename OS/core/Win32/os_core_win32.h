#ifndef OS_CORE_WIN32_H
#define OS_CORE_WIN32_H

#include <windows.h>
#include <aclapi.h>
#include <ws2tcpip.h>

#define CONDITION_VARIABLE_LOCKMODE_EXCLUSIVE 0

#define dbg_println(Fmt, ...) dbg_print(Fmt "\n", __VA_ARGS__)

typedef u64 W32_PrimitiveType;
enum {
  W32_Primitive_Nil,
  W32_Primitive_File,
  W32_Primitive_Thread,
  W32_Primitive_RWLock,
  W32_Primitive_Mutex,
  W32_Primitive_CondVar,
  W32_Primitive_Semaphore,
  W32_Primitive_Timer,
  W32_Primitive_Socket,
};

typedef struct W32_Primitive {
  struct W32_Primitive *next;
  struct W32_Primitive *prev;
  W32_PrimitiveType kind;

  union {
    struct {
      HANDLE handle;
      DWORD tid;
      Func_Thread *func;
      void *arg;
      volatile bool should_quit;
    } thread;

    struct {
      HANDLE handle;
      OS_AccessFlags flags;
    } file;

    struct {
      SOCKET handle;
      socklen_t size;
      struct sockaddr addr;
    } socket;

    CRITICAL_SECTION mutex;
    SRWLOCK rw_mutex;
    CONDITION_VARIABLE condvar;
    HANDLE semaphore;
    LARGE_INTEGER timer;
  };
} W32_Primitive;

typedef struct {
  Arena *arena;
  OS_SystemInfo info;
  W32_Primitive *free_list;

  struct {
    OS_Handle mutex;
    W32_Primitive *first;
    W32_Primitive *last;
  } thread_list;

  LARGE_INTEGER perf_freq;
  CRITICAL_SECTION mutex;
} W32_State;

typedef struct {
  HANDLE handle;
  WIN32_FIND_DATAW file_data;
  bool done;
} W32_FileIter;

StaticAssert(sizeof(W32_FileIter) <= sizeof(OS_FileIter), file_iter_size_check);

////////////////////////////////
//- NOTE(km): Thread entry point
fn DWORD w32_thread_entry_point(void *ptr);

////////////////////////////////
//- km: Primitive functions

fn W32_Primitive* w32_primitive_alloc(W32_PrimitiveType kind);
fn void w32_primitive_release(W32_Primitive *primitive);

////////////////////////////////
//- km: Time conversion helpers

fn time64 w32_time64_from_system_time(SYSTEMTIME* in);
fn DateTime w32_date_time_from_system_time(SYSTEMTIME* in);
fn SYSTEMTIME w32_system_time_from_time64(time64 in);
fn SYSTEMTIME w32_system_time_from_date_time(DateTime *in);

#endif //OS_CORE_WIN32_H
