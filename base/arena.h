#ifndef BASE_ARENA
#define BASE_ARENA

#define New(...) Newx(__VA_ARGS__,New3,New2)(__VA_ARGS__)
#define Newx(a,b,c,d,...) d
#define New2(arenaptr, type) (type*)arenaPush(arenaptr, sizeof(type), AlignOf(type))
#define New3(arenaptr, type, count) (type*)arenaPush(arenaptr, (count) * sizeof(type), AlignOf(type))

#define AlignFoward(p, a) (((p) + (a) - 1)&~((a) - 1))

#define ArenaDefaultReserveSize MB(4)
#define ArenaDefaultCommitSize KiB(4)

typedef u64 ArenaFlags;
enum {
  ArenaFlag_Growable = 1 << 0,
  ArenaFlag_UseHugePage = 1 << 1,
};

typedef struct {
  usize commit_size;
  usize reserve_size;
  ArenaFlags flags;
} ArenaArgs;

typedef struct Arena {
  void *base;
  u64 pos;
  u64 cmt_pos;
  u64 commit_size;
  u64 reserve_size;
  ArenaFlags flags;
  struct Arena *next;
  struct Arena *prev;
} Arena;

typedef struct {
  Arena *arena;
  usize pos;
} Scratch;

inline fn usize forwardAlign(usize ptr, usize align);
inline fn bool isPowerOfTwo(usize value);

fn void arena_pop_to(Arena *arean, u64 pos);
fn void arenaPop(Arena *arena, usize bytes);
fn void arenaFree(Arena *arena);
fn void *arenaPush(Arena *arena, usize size, usize align);

fn Arena *_arenaBuild(ArenaArgs args);
#if CPP
#  define ArenaBuild(...) _arenaBuild(ArenaArgs { __VA_ARGS__ })
#else
#  define ArenaBuild(...) _arenaBuild((ArenaArgs) {.commit_size = ArenaDefaultCommitSize, \
                                                   .reserve_size = ArenaDefaultReserveSize, \
                                                   __VA_ARGS__})
#endif

fn Scratch tmpBegin(Arena *arena);
fn void tmpEnd(Scratch tmp);

#endif
