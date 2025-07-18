inline fn bool is_leap_year(u32 year) {
    return (year % 4 == 0 && year % 100 != 0) ||
           (year % 400 == 0);
}

fn DateTime datetime_from_time64(time64 t) {
  DateTime res = {0};
  res.ms     = (u16)(t & bitmask10);
  res.second = (u8)((t >> 10) & bitmask6);
  res.minute = (u8)((t >> 16) & bitmask6);
  res.hour   = (u8)((t >> 22) & bitmask5);
  res.day    = (u8)((t >> 27) & bitmask5);
  res.month  = (u8)((t >> 32) & bitmask4);
  res.year   = ((t >> 36) & ~bit28) * (t >> 63 ? 1 : -1);
  return res;
}

fn DateTime datetime_from_unix(u64 timestamp) {
  DateTime dt = {.year = 1970, .month = 1, .day = 1};

  for (u64 secondsXyear = is_leap_year(dt.year)
                          ? UNIX_LEAP_YEAR
                          : UNIX_YEAR;
       timestamp >= secondsXyear;
       ++dt.year, timestamp -= secondsXyear,
                  secondsXyear = is_leap_year(dt.year)
                                 ? UNIX_LEAP_YEAR
                                 : UNIX_YEAR);

  while (1) {
    u8 days = daysXmonth[dt.month - 1];
    if (dt.month == 2 && is_leap_year(dt.year)) {
      ++days;
    }

    u64 secondsXmonth = days * UNIX_DAY;
    if (timestamp < secondsXmonth) {
      break;
    }

    timestamp -= secondsXmonth;
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

fn time64 time64_from_datetime(DateTime *dt) {
  time64 res = (dt->year >= 0 ? 1ULL << 63 : 0);
  res |= (u64)((dt->year >= 0 ? dt->year : -dt->year) & ~bit28) << 36;
  res |= (u64)(dt->month) << 32;
  res |= (u64)(dt->day) << 27;
  res |= (u64)(dt->hour) << 22;
  res |= (u64)(dt->minute) << 16;
  res |= (u64)(dt->second) << 10;
  res |= (u64)dt->ms;
  return res;
}

fn time64 time64_from_unix(u64 timestamp) {
  time64 res = 1ULL << 63;

  u32 year = 1970;
  for (u64 secondsXyear = is_leap_year(year)
                          ? UNIX_LEAP_YEAR
                          : UNIX_YEAR;
       timestamp >= secondsXyear;
       ++year, timestamp -= secondsXyear,
               secondsXyear = is_leap_year(year)
                              ? UNIX_LEAP_YEAR
                              : UNIX_YEAR);

  res |= ((u64)year & ~(1 << 27)) << 36;
  u64 month = 1;
  while (1) {
    u8 days = daysXmonth[month - 1];
    if (month == 2 && is_leap_year(year)) {
      ++days;
    }

    u64 secondsXmonth = days * UNIX_DAY;
    if (timestamp < secondsXmonth) {
      break;
    }

    timestamp -= secondsXmonth;
    ++month;
  }
  res |= month << 32;

  res |= (timestamp / UNIX_DAY + 1) << 27;
  timestamp %= UNIX_DAY;

  res |= (timestamp / UNIX_HOUR) << 22;
  timestamp %= UNIX_HOUR;

  res |= (timestamp / UNIX_MINUTE) << 16;
  timestamp %= UNIX_MINUTE;

  res |= timestamp << 10;

  return res;
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
    unix_time += daysXmonth[month - 1] * UNIX_DAY;
    if (month == 2 && is_leap_year(dt->year)) {
      unix_time += UNIX_DAY;
    }
  }

  return unix_time;
}

fn u64 unix_from_time64(time64 timestamp) {
  u64 res = 0;
  if (!(timestamp >> 63)) { return 0; }

  i32 year = ((timestamp >> 36) & ~(1 << 27));
  if (year < 1970) { return 0; }
  for (i32 i = 1970; i < year; ++i) {
    res += is_leap_year(i) ? UNIX_LEAP_YEAR : UNIX_YEAR;
  }

  u8 time_month = (u8)((timestamp >> 32) & bitmask4);
  for (u8 month = 1; month < time_month; ++month) {
    res += daysXmonth[month - 1] * UNIX_DAY;
    if (month == 2 && is_leap_year(year)) {
      res += UNIX_DAY;
    }
  }

  res += (((timestamp >> 27) & bitmask5) - 1) * UNIX_DAY;
  res += ((timestamp >> 22) & bitmask5) * UNIX_HOUR;
  res += ((timestamp >> 16) & bitmask6) * UNIX_MINUTE;
  res += (timestamp >> 10) & bitmask6;
  return res;
}
