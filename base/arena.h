#ifndef BASE_ARENA
#define BASE_ARENA

#include <math.h>

#define New(...) Newx(__VA_ARGS__,New3,New2)(__VA_ARGS__)
#define Newx(a,b,c,d,...) d
#define New2(arenaptr, type) (type*)arenaPush(arenaptr, sizeof(type), AlignOf(type))
#define New3(arenaptr, type, count) (type*)arenaPush(arenaptr, (count) * sizeof(type), AlignOf(type))

#define ArenaDefaultReserveSize MB(4)
#define ArenaDefaultCommitSize KiB(4)

typedef u64 ArenaFlags;
enum {
  Arena_Growable = 1 << 0,
  Arena_UseHugePage = 1 << 1,
};

typedef struct {
  usize commit_size;
  usize reserve_size;
  ArenaFlags flags;
} ArenaArgs;

typedef struct Arena {
  void *base;
  usize head;

  u64 flags;
  usize commits;
  usize commit_size;
  usize reserve_size;

  struct Arena *next;
  struct Arena *prev;
} Arena;

typedef struct {
  Arena *arena;
  usize pos;
} Scratch;

inline fn usize forwardAlign(usize ptr, usize align);
inline fn bool isPowerOfTwo(usize value);

inline fn void arenaPop(Arena *arena, usize bytes);
inline fn void arenaFree(Arena *arena);
       fn void *arenaPush(Arena *arena, usize size, usize align);

fn Arena *_arenaBuild(ArenaArgs args);
#if CPP
#  define ArenaBuild(...) _arenaBuild(ArenaArgs { __VA_ARGS__ })
#else
#  define ArenaBuild(...) _arenaBuild((ArenaArgs) {.commit_size = ArenaDefaultCommitSize, \
                                                   .reserve_size = ArenaDefaultReserveSize, \
                                                   __VA_ARGS__})
#endif

inline fn Scratch tmpBegin(Arena *arena);
inline fn void tmpEnd(Scratch tmp);

#endif
