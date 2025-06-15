#include "string.h"
#include "time.h"

#include <stdlib.h>

// `size` and `cstr` are to be considered immutable
// =============================================================================
// Unicode codepoint
fn Codepoint decodeUTF8(u8 *glyph_start) {
  Codepoint res = {0};

  if ((*glyph_start & 0x80) == 0) {
    res.codepoint = *glyph_start;
    res.size = 1;
  } else if ((*glyph_start & 0xE0) == 0xC0) {
    res.codepoint = glyph_start[1] & 0x3F;
    res.codepoint |= (glyph_start[0] & 0x1F) << 6;
    res.size = 2;
  } else if ((*glyph_start & 0xF0) == 0xE0) {
    res.codepoint = glyph_start[2] & 0x3F;
    res.codepoint |= (glyph_start[1] & 0x3F) << 6;
    res.codepoint |= (glyph_start[0] & 0xf) << 12;
    res.size = 3;
  } else if ((*glyph_start & 0xF8) == 0xF0) {
    res.codepoint = glyph_start[3] & 0x3F;
    res.codepoint |= (glyph_start[2] & 0x3F) << 6;
    res.codepoint |= (glyph_start[1] & 0x3F) << 12;
    res.codepoint |= (glyph_start[0] & 0x7) << 18;
    res.size = 4;
  } else {
    Assert(false);
  }

  return res;
}

fn Codepoint decodeUTF16(u16 *glyph_start) {
  Codepoint res = {0};

  if (glyph_start[0] <= 0xD7FF ||
      (glyph_start[0] >= 0xE000 && glyph_start[0] <= 0xFFFF)) {
    res.size = 1;
    res.codepoint = *glyph_start;
  } else if ((glyph_start[0] >= 0xD800 && glyph_start[0] <= 0xDBFF) &&
             (glyph_start[1] >= 0xDC00 && glyph_start[1] <= 0xDFFF)) {
    res.size = 2;
    res.codepoint =
    ((glyph_start[0] - 0xD800) << 10) + (glyph_start[1] - 0xDC00) + 0x10000;
  } else {
    Assert(false);
  }

  return res;
}

inline fn Codepoint decodeUTF32(u32 *glyph_start) {
  Codepoint res = {
    .codepoint = *glyph_start,
    .size = 1,
  };
  return res;
}

fn u8 encodeUTF8(u8 *res, Codepoint cp) {
  if (cp.codepoint <= 0x7F) {
    res[0] = (u8)cp.codepoint;
    return 1;
  } else if (cp.codepoint <= 0x7FF) {
    res[0] = (u8)(0xC0 | (cp.codepoint >> 6));
    res[1] = 0x80 | (cp.codepoint & 0x3F);
    return 2;
  } else if (cp.codepoint <= 0xFFFF) {
    res[0] = (u8)(0xE0 | (cp.codepoint >> 12));
    res[1] = 0x80 | ((cp.codepoint >> 6) & 0x3F);
    res[2] = 0x80 | (cp.codepoint & 0x3F);
    return 3;
  } else if (cp.codepoint <= 0x10FFFF) {
    res[0] = (u8)(0xF0 | (cp.codepoint >> 18));
    res[1] = 0x80 | ((cp.codepoint >> 12) & 0x3F);
    res[2] = 0x80 | ((cp.codepoint >> 6) & 0x3F);
    res[3] = 0x80 | (cp.codepoint & 0x3F);
    return 4;
  } else {
    Assert(false);
    // NOTE(km): return type is unsigned
    return -1;
  }
}

fn u8 encodeUTF16(u16 *res, Codepoint cp) {
  if (cp.codepoint <= 0xD7FF ||
      (cp.codepoint >= 0xE000 && cp.codepoint <= 0xFFFF)) {
    res[0] = (u16)cp.codepoint;
    return 1;
  } else if (cp.codepoint >= 0x10000 && cp.codepoint <= 0x10FFFF) {
    res[0] = (u16)(((cp.codepoint - 0x10000) >> 10) + 0xD800);
    res[1] = ((cp.codepoint - 0x10000) & 0x3FF) + 0xDC00;
    return 2;
  } else {
    Assert(false);
    // NOTE(km): same thing here
    return -1;
  }
}

