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

  if (args.flags & Arena_UseHugePage) {
    reserve = forwardAlign(args.reserve_size, os_getSystemInfo()->hugepage_size);
    commit = forwardAlign(args.commit_size, os_getSystemInfo()->hugepage_size);

    mem = os_reserveHuge(args.base_addr, reserve);
  } else {
    reserve = forwardAlign(args.reserve_size, os_getSystemInfo()->page_size);
    commit = forwardAlign(args.commit_size, os_getSystemInfo()->page_size);

    mem = os_reserve(args.base_addr, reserve);
  }

  if (!mem) { return 0; }
  os_commit(mem, commit);

  Arena *arena = (Arena *)mem;
  arena->base = (void *)((usize)mem + sizeof(Arena));
  arena->head = 0;
  arena->flags = args.flags;
  arena->commit_size = commit;
  arena->reserve_size = reserve;
  arena->commits = 1;
  return arena;
}

inline fn void arenaPop(Arena *arena, usize bytes) {
  arena->head = ClampBot((isize)arena->head - (isize)bytes, 0);
  /* TODO(lb): decommit pages */
}

inline fn void arenaFree(Arena *arena) {
  os_release((void *)((usize)arena->base - sizeof(Arena)), arena->reserve_size);
}

fn void *arenaPush(Arena *arena, usize size, usize align) {
  if (!align) {align = DefaultAlignment;}
  usize res = forwardAlign((usize)arena->base + arena->head, align);
  usize offset = res - ((usize)arena->base + arena->head);
  usize new_head = arena->head + size + offset + sizeof(Arena);

  if (new_head > arena->reserve_size) {
    // TODO(lb): handle resizable arena
    return 0;
  }

  if (new_head > arena->commits * arena->commit_size) {
    usize need2commit_bytes = forwardAlign(new_head, arena->commit_size);
    arena->commits = need2commit_bytes/arena->commit_size;
    os_commit((void *)((usize)arena->base - sizeof(Arena)), need2commit_bytes);
  }

  arena->head = new_head - sizeof(Arena);
  memZero((void*)res, size);
  return (void*)res;
}

inline fn Scratch tmpBegin(Arena *arena) {
  Scratch scratch = { arena, arena->head };
  return scratch;
}

inline fn void tmpEnd(Scratch tmp) {
  arenaPop(tmp.arena, tmp.arena->head - tmp.pos);
  tmp.arena->head = tmp.pos;
}
