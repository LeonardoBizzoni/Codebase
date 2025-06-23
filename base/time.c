inline fn bool is_leap_year(u32 year) {
    return (year % 4 == 0 && year % 100 != 0) ||
           (year % 400 == 0);
}

fn DateTime datetime_from_time64(time64 t) {
  DateTime r = {0};
  r.ms = t % 1000;
  t /= 1000;
  r.second = t%61;
  t /= 61;
  r.minute = t%60;
  t /= 60;
  r.hour = t%24;
  t /= 24;
  r.day = t%31 + 1;
  t /= 31;
  r.month = t%12 + 1;
  t /= 12;
  r.year = (i32)t;
  return r;
}

fn time64 time64_from_datetime(DateTime *dt) {
  time64 r = 0;
  r += dt->year;
  r *= 12;
  r += dt->month - 1;
  r *= 31;
  r += dt->day - 1;
  r *= 24;
  r += dt->hour;
  r *= 60;
  r += dt->minute;
  r *= 61;
  r += dt->second;
  r *= 1000;
  r += dt->ms;
  return r;
}

fn DateTime datetime_from_unix(u64 timestamp) {
  DateTime dt = {.year = 1970, .month = 1, .day = 1};
  
  for (;;){
    u64 seconds_per_year = is_leap_year(dt.year) ? UNIX_LEAP_YEAR : UNIX_YEAR;
    if(timestamp < seconds_per_year) break;
    dt.year += 1;
    timestamp -= seconds_per_year;
    seconds_per_year = is_leap_year(dt.year) ? UNIX_LEAP_YEAR : UNIX_YEAR;
  }
  
  while (1) {
    u8 days = days_per_month[dt.month - 1];
    if (dt.month == 2 && is_leap_year(dt.year)) {
      ++days;
    }
    
    u64 seconds_per_month = days * UNIX_DAY;
    if (timestamp < seconds_per_month) {
      break;
    }
    
    timestamp -= seconds_per_month;
    ++dt.month;
  }
  
  dt.day += (u8)(timestamp / UNIX_DAY);
  timestamp %= UNIX_DAY;
  dt.hour = (u8)(timestamp / UNIX_HOUR);
  timestamp %= UNIX_HOUR;
  dt.minute = (u8)(timestamp / UNIX_MINUTE);
  timestamp %= UNIX_MINUTE;
  dt.second = (u8)timestamp;
  return dt;
}

fn u64 unix_from_datetime(DateTime *dt) {
  if (dt->year < 1970) { return 0; }
  u64 unix_time = ((dt->day - 1) * UNIX_DAY) +
                  (dt->hour * UNIX_HOUR) +
                  (dt->minute * UNIX_MINUTE) +
                  (dt->second);

  for (u32 year = 1970; year < (u32)dt->year; ++year) {
    unix_time += is_leap_year(year) ? UNIX_LEAP_YEAR : UNIX_YEAR;
  }
  
  for (u8 month = 1; month < dt->month; ++month) {
    unix_time += days_per_month[month - 1] * UNIX_DAY;
    if (month == 2 && is_leap_year(dt->year)) {
      unix_time += UNIX_DAY;
    }
  }
}


fn time64 time64_from_unix(u64 timestamp){
  DateTime date_time = datetime_from_unix(timestamp);
  time64 result = time64_from_datetime(&date_time);
  return result;
}

fn u64 unix_from_time64(time64 timestamp){
  DateTime date_time = datetime_from_time64(timestamp);
  u64 result = unix_from_datetime(&date_time);
  return result;
}
