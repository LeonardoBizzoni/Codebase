#ifndef OS_BSD_CORE_H
#define OS_BSD_CORE_H

#include <sys/mman.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <dirent.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>
#include <netdb.h>
#include <ifaddrs.h>

#ifndef MEMFILES_ALLOWED
#  define MEMFILES_ALLOWED 234414
#endif

#ifndef HOST_NAME_MAX
#  define HOST_NAME_MAX MAXHOSTNAMELEN
#endif

#endif
