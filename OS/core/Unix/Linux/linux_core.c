// =============================================================================
// System information retrieval
fn void lnx_parseMeminfo(void) {
  OS_Handle meminfo = os_fs_open(Strlit("/proc/meminfo"), OS_acfRead);
  StringBuilder lines = str8_split(unx_state.arena,
                                   os_fs_read_virtual(unx_state.arena,
                                                      meminfo, 4096), '\n');
  for (StringBuilderNode *curr_line = lines.first;
       curr_line;
       curr_line = curr_line->next) {
    StringBuilder ss = str8_split(unx_state.arena, curr_line->value, ':');
    for (StringBuilderNode *curr = ss.first; curr; curr = curr->next) {
      if (str8_eq(curr->value, Strlit("MemTotal"))) {
        curr = curr->next;
        unx_state.info.total_memory =
          KiB(1) *
          u64_from_str8(str8_split(unx_state.arena,
                                   str8_trim(curr->value),
                                   ' ').first->value);
      } else if (str8_eq(curr->value, Strlit("Hugepagesize"))) {
        curr = curr->next;
        unx_state.info.hugepage_size =
          (i64)(KiB(1) *
                u64_from_str8(str8_split(unx_state.arena,
                                         str8_trim(curr->value),
                                         ' ').first->value));
        return;
      }
    }
  }
}

// =============================================================================
// Memory allocation
fn void* os_reserve_huge(isize size) {
  Assert(size > 0);
  // TODO(lb): MAP_HUGETLB vs MAP_HUGE_2MB/MAP_HUGE_1GB?
  void *res = mmap(0, (usize)size, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
  if (res == MAP_FAILED) { res = 0; }
  return res;
}

// =============================================================================
// File reading and writing/appending
fn OS_Handle os_fs_open(String8 filepath, OS_AccessFlags flags) {
  i32 access_flags = O_CREAT | unx_flags_from_acf(flags);
  Scratch scratch = ScratchBegin(0, 0);
  i32 fd = open(cstr_from_str8(scratch.arena, filepath), access_flags,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  ScratchEnd(scratch);
  if(fd < 0) {
    fd = 0;
  }

  OS_Handle res = {{(u64)fd}};
  return res;
}

fn bool os_fs_close(OS_Handle fd) {
  return close((i32)fd.h[0]) == 0;
}

fn String8 os_fs_path_from_handle(Arena *arena, OS_Handle fd) {
  char path[PATH_MAX];
  isize len = snprintf(path, sizeof(path), "/proc/self/fd/%ld", fd.h[0]);
  return os_fs_readlink(arena, str8((u8 *)path, len));
}

fn bool os_fs_copy(String8 source, String8 destination) {
  struct stat stat_src = {0};

  Scratch scratch = ScratchBegin(0, 0);
  i32 src = open(cstr_from_str8(scratch.arena, source), O_RDONLY, 0);
  fstat(src, &stat_src);
  i32 dest = open(cstr_from_str8(scratch.arena, destination), O_WRONLY | O_CREAT, stat_src.st_mode);
  ScratchEnd(scratch);

  isize res = sendfile(dest, src, 0, (usize)stat_src.st_size);
  fchmod(dest, stat_src.st_mode & 0777);

  fsync(dest);
  close(src);
  close(dest);

  scratch = ScratchBegin(0, 0);
  Assert(access(cstr_from_str8(scratch.arena, destination), F_OK) == 0);
  ScratchEnd(scratch);

  return res != -1;
}

// =============================================================================
// Glibc missing wrappers
fn i32 lnx_sched_setattr(u32 policy, u64 runtime_ns, u64 deadline_ns, u64 period_ns) {
  struct lnx_sched_attr attr = {
    .size = sizeof(attr),
    .sched_policy = policy,
    .sched_flags = SCHED_FLAG_DL_OVERRUN,
    .sched_runtime = runtime_ns,
    .sched_deadline = deadline_ns,
    .sched_period = period_ns
  };
  return (i32)syscall(__NR_sched_setattr, syscall(__NR_gettid), &attr, 0);
}

fn void lnx_sched_set_deadline(u64 runtime_ns, u64 deadline_ns, u64 period_ns,
                               Func_Signal *deadline_miss_handler) {
  Assert(!lnx_sched_setattr(SCHED_DEADLINE, runtime_ns, deadline_ns, period_ns));
  //       \CPU time exceeded/
  (void)signal(SIGXCPU, deadline_miss_handler);
}

fn void lnx_sched_yield(void) {
  sched_yield();
}

// =============================================================================
// Signals
// TODO(lb): are there signals on windows?
fn void lnx_signal_send_private(i32 signal) {
  kill(getpid(), signal);
}

fn void lnx_signal_wait(i32 signal) {
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, signal);
  for (i32 signum = 0; signum != signal; ) {
    sigwait(&signal_set, &signum);
  }
}

// =============================================================================
fn void os_env_setup(void) {
  srand((u32)time(0));
  unx_state.info.core_count = (u8)get_nprocs();
  unx_state.info.page_size = getpagesize();
  unx_state.info.hostname = unx_gethostname();

  unx_state.arena = arena_build();
  pthread_mutex_init(&unx_state.primitive_lock, 0);
  lnx_parseMeminfo();

  struct timespec tms;
  struct tm lt = {0};
  (void)clock_gettime(CLOCK_REALTIME, &tms);
  (void)localtime_r(&tms.tv_sec, &lt);
  unx_state.unix_utc_offset = (u64)lt.tm_gmtoff;

  clock_gettime(CLOCK_MONOTONIC, &tms);
  unx_state.time_offset = tms.tv_sec * UNX_TIME_FREQ + tms.tv_nsec;

#if OS_GUI
  unx_gfx_init();
#endif
#if OS_SOUND
  lnx_snd_init();
#endif
}

#ifndef CBUILD_H
i32 main(i32 argc, char **argv) {
  os_env_setup();

  CmdLine cli = {0};
  cli.count = argc - 1;
  cli.exe = str8_from_cstr(argv[0]);
  cli.args = arena_push_many(unx_state.arena, String8, argc - 1);
  for (isize i = 1; i < argc; ++i) {
    cli.args[i - 1] = str8_from_cstr(argv[i]);
  }

  start(&cli);
  rhi_deinit();
  unx_gfx_deinit();
}
#endif
