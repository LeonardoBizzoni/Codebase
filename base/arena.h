#ifndef BASE_ARENA
#define BASE_ARENA

#if OS_LINUX || OS_BSD
#include <sys/mman.h>
#elif OS_WINDOWS
#include <windows.h>
#endif

#define New(...) Newx(__VA_ARGS__,New3,New2)(__VA_ARGS__)
#define Newx(a,b,c,d,...) d
#define New2(arenaptr, type) (type*)arenaPush(arenaptr, sizeof(type), AlignOf(type))
#define New3(arenaptr, type, count) (type*)arenaPush(arenaptr, (count) * sizeof(type), AlignOf(type))

typedef struct {
  void *base;
  usize head;
  usize total_size;
} Arena;

inline fn void *forwardAlign(void *ptr, usize align);
inline fn bool isPowerOfTwo(usize value);

       fn Arena *arenaBuild(usize size, usize base_addr);
inline fn void arenaPop(Arena *arena, usize bytes);
inline fn bool arenaFree(Arena *arena);
       fn void *arenaPush(Arena *arena, usize size, usize align);

typedef struct {
  Arena *arena;
  usize pos;
} Scratch;

inline fn Scratch tmpBegin(Arena *arena);
inline fn void tmpEnd(Scratch tmp);

#endif
