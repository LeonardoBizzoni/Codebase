#ifndef BASE_ARENA
#define BASE_ARENA

#include <math.h>

#define New(...) Newx(__VA_ARGS__,New3,New2)(__VA_ARGS__)
#define Newx(a,b,c,d,...) d
#define New2(arenaptr, type) (type*)arena_push(arenaptr, sizeof(type), \
                                               (isize)AlignOf(type))
#define New3(arenaptr, type, count)                       \
  (type*)arena_push(arenaptr,                             \
                    (isize)(count) * (isize)sizeof(type), \
                    (isize)AlignOf(type))

#define ArenaDefaultReserveSize MB(4)
#define ArenaDefaultCommitSize KiB(4)

typedef u64 ArenaFlags;
enum {
  Arena_Growable = 1 << 0,
  Arena_UseHugePage = 1 << 1,
};

typedef struct {
  isize commit_size;
  isize reserve_size;
  ArenaFlags flags;
} ArenaArgs;

typedef struct Arena {
  void *base;
  usize head;

  u64 flags;
  isize commits;
  isize commit_size;
  isize reserve_size;

  struct Arena *next;
  struct Arena *prev;
} Arena;

typedef struct {
  Arena *arena;
  usize pos;
} Scratch;

fn bool is_power_of_two(isize value);
fn isize align_forward(isize ptr, isize align);

fn void arena_pop(Arena *arena, isize bytes);
fn void arena_free(Arena *arena);
fn void *arena_push(Arena *arena, isize size, isize align);

fn Arena *_arena_build(ArenaArgs args);
#if CPP
#  define ArenaBuild(...) _arena_build(ArenaArgs { __VA_ARGS__ })
#else
#  define ArenaBuild(...) _arena_build((ArenaArgs) {.commit_size = ArenaDefaultCommitSize, \
                                                   .reserve_size = ArenaDefaultReserveSize, \
                                                   __VA_ARGS__})
#endif

inline fn Scratch tmp_begin(Arena *arena);
inline fn void tmp_end(Scratch tmp);

#endif
