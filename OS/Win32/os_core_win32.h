#ifndef OS_CORE_WIN32_H
#define OS_CORE_WIN32_H

typedef struct OS_W32_Thread OS_W32_Thread;
struct OS_W32_Thread
{
  OS_W32_Thread *next;
  HANDLE handle;
  DWORD tid;
  ThreadFunc *func;
  void *arg;
};

typedef struct OS_W32_State OS_W32_State;
struct OS_W32_State
{
  OS_W32_Thread thread_pool[256];
  OS_W32_Thread *free_list;
  u64 pos;
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

fn OS_W32_Thread* os_w32_thread_alloc(void);
fn void os_w32_thread_release(OS_W32_Thread *thread);
fn DWORD os_w32_thread_entry_point(void *ptr);

#endif //OS_CORE_WIN32_H
