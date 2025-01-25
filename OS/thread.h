#ifndef BASE_OS_LINUX_THREAD
#define BASE_OS_LINUX_THREAD

#include <stdio.h>

#if OS_LINUX || OS_BSD
#include <pthread.h>
#endif

typedef struct{
    u64 v[1];
} OS_Thread;

typedef void ThreadFunc(void*);

fn OS_Thread os_thread_start(ThreadFunc *thread_main, void *args);
fn bool os_thread_join(OS_Thread ids);

#endif
