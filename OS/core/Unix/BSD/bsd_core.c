String8 bsd_filemap[MEMFILES_ALLOWED] = {0};

// =============================================================================
// Memory allocation
fn void* os_reserve_huge(usize size) {
  void *res = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED_SUPER, -1, 0);
  if (res == MAP_FAILED) {
    res = 0;
  }

  return res;
}

// =============================================================================
// File reading and writing/appending
fn OS_Handle fs_open(String8 filepath, OS_AccessFlags flags) {
  i32 access_flags = 0;

  if((flags & OS_acfRead) && (flags & OS_acfWrite)) {
    access_flags |= O_RDWR;
  } else if(flags & OS_acfRead) {
    access_flags |= O_RDONLY;
  } else if(flags & OS_acfWrite) {
    access_flags |= O_WRONLY | O_CREAT | O_TRUNC;
  }
  if(flags & OS_acfAppend) { access_flags |= O_APPEND | O_CREAT; }

  i32 fd = open((char*)filepath.str, access_flags,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
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
  if (close(fd.h[0]) == 0) {
    bsd_filemap[fd.h[0]].str = 0;
    bsd_filemap[fd.h[0]].size = 0;
    return true;
  } else {
    return false;
  }
}

fn String8 fs_path_from_handle(Arena *arena, OS_Handle fd) {
  return bsd_filemap[fd.h[0]];
}

fn bool fs_copy(String8 source, String8 destination) {
  return false;
}

// =============================================================================
i32 main(i32 argc, char **argv) {
  i32 mib[2] = { CTL_HW, HW_NCPU };
  usize len = sizeof(unx_state.info.core_count);
  sysctl(mib, 2, &unx_state.info.core_count, &len, 0, 0);

  mib[1] = HW_PHYSMEM;
  len = sizeof(unx_state.info.total_memory);
  sysctl(mib, 2, &unx_state.info.total_memory, &len, 0, 0);

  usize page_sizes[3] = {0};
  getpagesizes(page_sizes, 3);
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

  unx_state.unix_utc_offset = lt.tm_gmtoff;

  start(&cli);
}
