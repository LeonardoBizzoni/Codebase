inline fn bool isPowerOfTwo(usize value) {
  return !(value & (value - 1));
}

inline fn usize forwardAlign(usize ptr, usize align) {
  Assert(isPowerOfTwo(align));
  
  usize mod = ptr & (align - 1);
  return (mod ? ptr = ptr + align - mod : ptr);
}

fn Arena *_arenaBuild(ArenaArgs args) {
  void *mem = 0;
  usize reserve, commit;
  
#if CPP
  if (args.commit_size == 0) {
    args.commit_size = ArenaDefaultCommitSize;
  }
  if (args.reserve_size == 0) {
    args.reserve_size = ArenaDefaultReserveSize;
  }
#endif
  
  if (args.flags & ArenaFlag_UseHugePage) {
    reserve = forwardAlign(args.reserve_size, os_getSystemInfo()->hugepage_size);
    commit = forwardAlign(args.commit_size, os_getSystemInfo()->hugepage_size);
    mem = os_reserveHuge(reserve);
  } else {
    reserve = forwardAlign(args.reserve_size, os_getSystemInfo()->page_size);
    commit = forwardAlign(args.commit_size, os_getSystemInfo()->page_size);
    mem = os_reserve(reserve);
  }
  
  if (!mem) { return 0; }
  os_commit(mem, commit);
  
  Arena *arena = (Arena *)mem;
  arena->base = (u8*)mem + sizeof(Arena);
  arena->pos = 0;
  arena->flags = args.flags;
  arena->cmt_pos = commit;
  arena->commit_size = commit;
  arena->reserve_size = reserve;
  return arena;
}

fn void arena_pop_to(Arena *arena, u64 pos){
  pos = ClampBot(sizeof(Arena), pos);
  arena->pos = pos;
}

fn void arenaPop(Arena *arena, usize bytes){
  u64 pos = arena->pos;
  u64 new_pos = pos;
  if(bytes < pos){
    new_pos = pos - bytes;
  }
  arena_pop_to(arena, new_pos);
}

fn void arenaFree(Arena *arena) {
  os_release(arena->base, arena->reserve_size);
}

fn void *arenaPush(Arena *arena, usize size, usize align) {
  if (!align) { align = DefaultAlignment; }
  
  void *result = 0;
  u64 pos_aligned = AlignFoward(arena->pos, align);
  u64 new_pos = pos_aligned+size;
  if(new_pos > arena->cmt_pos){
    u64 new_cmt_pos = AlignFoward(new_pos, arena->commit_size);
    new_cmt_pos = ClampTop(new_cmt_pos, arena->reserve_size);
    u64 commit_size = new_cmt_pos - arena->cmt_pos;
    u8* ptr = (u8*)arena->base + arena->cmt_pos;
    os_commit(ptr, commit_size);
    arena->cmt_pos = new_cmt_pos;
  }
  
  if(arena->cmt_pos > new_pos){
    result = (u8*)arena->base + pos_aligned;
    arena->pos = new_pos;
  }
  
  memset(result, 0, size);
  return result;
}



fn Scratch tmpBegin(Arena *arena) {
  Scratch scratch = {arena, arena->pos};
  return scratch;
}

fn void tmpEnd(Scratch tmp) {
  arena_pop_to(tmp.arena, tmp.pos);
}