fn u8 encodeUTF32(u32 *res, Codepoint cp) {
  res[0] = cp.codepoint;
  return 1;
}

// =============================================================================
// UTF-8 string streams
fn void strstream_append_str(Arena *arena, StringStream *strlist, String8 other) {
  strlist->node_count += 1;
  strlist->total_size += other.size;
  StringNode *str = New(arena, StringNode);
  str->value = other;
  DLLPushBack(strlist->first, strlist->last, str);
}

fn void strstream_append_stream(StringStream *strlist, StringStream other) {
  if (!other.first) { return; }
  strlist->total_size += other.total_size;
  strlist->node_count += other.node_count;
  DLLPushBack(strlist->first, strlist->last, other.first);
}

fn String8 strstream_join_char(Arena *arena, StringStream strlist, char ch) {
  String8 res = str8(New(arena, u8, strlist.total_size + strlist.node_count - 1),
                     strlist.total_size + strlist.node_count - 1);

  usize i = 0;
  for (StringNode *curr = strlist.first; curr && curr->next; curr = curr->next) {
    memcopy(&res.str[i], curr->value.str, curr->value.size);
    i += curr->value.size;
    res.str[i++] = ch;
  }
  memcopy(&res.str[i], strlist.last->value.str, strlist.last->value.size);

  return res;
}

fn String8 strstream_join_str(Arena *arena, StringStream strlist, String8 str) {
  String8 res = str8(New(arena, u8, strlist.total_size +
                                    str.size * (strlist.node_count - 1)),
                     strlist.total_size + str.size * (strlist.node_count - 1));

  usize i = 0;
  for (StringNode *curr = strlist.first; curr && curr->next; curr = curr->next) {
    memcopy(&res.str[i], curr->value.str, curr->value.size);
    i += curr->value.size;
    memcopy(&res.str[i], str.str, str.size);
    i += str.size;
  }
  memcopy(&res.str[i], strlist.last->value.str, strlist.last->value.size);

  return res;
}

// =============================================================================
// UTF-8 string
fn String8 str8(u8 *chars, isize len) {
#if CPP
  String8 res(chars, len);
#else
  String8 res = {chars, len};
#endif
  return res;
}

fn String8 str8_from_stream(Arena *arena, StringStream stream) {
  u8 *str = New(arena, u8, stream.total_size);
  u8 *ptr = str;
  for (StringNode *curr = stream.first; curr; curr = curr->next) {
    memcopy(ptr, curr->value.str, curr->value.size);
    ptr += curr->value.size;
  }

  return str8(str, stream.total_size);
}

fn String8 str8_from_char(Arena *arena, char ch) {
  String8 res = str8(New(arena, u8), 1);
  res.str[0] = ch;
  return res;
}

fn String8 str8_from_cstr(char *chars) {
  return str8((u8*)chars, str8_len(chars));
}

fn String8 str8_from_i64(Arena *arena, i64 n) {
  i64 sign = n;
  if (n < 0) {
    n = -n;
  }

  usize i = 0, approx = 30;
  u8 *str = New(arena, u8, approx);
  for (; n > 0; ++i, n /= 10) {
    str[i] = n % 10 + '0';
  }

  if (sign < 0) {
    str[i++] = '-';
  }

  for (usize j = 0, k = i - 1; j < k; ++j, --k) {
    u8 tmp = str[k];
    str[k] = str[j];
    str[j] = tmp;
  }

  arenaPop(arena, approx - i);

  return str8(str,i);
}

fn String8 str8_from_u64(Arena *arena, u64 n) {
  usize i = 0, approx = 30;
  u8 *str = New(arena, u8, approx);
  for (; n > 0; ++i, n /= 10) {
    str[i] = n % 10 + '0';
  }

  for (usize j = 0, k = i - 1; j < k; ++j, --k) {
    u8 tmp = str[k];
    str[k] = str[j];
    str[j] = tmp;
  }

  arenaPop(arena, approx - i);

  return str8(str, i);
}

fn String8 str8_from_f64(Arena *arena, f64 n) {
  usize approx = 100, size = 0;
  u8 *str = New(arena, u8, approx);

  // TODO: maybe implement `sprintf`?
  size = sprintf((char *)str, "%f", n);
  arenaPop(arena, approx - size);

  return str8(str, size);
}

