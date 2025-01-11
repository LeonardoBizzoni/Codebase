#ifndef BASE_ARENA
#define BASE_ARENA

#include "base.h"

#if OS_LINUX || OS_BSD
#include <sys/mman.h>
#elif OS_WINDOWS
#include <windows.h>
#endif

#define New(...) Newx(__VA_ARGS__,New3,New2)(__VA_ARGS__)
#define Newx(a,b,c,d,...) d
#define New2(arenaptr, type) (type*)arenaPush(arenaptr, sizeof(type), _Alignof(type))
#define New3(arenaptr, type, count) (type*)arenaPush(arenaptr, (count) * sizeof(type), _Alignof(type))

typedef struct {
  void *base;
  usz head;
  usz total_size;
} Arena;

inline fn void *forwardAlign(void *ptr, usz align);
inline fn bool isPowerOfTwo(usz value);

fn Arena *arenaBuild(usz size, usz base_addr);
inline fn void arenaPop(Arena *arena, usz bytes);
inline fn bool arenaFree(Arena *arena);
fn void *arenaPush(Arena *arena, usz size, usz align);

#endif
