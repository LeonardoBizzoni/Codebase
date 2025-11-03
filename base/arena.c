fn bool is_power_of_two(isize value) {
  if (value <= 0) { return false; }
  return !(value & (value - 1));
}

fn isize align_forward(isize ptr, isize align) {
  Assert(is_power_of_two(align));
  isize mod = ptr & (align - 1);
  return (mod ? ptr = ptr + align - mod : ptr);
}

fn void arena_pop(Arena *arena, isize bytes_to_pop) {
  Assert(arena);
  for (Arena *current = arena->curr;
       bytes_to_pop > 0 && current;) {
    isize new_head = (isize)current->head - bytes_to_pop;
    isize bytes_popped = (isize)current->head - ClampBot(new_head, 0);
    current->head = (usize)ClampBot(new_head, 0);
    isize pages_to_decommit = (isize)ceil((f64)bytes_popped /
                                          (f64)current->commit_size);
    bytes_to_pop -= bytes_popped;

    if (pages_to_decommit) {
      if (current->commits > pages_to_decommit) {
        current->commits -= pages_to_decommit;
        os_decommit((void *)align_forward((isize)((u8*)current +
                                                  (current->commits *
                                                   current->commit_size)),
                                          current->commit_size),
                    pages_to_decommit * current->commit_size);
      } else if (current->prev) {
        Arena *to_free = current;
        arena->curr = current->prev;
        current = current->prev;
        arena_free(to_free);
        continue;
      }
    }
    current = current->prev;
  }
}

fn void arena_free(Arena *arena) {
  Assert(arena);
  os_release((void *)((usize)arena->base - sizeof(Arena)), arena->reserve_size);
}

fn Scratch tmp_begin(Arena *arena) {
  Assert(arena);
  Scratch scratch = { arena, arena->head };
  return scratch;
}

fn void tmp_end(Scratch tmp) {
  Assert(tmp.arena);
  arena_pop(tmp.arena, (isize)(tmp.arena->head - tmp.pos));
  tmp.arena->head = tmp.pos;
}

internal Arena *_arena_build(ArenaArgs args) {
  void *mem = 0;
  isize reserve, commit;

  Assert(args.commit_size > 0);
  Assert(args.reserve_size > 0);
  if (args.flags & ArenaFlags_UseHugePage) {
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
  arena->curr = arena;
  arena->prev = 0;
  return arena;
}

internal void *_arena_push(Arena *arena, isize size, isize align) {
  Assert(arena);
  if (!align) { align = DefaultAlignment; }
  Arena *current = arena->curr;
  isize res = align_forward((isize)((u8*)current->base + current->head), align);
  isize offset = res - (isize)((u8*)current->base + current->head);
  isize new_head = (isize)current->head + size + offset + (isize)sizeof(Arena);

  if (new_head > current->reserve_size) {
    if (current->flags & ArenaFlags_Growable) {
      Warn("Resizing arena.");
#if 0
      isize reserve_size = arena->reserve_size * 2;
      while (reserve_size <= size) {
        reserve_size *= 2;
      }
#else
      isize times_to_double_reserve_size =
        Max(1, (isize)floor(log2((f64)size / (f64)arena->reserve_size)) + 1);
      isize reserve_size = arena->reserve_size << times_to_double_reserve_size;
#endif
      Arena *next = arena_build(.commit_size = arena->commit_size,
                                .reserve_size = reserve_size,
                                .flags = arena->flags);
      if (!next) { return 0; }
      arena->curr = next;
      next->prev = current;
      return _arena_push(next, size, align);
    } else {
      return 0;
    }
  }

  if (new_head > current->commits * current->commit_size) {
    isize need2commit_bytes = align_forward(new_head, current->commit_size);
    current->commits = need2commit_bytes/current->commit_size;
    os_commit((void *)((usize)current->base - sizeof(Arena)), need2commit_bytes);
  }

  new_head -= (isize)sizeof(Arena);
  Assert(new_head >= 0);
  current->head = (usize)new_head;
  memzero((void*)res, size);
  return (void*)res;
}


#ifdef CBUILD_H
CB_TEST(growable_arena_allocation) {
  Arena *arena = arena_build(.reserve_size = KiB(4),
                             .flags = ArenaFlags_Growable);
  cb_assert(arena);
  cb_assert(arena_push_many(arena, u8, 4032));
  cb_assert(arena->curr == arena);

  // Allocates a new arena with reserve_size = KiB(8)
  cb_assert(arena_push(arena, u8));
  cb_assert(arena->curr);
  cb_assert(arena->curr->prev == arena);

  // Pops&frees the new arena
  arena_pop(arena, 1);
  cb_assert(arena->curr == arena);

  // Allocates a new arena with reserve_size = KiB(8) and fills it up
  cb_assert(arena_push_many(arena, u8, 4032 + 4096));
  cb_assert(arena->curr);
  cb_assert(arena->curr->prev == arena);

  // Allocates a new arena with reserve_size = KiB(8)
  cb_assert(arena_push(arena, u8));
  cb_assert(arena->curr);
  cb_assert(arena->curr->prev != arena);
  cb_assert(arena->curr->prev->prev == arena);

  // Pops&frees both arenas keeping the original
  arena_pop(arena, 1 + 4032 + 4096);
  cb_assert(arena->curr == arena);
}

CB_TEST(growable_arena_big_allocation) {
  Arena *arena = arena_build(.flags = ArenaFlags_Growable);
  cb_assert(arena);

  u8 *bytes = arena_push_many(arena, u8, GB(1));
  cb_assert(bytes);
  cb_assert(arena->curr);
  cb_assert(arena->curr->prev == arena);
}
#endif
