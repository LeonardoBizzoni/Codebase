inline fn bool is_power_of_two(isize value) {
  return !(value & (value - 1));
}

inline fn isize align_forward(isize ptr, isize align) {
  Assert(is_power_of_two(align));
  isize mod = ptr & (align - 1);
  return (mod ? ptr = ptr + align - mod : ptr);
}

fn Arena *_arena_build(ArenaArgs args) {
  void *mem = 0;
  isize reserve, commit;

#if CPP
  if (args.commit_size == 0) {
    args.commit_size = ArenaDefaultCommitSize;
  }
  if (args.reserve_size == 0) {
    args.reserve_size = ArenaDefaultReserveSize;
  }
#endif

  if (args.flags & Arena_UseHugePage) {
    reserve = align_forward(args.reserve_size, os_sysinfo()->hugepage_size);
    commit = align_forward(args.commit_size, os_sysinfo()->hugepage_size);
    mem = os_reserve_huge(reserve);
  } else {
    reserve = align_forward(args.reserve_size, os_sysinfo()->page_size);
    commit = align_forward(args.commit_size, os_sysinfo()->page_size);
    mem = os_reserve(reserve);
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

inline fn void arena_pop(Arena *arena, isize bytes) {
  arena->head = (usize)ClampBot((isize)arena->head - bytes, 0);
  isize pages2decommit = (isize)ceil((f64)bytes / (f64)arena->commit_size);
  if (pages2decommit) {
    arena->commits = ClampBot(arena->commits - pages2decommit, 0);
    os_decommit((void *)align_forward((isize)((u8*)arena->base + arena->head),
                                      arena->commit_size),
                pages2decommit * arena->commit_size);
  }
}

inline fn void arena_free(Arena *arena) {
  os_release((void *)((usize)arena->base - sizeof(Arena)), arena->reserve_size);
}

fn void *arena_push(Arena *arena, isize size, isize align) {
  if (!align) { align = DefaultAlignment; }
  isize res = align_forward((isize)((u8*)arena->base + arena->head), align);
  isize offset = res - (isize)((u8*)arena->base + arena->head);
  isize new_head = (isize)arena->head + size + offset + (isize)sizeof(Arena);

  if (new_head > arena->reserve_size) {
    if (arena->next) {
      return arena_push(arena->next, size, align);
    } else if (arena->flags & Arena_Growable) {
      Warn("Resizing arena.");
      isize reserve_size = arena->reserve_size;
      if (size + (isize)sizeof(Arena) >= reserve_size) {
        reserve_size *= 2;
      }
      Arena *next = ArenaBuild(.commit_size = arena->commit_size,
                               .reserve_size = reserve_size,
                               .flags = arena->flags);
      Assert(next);
      arena->next = next;
      next->prev = arena;
      return arena_push(next, size, align);
    } else {
      return 0;
    }
  }

  if (new_head > arena->commits * arena->commit_size) {
    isize need2commit_bytes = align_forward(new_head, arena->commit_size);
    arena->commits = need2commit_bytes/arena->commit_size;
    os_commit((void *)((usize)arena->base - sizeof(Arena)), need2commit_bytes);
  }

  new_head -= (isize)sizeof(Arena);
  Assert(new_head >= 0);
  arena->head = (usize)new_head;
  memzero((void*)res, size);
  return (void*)res;
}

inline fn Scratch tmp_begin(Arena *arena) {
  Scratch scratch = { arena, arena->head };
  return scratch;
}

inline fn void tmp_end(Scratch tmp) {
  arena_pop(tmp.arena, (isize)(tmp.arena->head - tmp.pos));
  tmp.arena->head = tmp.pos;
}