fn String8 str8_from_datetime(Arena *arena, DateTime dt) {
  return str8_format(arena, "%02d/%02d/%04d %02d:%02d:%02d.%02d",
                     dt.day, dt.month, dt.year,
                     dt.hour, dt.minute, dt.second, dt.ms);
}

fn String8 str8_from_unixtime(Arena *arena, u64 unix_timestamp) {
  DateTime dt = datetime_from_unix(unix_timestamp);
  return str8_from_datetime(arena, dt);
}

fn String8 str8_from_time64(Arena *arena, time64 timestamp) {
  DateTime dt = datetime_from_time64(timestamp);
  return str8_from_datetime(arena, dt);
}

inline fn String8 str8_prefix(String8 s, isize end) {
  return str8(s.str, ClampTop(s.size, end));
}

inline fn String8 str8_postfix(String8 s, isize start) {
  start = ClampTop(start, s.size);
  return str8(s.str + start, s.size - start);
}

fn String8 str8_substr(String8 s, isize first, isize opl) {
  opl = ClampTop(opl, s.size);
  String8 result = str8_range(s.str+first, s.str+opl);
  return result;
}

fn String8 str8_range(u8 *first, u8 *opl) {
  String8 result = {first, (isize)(opl - first)};
  return result;
}

fn String8 str8_lcs(Arena *arena, String8 s1, String8 s2) {
#define at(m,i,j) m.x[(i)*m.n + (j)]
  typedef struct Table Table;
  struct Table {
    usize *x;
    usize m;
    usize n;
  };

  usize m = s1.size + 1;
  usize n = s2.size + 1;

  Table c = {0};
  c.x = New(arena, usize, m*n);
  c.m = m;
  c.n = n;

  for(usize i = 1; i < m; ++i) {
    for(usize j = 1; j < n; ++j) {
      if(s1.str[i-1] == s2.str[j-1]) {
        at(c,i,j) = at(c,i-1,j-1) + 1;
      } else {
        at(c,i,j) = Max(at(c,i-1,j), at(c,i,j-1));
      }
    }
  }

  usize size = at(c,m-1,n-1);
  u8 *str = New(arena, u8, size);
  usize idx = size - 1;
  for(usize i = m - 1, j = n - 1; i > 0 && j > 0;) {
    if(at(c,i,j) == at(c,i-1,j)) {
      --i;
    } else if(at(c,i,j) == at(c,i,j-1)) {
      --j;
    } else {
      str[idx] = s1.str[i - 1];
      --idx;
      --i;
      --j;
    }
  }
#undef at
  return str8(str, size);
}

fn String8 str8_to_upper(Arena *arena, String8 s) {
  String8 res = {New(arena, u8, s.size), s.size};

  for (isize i = 0; i < s.size; ++i) {
    res.str[i] = char_to_upper(s.str[i]);
  }

  return res;
}

fn String8 str8_to_lower(Arena *arena, String8 s) {
  String8 res = {New(arena, u8, s.size), s.size};

  for (isize i = 0; i < s.size; ++i) {
    res.str[i] = char_to_lower(s.str[i]);
  }

  return res;
}

fn String8 str8_to_capitalized(Arena *arena, String8 s) {
  String8 res = {New(arena, u8, s.size), s.size};

  res.str[0] = char_to_upper(s.str[0]);
  for (isize i = 1; i < s.size; ++i) {
    if (char_is_space(s.str[i])) {
      res.str[i] = s.str[i];
      ++i;
      res.str[i] = char_to_upper(s.str[i]);
    } else {
      res.str[i] = char_to_lower(s.str[i]);
    }
  }

  return res;
}

fn String8 str8_trim(String8 s) {
  u8 *ptr = s.str;
  u8 *opl = s.str+s.size;
  for(;ptr < opl && char_is_whitespace(*ptr); ptr += 1);
  for(;opl > ptr && char_is_whitespace(*(opl-1)); opl -= 1);
  String8 result = str8_range(ptr, opl);
  return result;
}

fn String8 str8_trim_left(String8 s) {
  u8 *ptr = s.str;
  u8 *opl = s.str+s.size;
  for(;ptr < opl && char_is_whitespace(*ptr); ptr += 1);
  String8 result = str8_range(ptr, opl);
  return result;
}

