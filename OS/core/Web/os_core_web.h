#ifndef OS_CORE_WEB_H
#define OS_CORE_WEB_H

#include <stdio.h>

#undef ArenaDefaultReserveSize
#define ArenaDefaultReserveSize KiB(64)

#if DEBUG
#  define dbg_println(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#  define dbg_print(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#  define dbg_println(fmt, ...)
#  define dbg_print(fmt, ...)
#endif

typedef struct {
  Arena *arena;
  OS_SystemInfo info;
} OS_Web_State;

#endif
