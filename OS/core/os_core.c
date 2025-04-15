// =============================================================================
// Logging
fn void os_print(OS_LogLevel level, const char *caller, const char *file,
                 i32 line, const char *fmt, ...) {
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

 print_str:
  va_list args;
  Scratch scratch = ScratchBegin(0, 0);
  va_start(args, fmt);
  String8 msg = _str8_format(scratch.arena, fmt, args);
  printf("%.*s\n", Strexpand(msg));
  va_end(args);
  ScratchEnd(scratch);
}

// =============================================================================
inline fn void fs_fwrite(File *file, String8 content) {
  if (file->prop.size < (usize)content.size) {
    fs_fresize(file, content.size);
  }
  memZero(file->content + content.size, ClampBot(0, (isize)file->prop.size -
                                                    (isize)content.size));
  (void)memCopy(file->content, content.str, content.size);
}