fn String8 str8_trim_right(String8 s) {
  u8 *first = s.str;
  u8 *opl = s.str+s.size;
  for(;opl > first && char_is_whitespace(*(opl-1)); opl -= 1);
  String8 result = str8_range(first, opl);
  return result;
}

fn String8 str8_format(Arena *arena, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  String8 res = _str8_format(arena, fmt, args);
  va_end(args);

  return res;
}

fn String8 _str8_format(Arena *arena, const char *fmt, va_list args) {
  va_list args2;
  va_copy(args2, args);
  u32 needed_bytes = vsnprintf(0, 0, fmt, args2) + 1;

  String8 res;
  res.str = New(arena, u8, needed_bytes);
  res.size = vsnprintf((char *)res.str, needed_bytes, fmt, args);
  res.str[res.size] = 0;

  va_end(args2);
  return res;
}

fn StringStream str8_split(Arena *arena, String8 s, char ch) {
  StringStream res = {0};

  isize prev = 0;
  for (isize i = 0; i < s.size;) {
    if (s.str[i] == ch) {
      if (prev != i) {
        strstream_append_str(arena, &res, str8_substr(s, prev, i));
      }

      do {
        prev = ++i;
      } while (s.str[i] == ch);
    } else {
      ++i;
    }
  }

  if (prev != s.size) {
    strstream_append_str(arena, &res, str8_substr(s, prev, s.size));
  }

  return res;
}

fn usize str8_find_first_ch(String8 s, char ch) {
  for (u8 *curr = s.str; curr < s.str + s.size + 1; ++curr) {
    if (*curr == ch) {
      return curr - s.str;
    }
  }

  return 0;
}

fn usize str8_find_first_str8(String8 haystack, String8 needle) {
  if (haystack.size < needle.size) {
    return 0;
  }

  for (isize i = 0; i < haystack.size; ++i) {
    if (haystack.str[i] == needle.str[0]) {
      for (isize j = 0; j < needle.size; ++j) {
        if (haystack.str[i + j] != needle.str[j]) {
          goto outer;
        }
      }

      return i;
    }

    outer: ;
  }

  return 0;
}

/* Djb2: http://www.cse.yorku.ca/~oz/hash.html */
fn usize str8_hash(String8 s) {
  usize hash = 5381;
  for (isize i = 0; i < s.size; ++i) {
    hash = (hash << 5) + hash + s.str[i];
  }

  return hash;
}

fn isize str8_len(char *chars) {
  char *start = chars;
  for (; *start; ++start);
  return start - chars;
}

fn bool str8_eq(String8 s1, String8 s2) {
  if (s1.size != s2.size) {
    return false;
  }

  for (isize i = 0; i < s1.size; ++i) {
    if (s1.str[i] != s2.str[i]) {
      return false;
    }
  }
  return true;
}

fn bool str8_eq_cstr(String8 s, const char *cstr) {
  if (s.size == 0 && !cstr) {
    return true;
  } else if (!cstr || s.size == 0) {
    return false;
  }

  isize i = 0;
  for (; i < s.size; ++i) {
    if (s.str[i] != cstr[i]) {
      return false;
    }
  }
  return !cstr[i];
}

fn bool str8_ends_with_ch(String8 s, char ch) { return s.str[s.size - 1] == ch; }

fn bool str8_ends_with_str8(String8 s, String8 needle) {
  if (needle.size > s.size) { return false; }
  for (isize i = s.size - 1, j = needle.size - 1; j >= 0; --i, --j) {
    if (s.str[i] != needle.str[j]) { return false; }
  }
  return true;
}

fn bool str8_contains(String8 s, char ch) {
  for (u8 *curr = s.str; curr < s.str + s.size + 1; ++curr) {
    if (*curr == ch) {
      return true;
    }
  }
  return false;
}

fn bool str8_is_integer(String8 s) {
  for (u8 *curr = s.str; curr < s.str + s.size; ++curr) {
    if (!char_is_digit(*curr)) {
      return false;
    }
  }

  return true;
}

