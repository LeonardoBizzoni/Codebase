#ifndef BASE_OS_LINUX_THREAD
#define BASE_OS_LINUX_THREAD

#include <stdio.h>

#if OS_LINUX
#include <pthread.h>
typedef pthread_t Thread;
#elif OS_BSD
#include <pthread.h>
typedef struct pthread Thread;
#else
#error os layer is not supported for this platform
#endif

typedef void* OS_ThreadFunction(void*);

inline fn Thread os_thdSpawn(OS_ThreadFunction *func);
fn Thread os_thdSpawnArgs(OS_ThreadFunction *func, void *arg_data);

inline fn void os_thdJoin(Thread id);
fn void os_thdJoinReturn(Thread id, void **save_return_value_in);

#endif
