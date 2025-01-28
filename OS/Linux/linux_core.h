#ifndef OS_LINUX_CORE_H
#define OS_LINUX_CORE_H

#include <sys/sysinfo.h>

typedef struct {
  ThreadFunc *func;
  void *args;
} lnx_thdData;

fn void* lnx_thdEntry(void *args);

fn String8 lnx_getHostname();

#endif