fn bool str8_is_integer_signed(String8 s) {
  u8 *curr = s.str;
  if (*curr == '-' || *curr == '+') {
    ++curr;
  }

  for (; curr < s.str + s.size; ++curr) {
    if (!char_is_digit(*curr)) {
      return false;
    }
  }

  return true;
}

fn bool str8_is_floating(String8 s) {
  bool decimal_found = false;
  u8 *curr = s.str;
  if (*curr == '-' || *curr == '+') {
    ++curr;
  }

  for (; curr < s.str + s.size; ++curr) {
    if (!char_is_digit(*curr)) {
      if (*curr == '.' && !decimal_found) {
        decimal_found = true;
      } else {
        return false;
      }
    }
  }

  return true;
}

fn bool str8_is_numerical(String8 s) {
  return str8_is_floating(s) || str8_is_integer(s);
}

fn char* cstr_from_str8(Arena *arena, String8 str) {
  char *res = New(arena, char, str.size + 1);
  memcopy(res, str.str, str.size);
  res[str.size] = 0;
  return res;
}

fn bool cstr_eq(char *s1, char *s2) {
  if (s1 == s2) {
    return true;
  }
  if (!s1 || !s2) {
    return false;
  }

  char *it1 = s1, *it2 = s2;
  for (; *it1 && *it2; ++it1, ++it2) {
    if (*it1 != *it2) {
      return false;
    }
  }
  return !*it1 && !*it2;
}

fn i64 i64_from_str8(String8 s) {
  i64 res = 0, decimal = 1;

  Assert(str8_is_integer_signed(s));
  for (u8 *curr = s.str + s.size - 1; curr > s.str; --curr, decimal *= 10) {
    res += (*curr - '0') * decimal;
  }

  if (s.str[0] == '-') {
    return -res;
  } else if (s.str[0] == '+') {
    return res;
  } else {
    return res + (s.str[0] - '0') * decimal;
  }
}

fn u64 u64_from_str8(String8 s) {
  i64 res = 0, decimal = 1;

  Assert(str8_is_integer(s));
  for (u8 *curr = s.str + s.size - 1; curr >= s.str; --curr, decimal *= 10) {
    res += (*curr - '0') * decimal;
  }

  return res;
}

fn f64 f64_from_str8(String8 s) {
  Scratch scratch = ScratchBegin(0, 0);
  f64 res = strtod(cstr_from_str8(scratch.arena, s), 0);
  ScratchEnd(scratch);
  return res;
}

fn bool char_is_whitespace(u8 ch) { return ch == ' ' || ch == '\t' ||
                                           ch == '\n' || ch == '\r'; }
fn bool char_is_space(u8 ch) { return ch == ' '; }
fn bool char_is_slash(u8 ch) { return ch == '/'; }
fn bool char_is_upper(u8 ch) { return ch >= 'A' && ch <= 'Z'; }
fn bool char_is_lower(u8 ch) { return ch >= 'a' && ch <= 'z'; }
fn bool char_is_digit(u8 ch) { return ch >= '0' && ch <= '9'; }
fn bool char_is_octal(u8 ch) { return ch >= '0' && ch <= '7'; }
fn bool char_is_hex(u8 ch) {
  return (ch >= '0' && ch <= '9') ||
         (ch >= 'a' && ch <= 'f') ||
         (ch >= 'A' && ch <= 'F');
}
fn bool char_is_alphabetic(u8 ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}
fn bool char_is_alphanumeric(u8 ch) {
  return char_is_digit(ch) || char_is_alphabetic(ch);
}
fn u8 char_to_upper(u8 ch) { return char_is_lower(ch) ? ch - 32 : ch; }
fn u8 char_to_lower(u8 ch) { return char_is_upper(ch) ? ch + 32 : ch; }
fn u8 path_separator(void) {
#if OS_WINDOWS
  return '\\';
#else
  return '/';
#endif
}

// =============================================================================
// Other UTF strings
bool str16Eq(String16 s1, String16 s2) {
  if (s1.size != s2.size) {
    return false;
  }

  for (isize i = 0; i < s1.size; ++i) {
    if (s1.str[i] != s2.str[i]) {
      return false;
    }
  }

  return true;
}

fn usize cstring16_length(u16 *str){
  u16 *p = str;
  for(; *p; ++p);
  return p - str;
}

