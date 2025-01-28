#ifndef WIN32_THREAD_H
#define WIN32_THREAD_H

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

global OS_W32_State w32_state;

fn OS_W32_Thread* os_w32_thread_alloc(void);
fn void os_w32_thread_release(OS_W32_Thread *thread);
fn DWORD os_w32_thread_entry_point(void *ptr);

#endif //WIN32_THREAD_H
