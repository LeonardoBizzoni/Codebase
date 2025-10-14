#ifndef BASE_ARENA
#define BASE_ARENA

#include <math.h>

#define arena_push(arenaptr, type)           \
  (type*)_arena_push(arenaptr, sizeof(type), \
                     (isize)AlignOf(type))
#define arena_push_many(arenaptr, type, count)            \
  (type*)_arena_push(arenaptr,                            \
                    (isize)(count) * (isize)sizeof(type), \
                    (isize)AlignOf(type))

#if CPP
#  define arena_build(...) _arena_build(ArenaArgs { __VA_ARGS__ })
#else
#  define arena_build(...) _arena_build((ArenaArgs) {.commit_size = ArenaDefaultCommitSize, \
                                                     .reserve_size = ArenaDefaultReserveSize, \
                                                     __VA_ARGS__})
#endif

#define ArenaDefaultReserveSize MB(4)
#define ArenaDefaultCommitSize KiB(4)

typedef u64 ArenaFlags;
enum {
  ArenaFlags_Growable = 1 << 0,
  ArenaFlags_UseHugePage = 1 << 1,
};

typedef struct {
  isize commit_size;
  isize reserve_size;
  ArenaFlags flags;
} ArenaArgs;

typedef struct Arena {
  void *base;
  usize head;

  ArenaFlags flags;
  isize commits;
  isize commit_size;
  isize reserve_size;

  struct Arena *curr;
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

fn Scratch tmp_begin(Arena *arena);
fn void tmp_end(Scratch tmp);

internal void *_arena_push(Arena *arena, isize size, isize align);
internal Arena *_arena_build(ArenaArgs args);

#endif
