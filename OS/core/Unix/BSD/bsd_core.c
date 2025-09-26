global String8 bsd_filemap[MEMFILES_ALLOWED] = {0};

// =============================================================================
// Memory allocation
fn void* os_reserve_huge(isize size) {
  Assert(size > 0);
  void *res = mmap(0, (usize)size, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED_SUPER, -1, 0);
  if (res == MAP_FAILED) { res = 0; }
  return res;
}

// =============================================================================
// File reading and writing/appending
fn OS_Handle fs_open(String8 filepath, OS_AccessFlags flags) {
  i32 access_flags = O_CREAT | unx_flags_from_acf(flags);
  Scratch scratch = ScratchBegin(0, 0);
  i32 fd = open(cstr_from_str8(scratch.arena, filepath), access_flags,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  ScratchEnd(scratch);
  if(fd < 0) {
    fd = 0;
  } else {
    // NOTE(lb): there is no other way that i know of to keep track of the path
    // other than tracking it myself. Getting the fullpath would require either
    // allocating more memory (preferred but still shit) or using the system arena
    // (can't remove the path from the arena without introducing internal fragmentation),
    // so paths will be relative to the cwd until a better solution.
    bsd_filemap[fd] = filepath;
  }

  OS_Handle res = {(u64)fd};
  return res;
}

fn bool fs_close(OS_Handle fd) {
  bsd_filemap[fd.h[0]].str = 0;
  bsd_filemap[fd.h[0]].size = 0;
  return !close((i32)fd.h[0]);
}

fn String8 fs_path_from_handle(Arena *arena, OS_Handle fd) {
  Unused(arena);
  return bsd_filemap[fd.h[0]];
}

fn bool fs_copy(String8 source, String8 destination) {
  Scratch scratch = ScratchBegin(0, 0);
  i32 src = open(cstr_from_str8(scratch.arena, source), O_RDONLY, 0);
  i32 dest = open(cstr_from_str8(scratch.arena, destination),
                  O_WRONLY | O_CREAT, 0644);
  ScratchEnd(scratch);

  struct stat stat_src = {0};
  fstat(src, &stat_src);
  off_t offset = 0;
  isize bytes_copied = copy_file_range(src, &offset, dest, &offset,
                                       (usize)stat_src.st_size, 0);
  fchmod(dest, stat_src.st_mode & 0777);

  fdatasync(dest);
  close(src);
  close(dest);

  scratch = ScratchBegin(0, 0);
  Assert(access(cstr_from_str8(scratch.arena, destination), F_OK) == 0);
  ScratchEnd(scratch);

  return bytes_copied == stat_src.st_size;
}

// =============================================================================
i32 main(i32 argc, char **argv) {
  i32 mib[2] = { CTL_HW, HW_NCPU };
  usize len = sizeof(unx_state.info.core_count);
  sysctl(mib, 2, &unx_state.info.core_count, &len, 0, 0);

  mib[1] = HW_PHYSMEM;
  len = sizeof(unx_state.info.total_memory);
  sysctl(mib, 2, &unx_state.info.total_memory, &len, 0, 0);

  isize page_sizes[3] = {0};
  getpagesizes((usize*)page_sizes, 3);
  unx_state.info.page_size = page_sizes[0];
#ifdef USE_SUPERLARGE_PAGES
  unx_state.info.hugepage_size = page_sizes[2];
#else
  unx_state.info.hugepage_size = page_sizes[1];
#endif

  unx_state.info.hostname = unx_gethostname();
  unx_state.arena = ArenaBuild();

  CmdLine cli = {0};
  cli.count = argc - 1;
  cli.exe = str8_from_cstr(argv[0]);
  cli.args = New(unx_state.arena, String8, argc - 1);
  for (isize i = 1; i < argc; ++i) {
    cli.args[i - 1] = str8_from_cstr(argv[i]);
  }

  struct timespec tms;
  struct tm lt = {0};

  (void)clock_gettime(CLOCK_REALTIME, &tms);
  (void)localtime_r(&tms.tv_sec, &lt);
  unx_state.unix_utc_offset = (u64)lt.tm_gmtoff;

#if OS_GUI
  unx_gfx_init();
#endif

  start(&cli);
}
