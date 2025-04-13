#ifndef BASE_CLOCK_H
#define BASE_CLOCK_H

typedef struct {
  i32 year;
  u8 month;
  u8 day;
  u8 hour;
  u8 minute;
  u8 second;
  u16 ms;
} DateTime;

typedef u64 time64;

#define UNIX_MINUTE 60
#define UNIX_HOUR 3600
#define UNIX_DAY 86400
#define UNIX_WEEK 604800

#define UNIX_YEAR 31536000
#define UNIX_LEAP_YEAR 31622400

global const u8 daysXmonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

inline fn bool is_leap_year(u32 year);

// =============================================================================
// Time format conversions
fn DateTime datetime_from_time64(time64 timestamp);
fn DateTime datetime_from_unix(u64 timestamp);

fn time64 time64_from_datetime(DateTime *dt);
fn time64 time64_from_unix(u64 timestamp);

fn u64 unix_from_datetime(DateTime *dt);
fn u64 unix_from_time64(time64 timestamp);

#define TimedScope                                                           \
  for (OS_Handle __start = os_timer_start(), __end = {0};                    \
       __end.h[0] == 0; __end = os_timer_start(),                            \
           printf("%ldms (%ldns)\n",                                         \
                  os_timer_elapsed(OS_TimerGranularity_ms, __start, __end),  \
                  os_timer_elapsed(OS_TimerGranularity_ns, __start, __end)))

#define TimeTrack(expr) TimedScope { (void)(expr); }

#endif // BASE_CLOCK_H