fn String16 str16_cstr(u16 *str){
  String16 result = { str, (isize)cstring16_length(str) };
  return result;
}

fn bool str32Eq(String32 s1, String32 s2) {
  if (s1.size != s2.size) {
    return false;
  }

  for (isize i = 0; i < s1.size; ++i) {
    if (s1.str[i] != s2.str[i]) {
      return false;
    }
  }

  return true;
}

// =============================================================================
// UTF string conversion
fn String8 UTF8From16(Arena *arena, String16 in) {
  usize res_size = 0, approx_size = in.size * 4;
  u8 *bytes = New(arena, u8, approx_size), *res_offset = bytes;

  Codepoint codepoint = {0};
  for (u16 *start = in.str, *end = in.str + in.size; start < end;
       start += codepoint.size) {
    codepoint = decodeUTF16(start);

    u8 utf8_codepoint_size = encodeUTF8(res_offset, codepoint);
    res_size += utf8_codepoint_size;
    res_offset += utf8_codepoint_size;
  }

  arenaPop(arena, (approx_size - res_size));
  return str8(bytes, res_size);
}

fn String8 UTF8From32(Arena *arena, String32 in) {
  usize res_size = 0, approx_size = in.size * 4;
  u8 *bytes = New(arena, u8, approx_size), *res_offset = bytes;

  Codepoint codepoint = {0};
  for (u32 *start = in.str, *end = in.str + in.size; start < end;
       start += codepoint.size) {
    codepoint = decodeUTF32(start);

    u8 utf8_codepoint_size = encodeUTF8(res_offset, codepoint);
    res_size += utf8_codepoint_size;
    res_offset += utf8_codepoint_size;
  }

  arenaPop(arena, (approx_size - res_size));
  return str8(bytes, res_size);
}

fn String16 UTF16From8(Arena *arena, String8 in) {
  isize res_size = 0, approx_size = in.size * 2;
  u16 *words = New(arena, u16, approx_size), *res_offset = words;

  Codepoint codepoint = {0};
  for (u8 *start = in.str, *end = in.str + in.size; start < end;
       start += codepoint.size) {
    codepoint = decodeUTF8(start);

    u8 utf16_codepoint_size = encodeUTF16(res_offset, codepoint);
    res_size += utf16_codepoint_size;
    res_offset += utf16_codepoint_size;
  }

  arenaPop(arena, (approx_size - res_size));
  String16 res = {words, res_size};
  return res;
}

fn String16 UTF16From32(Arena *arena, String32 in) {
  isize res_size = 0, approx_size = in.size * 2;
  u16 *words = New(arena, u16, approx_size), *res_offset = words;

  Codepoint codepoint = {0};
  for (u32 *start = in.str, *end = in.str + in.size; start < end;
       start += codepoint.size) {
    codepoint = decodeUTF32(start);

    u8 utf16_codepoint_size = encodeUTF16(res_offset, codepoint);
    res_size += utf16_codepoint_size;
    res_offset += utf16_codepoint_size;
  }

  arenaPop(arena, (approx_size - res_size));
  String16 res = {words, res_size};
  return res;
}

fn String32 UTF32From8(Arena *arena, String8 in) {
  isize res_size = 0, approx_size = in.size * 2;
  u32 *dwords = New(arena, u32, approx_size), *res_offset = dwords;

  Codepoint cp = {0};
  for (u8 *start = in.str, *end = in.str + in.size; start < end;
       start += cp.size, ++res_size) {
    cp = decodeUTF8(start);
    *res_offset++ = cp.codepoint;
  }

  arenaPop(arena, (approx_size - res_size));
  String32 res = {dwords, res_size};
  return res;
}

fn String32 UTF32From16(Arena *arena, String16 in) {
  isize res_size = 0, approx_size = in.size * 2;
  u32 *dwords = New(arena, u32, approx_size), *res_offset = dwords;

  Codepoint cp = {0};
  for (u16 *start = in.str, *end = in.str + in.size; start < end;
       start += cp.size, ++res_size) {
    cp = decodeUTF16(start);
    *res_offset++ = cp.codepoint;
  }

  arenaPop(arena, (approx_size - res_size));
  String32 res = {dwords, res_size};
  return res;
}
