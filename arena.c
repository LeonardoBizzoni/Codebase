#include "base.h"
#include "arena.h"

inline fn bool isPowerOfTwo(usize value) {
  return !(value & (value - 1));
}

inline fn void *forwardAlign(void *ptr, usize align) {
  Assert(isPowerOfTwo(align));

  usize mod = (usize)ptr & (align - 1);
  return (mod ? ptr = (u8 *)ptr + align - mod
	      : ptr);
}

fn Arena *arenaBuild(usize size, usize base) {
#if OS_LINUX || OS_BSD
  void *fail_state = MAP_FAILED;
  void *mem = mmap((void *)base, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#elif OS_WINDOWS
  void *fail_state = 0;
  void *mem = VirtualAlloc((void *)base, size,
			   MEM_COMMIT | MEM_RESERVE,
			   PAGE_READWRITE);
#endif

  if (mem == fail_state) {
    return 0;
  } else {
    Arena *arena = (Arena *)mem;
    arena->base = arena + sizeof(Arena);
    arena->total_size = size - sizeof(Arena);
    arena->head = 0;

    return arena;
  }
}

inline fn void arenaPop(Arena *arena, usize bytes) {
  arena->head = ClampBot((isize)arena->head - (isize)bytes, 0);
}

inline fn bool arenaFree(Arena *arena) {
#if OS_LINUX || OS_BSD
  return munmap(arena->base, arena->total_size + sizeof(Arena));
#elif OS_WINDOWS
  return VirtualFree(arena->base, 0, MEM_RELEASE);
#else
  return false;
#endif
}

fn void *arenaPush(Arena *arena, usize size, usize align) {
  if (!align) {align = DefaultAlignment;}
  void *aligned_head = forwardAlign((u8 *)arena->base + arena->head, align);
  usize offset = (u8 *)aligned_head - (u8 *)arena->base;

  if ((u8 *)aligned_head + size > (u8 *)arena->base + arena->total_size) {
    return 0;
  }

  void *res = aligned_head;
  arena->head = offset + size;

  memZero(res, size);
  return res;
}
