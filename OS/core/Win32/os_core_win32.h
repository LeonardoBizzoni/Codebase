#ifndef OS_CORE_WIN32_H
#define OS_CORE_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ws2def.h>
#include <ws2tcpip.h>

#define CONDITION_VARIABLE_LOCKMODE_EXCLUSIVE 0

typedef u64 OS_W32_PrimitiveType;
enum
{
  OS_W32_Primitive_Nil,
  OS_W32_Primitive_File,
  OS_W32_Primitive_Thread,
  OS_W32_Primitive_RWLock,
  OS_W32_Primitive_Mutex,
  OS_W32_Primitive_CondVar,
  OS_W32_Primitive_Semaphore,
  OS_W32_Primitive_Socket,
};

typedef struct OS_W32_Primitive OS_W32_Primitive;
struct OS_W32_Primitive
{
  OS_W32_Primitive *next;
  OS_W32_PrimitiveType kind;
  union
  {
    struct
    {
      HANDLE handle;
      DWORD tid;
      ThreadFunc *func;
      void *arg;
    } thread;

    struct {
      HANDLE handle;
      OS_AccessFlags flags;
    } file;

    CRITICAL_SECTION mutex;
    SRWLOCK rw_mutex;
    CONDITION_VARIABLE condvar;
    HANDLE semaphore;

    struct {
      SOCKET handle;
      socklen_t size;
      struct sockaddr addr;
    } socket;
  };
};

typedef struct OS_W32_State OS_W32_State;
struct OS_W32_State
{
  Arena *arena;
  OS_SystemInfo info;
  OS_W32_Primitive *free_list;
  CRITICAL_SECTION mutex;
};

typedef struct OS_W32_FileIter OS_W32_FileIter;
struct OS_W32_FileIter
{
  HANDLE handle;
  WIN32_FIND_DATAW file_data;
  bool done;
};

StaticAssert(sizeof(OS_W32_FileIter) <= sizeof(OS_FileIter), file_iter_size_check);

global OS_W32_State w32_state;


////////////////////////////////
//- NOTE(km): Thread entry point
fn DWORD os_w32_thread_entry_point(void *ptr);

////////////////////////////////
//- km: Primitive functions

fn OS_W32_Primitive* os_w32_primitive_alloc(OS_W32_PrimitiveType kind);
fn void os_w32_primitive_release(OS_W32_Primitive *primitive);

////////////////////////////////
//- km: Time conversion helpers

fn DateTime os_w32_date_time_from_system_time(SYSTEMTIME* in);
fn SYSTEMTIME os_w32_system_time_from_date_time(DateTime *in);

#endif //OS_CORE_WIN32_H
