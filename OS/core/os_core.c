fn void os_print(OS_LogLevel level, const char *caller, const char *file,
                 i32 line, const char *fmt, ...) {
  va_list args;
  switch (level) {
    case OS_LogLevel_Info: {
      printf(ANSI_COLOR_CYAN "[INFO ");
    } break;
    case OS_LogLevel_Warn: {
      printf(ANSI_COLOR_YELLOW "[WARNING ");
    } break;
    case OS_LogLevel_Error: {
      printf(ANSI_COLOR_RED "[ERROR ");
    } break;
    default: printf(ANSI_COLOR_RESET); goto print_str;
  }
  printf("%s:%s@L%d] " ANSI_COLOR_RESET, file, caller, line);

 print_str: ;
  Scratch scratch = ScratchBegin(0, 0);
  va_start(args, fmt);
  String8 msg = _str8_format(scratch.arena, fmt, args);
  printf("%.*s\n", Strexpand(msg));
  va_end(args);
  ScratchEnd(scratch);
}

fn void os_fs_fwrite(File *file, String8 content) {
  if (file->prop.size < content.size) {
    os_fs_fresize(file, content.size);
  }
  memzero(file->content + content.size, ClampBot(0, (isize)file->prop.size -
                                                    (isize)content.size));
  (void)memcopy(file->content, content.str, content.size);
}

fn u64 os_timer_elapsed(OS_Handle start, OS_TimerGranularity unit) {
  OS_Handle now = os_timer_start();
  u64 elapsed = os_timer_elapsed_between(start, now, unit);
  os_timer_free(now);
  return elapsed;
}

fn bool os_timer_reached(OS_Handle timer, u64 how_much, OS_TimerGranularity unit) {
  return os_timer_elapsed(timer, unit) >= how_much;
}

fn OS_FileIter* os_fs_iter_begin_filtered(Arena *arena, String8 path, OS_FileType allowed) {
  OS_FileIter *os_iter = os_fs_iter_begin(arena, path);
  os_iter->filter_allowed = allowed;
  return os_iter;
}

fn NetInterface os_net_interface_lookup(String8 interface) {
  NetInterface res = {0};

  Scratch scratch = ScratchBegin(0, 0);
  NetInterfaceList inters = os_net_interfaces(scratch.arena);
  for (NetInterface *curr = inters.first; curr; curr = curr->next) {
    if (str8_eq(curr->strip, interface) || str8_eq(curr->name, interface)) {
      memcopy(&res, curr, sizeof(NetInterface));
      break;
    }
  }
  ScratchEnd(scratch);

  return res;
}
