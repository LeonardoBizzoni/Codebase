// =============================================================================
// Logging
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

fn void os_print(OS_LogLevel level, const char *caller, const char *file,
   i32 line, String8 str) {
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
  printf("%.*s\n", Strexpand(str));
}

#define Log(STR) os_print(OS_LogLevel_Log, __func__, __FILE__, __LINE__, (STR))
#if DEBUG
#  define Info(STR) os_print(OS_LogLevel_Info, __func__, __FILE__, __LINE__, (STR))
#  define Warn(STR) os_print(OS_LogLevel_Warn, __func__, __FILE__, __LINE__, (STR))
#  define Err(STR) os_print(OS_LogLevel_Error, __func__, __FILE__, __LINE__, (STR))
#else
#  define Info(STR)
#  define Warn(STR)
#  define Err(STR)
#endif
