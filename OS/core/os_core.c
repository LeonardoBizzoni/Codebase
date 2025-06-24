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

inline fn void fs_fwrite(File *file, String8 content) {
  if (file->prop.size < (usize)content.size) {
    fs_fresize(file, content.size);
  }
  memzero(file->content + content.size, ClampBot(0, (isize)file->prop.size -
                                                    (isize)content.size));
  (void)memcopy(file->content, content.str, content.size);
}

fn void os_socket_send_format(OS_Socket *socket, char *format, ...) {
  Scratch scratch = ScratchBegin(0, 0);
  va_list args;
  va_start(args, format);
  os_socket_send_str8(socket, _str8_format(scratch.arena, format, args));
  va_end(args);
  ScratchEnd(scratch);
}

inline fn bool os_timer_elapsed_time(OS_TimerGranularity unit, OS_Handle timer,
                                     u64 how_much) {
  OS_Handle now = os_timer_start();
  u64 elapsed = os_timer_elapsed_start2end(unit, timer, now);
  os_timer_free(now);
  return elapsed >= how_much;
}

fn time64 os_local_now(){
  DateTime now = os_utc_now();
  DateTime local_time = os_local_from_utc_time(&now);
  time64 result = time64_from_datetime(&local_time);
  return result;
}

fn DateTime os_local_date_time_now(){
  DateTime now = os_utc_now();
  DateTime local_time = os_local_from_utc_time(&now);
  return local_time;
}

fn time64 os_local_from_utc_time64(time64 t){
  DateTime date_time = datetime_from_time64(t);
  DateTime local_time = os_local_from_utc_time(&date_time);
  time64 result = time64_from_datetime(&local_time);
  return result;
}

fn time64 os_utc_time64_now(){
  DateTime date_time = os_utc_now();
  time64 result = time64_from_datetime(&date_time);
  return result;
}

fn DateTime os_utc_localized_date_time(i8 utc_offset) {
  DateTime res = os_utc_now();
  res.hour += utc_offset;
  if (res.hour < 0) {
    res.day -= 1;
    res.hour += 24;
  } else if (res.hour > 23) {
    res.day += 1;
    res.hour -= 24;
  }
  
  if (res.day < 1) {
    res.month -= 1;
    res.day = days_per_month[res.month - 1];
  } else if (res.day > days_per_month[res.month - 1]) {
    res.month += 1;
    res.day = 1;
  }
  
  if (res.month < 1) {
    res.year -= 1;
    res.month = 12;
  } else if (res.month > 12) {
    res.year += 1;
    res.month = 1;
  }
  
  return res;
}

fn time64 os_utc_localized_time64(i8 utc_offset){
  DateTime date_time = os_utc_localized_date_time(utc_offset);
  time64 result = time64_from_datetime(&date_time);
  return result;
}

fn time64 os_utc_from_local_time64(time64 t){
  DateTime date_time = datetime_from_time64(t);
  DateTime local_time = os_local_from_utc_time(&date_time);
  time64 result = time64_from_datetime(&local_time);
  return result;
}