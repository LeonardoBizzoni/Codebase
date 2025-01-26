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
    res[0] = cp.codepoint;
    return 1;
  } else if (cp.codepoint <= 0x7FF) {
    res[0] = 0xC0 | (cp.codepoint >> 6);
    res[1] = 0x80 | (cp.codepoint & 0x3F);
    return 2;
  } else if (cp.codepoint <= 0xFFFF) {
    res[0] = 0xE0 | (cp.codepoint >> 12);
    res[1] = 0x80 | ((cp.codepoint >> 6) & 0x3F);
    res[2] = 0x80 | (cp.codepoint & 0x3F);
    return 3;
  } else if (cp.codepoint <= 0x10FFFF) {
    res[0] = 0xF0 | (cp.codepoint >> 18);
    res[1] = 0x80 | ((cp.codepoint >> 12) & 0x3F);
    res[2] = 0x80 | ((cp.codepoint >> 6) & 0x3F);
    res[3] = 0x80 | (cp.codepoint & 0x3F);
    return 4;
  } else {
    Assert(false);
    return -1;
  }
}

fn u8 encodeUTF16(u16 *res, Codepoint cp) {
  if (cp.codepoint <= 0xD7FF ||
      (cp.codepoint >= 0xE000 && cp.codepoint <= 0xFFFF)) {
    res[0] = cp.codepoint;
    return 1;
  } else if (cp.codepoint >= 0x10000 && cp.codepoint <= 0x10FFFF) {
    res[0] = ((cp.codepoint - 0x10000) >> 10) + 0xD800;
    res[1] = ((cp.codepoint - 0x10000) & 0x3FF) + 0xDC00;
    return 2;
  } else {
    Assert(false);
    return -1;
  }
}

inline fn u8 encodeUTF32(u32 *res, Codepoint cp) {
  res[0] = cp.codepoint;
  return 1;
}

// =============================================================================
// UTF-8 string
fn String8 str8FromStream(Arena *arena, StringStream stream) {
  u8 *str = New(arena, u8, stream.total_size);
  u8 *ptr = str;
  for (StringNode *curr = stream.first; curr; curr = curr->next) {
    memCopy(ptr, curr->value.str, curr->value.size);
    ptr += curr->value.size;
  }
  
  return str8(str, stream.total_size);
}

fn void stringstreamAppend(Arena *arena, StringStream *strlist, String8 other) {
  Assert(arena);
  Assert(strlist);
  strlist->node_count += 1;
  strlist->total_size += other.size;
  StringNode *str = New(arena, StringNode);
  str->value = other;
  DLLPushBack(strlist->first, strlist->last, str);
}

inline fn String8 str8(u8 *chars, usize len) {
  String8 res = {chars, len};
  return res;
}

inline fn String8 strFromCstr(char *chars) {
  String8 res = {(u8*)chars, str8len(chars)};
  return res;
}

inline fn String8 strFromUnixTime(Arena *arena, u64 unix_timestamp) {
  DateTime dt = dateTimeFromUnix(unix_timestamp);
  return strFromDateTime(arena, dt);
}

inline fn String8 strFromDateTime(Arena *arena, DateTime dt) {
  return strFormat(arena, "%02d/%02d/%04d %02d:%02d:%02d.%02d",
		   dt.day, dt.month, dt.year,
		   dt.hour, dt.minute, dt.second, dt.ms);
}

fn String8 str8_copy(Arena *arena, String8 str){
  String8 result = {0};
  
  result.str = New(arena, u8, str.size + 1);
  result.size = str.size;
  memcpy(result.str, str.str, str.size);
  result.str[str.size] = 0;
  return result;
}

fn bool strEq(String8 s1, String8 s2) {
  if (s1.size != s2.size) {
    return false;
  }

  for (usize i = 0; i < s1.size; ++i) {
    if (s1.str[i] != s2.str[i]) {
      return false;
    }
  }

  return true;
}

fn bool strEqCstr(String8 s, const char *cstr) {
  if (s.size == 0 && !cstr) {
    return true;
  } else if (!cstr || s.size == 0) {
    return false;
  }

  usize i = 0;
  for (; i < s.size; ++i) {
    if (s.str[i] != cstr[i]) {
      return false;
    }
  }

  if (cstr[i]) {
    return false;
  } else {
    return true;
  }
}

fn bool cstrEq(char *s1, char *s2) {
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

fn bool strIsSignedInteger(String8 s) {
  u8 *curr = s.str;
  if (*curr == '-' || *curr == '+') {
    ++curr;
  }

  for (; curr < s.str + s.size; ++curr) {
    if (!charIsDigit(*curr)) {
      return false;
    }
  }

  return true;
}

fn bool strIsInteger(String8 s) {
  for (u8 *curr = s.str; curr < s.str + s.size; ++curr) {
    if (!charIsDigit(*curr)) {
      return false;
    }
  }

  return true;
}

fn bool strIsFloating(String8 s) {
  bool decimal_found = false;
  u8 *curr = s.str;
  if (*curr == '-' || *curr == '+') {
    ++curr;
  }

  for (; curr < s.str + s.size; ++curr) {
    if (!charIsDigit(*curr)) {
      if (*curr == '.' && !decimal_found) {
        decimal_found = true;
      } else {
        return false;
      }
    }
  }

  return true;
}

inline fn bool strIsNumerical(String8 s) {
  return strIsFloating(s) || strIsInteger(s);
}

fn i64 i64FromStr(String8 s) {
  i64 res = 0, decimal = 1;

  Assert(strIsSignedInteger(s));
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

fn u64 u64FromStr(String8 s) {
  i64 res = 0, decimal = 1;

  Assert(strIsInteger(s));
  for (u8 *curr = s.str + s.size - 1; curr >= s.str; --curr, decimal *= 10) {
    res += (*curr - '0') * decimal;
  }

  return res;
}

// TODO: maybe implement `strtod`?
fn f64 f64FromStr(String8 s) {
  u8 *end = s.str + s.size;
  return strtod((char *)s.str, (char **)&end);
}

/* Djb2: http://www.cse.yorku.ca/~oz/hash.html */
fn usize strHash(String8 s) {
  usize hash = 5381;
  for (usize i = 0; i < s.size; ++i) {
    hash = (hash << 5) + hash + s.str[i];
  }

  return hash;
}

fn String8 stringifyI64(Arena *arena, i64 n) {
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

fn String8 stringifyU64(Arena *arena, u64 n) {
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

fn String8 stringifyF64(Arena *arena, f64 n) {
  usize approx = 100, size = 0;
  u8 *str = New(arena, u8, approx);

  // TODO: maybe implement `sprintf`?
  size = sprintf((char *)str, "%f", n);
  arenaPop(arena, approx - size);

  return str8(str, size);
}

fn usize str8len(char *chars) {
  char *start = chars;
  for (; *start; ++start)
    ;

  return start - chars;
}

fn String8 strFormat(Arena *arena, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  String8 res = strFormatVa(arena, fmt, args);
  va_end(args);

  return res;
}

fn String8 strFormatVa(Arena *arena, const char *fmt, va_list args) {
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

inline fn String8 strPrefix(String8 s, usize end) {
  return str8(s.str, ClampTop(s.size, end));
}

inline fn String8 strPostfix(String8 s, usize start) {
  return str8(s.str + start, s.size >= start ? s.size - start : 0);
}

inline fn String8 substr(String8 s, usize end) {
  return str8(s.str, ClampTop(s.size,end));
}

inline fn String8 strRange(String8 s, usize start, usize end) {
  return str8(s.str + start, ClampTop(end, s.size) - start);
}

inline fn bool strEndsWith(String8 s, char ch) { return s.str[s.size - 1] == ch; }

fn String8 longestCommonSubstring(Arena *arena, String8 s1, String8 s2) {
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

fn String8 upperFromStr(Arena *arena, String8 s) {
  String8 res = {New(arena, u8, s.size), s.size};

  for (usize i = 0; i < s.size; ++i) {
    res.str[i] = charToUpper(s.str[i]);
  }

  return res;
}

fn String8 lowerFromStr(Arena *arena, String8 s) {
  String8 res = {New(arena, u8, s.size), s.size};

  for (usize i = 0; i < s.size; ++i) {
    res.str[i] = charToLower(s.str[i]);
  }

  return res;
}

fn String8 capitalizeFromStr(Arena *arena, String8 s) {
  String8 res = {New(arena, u8, s.size), s.size};

  res.str[0] = charToUpper(s.str[0]);
  for (usize i = 1; i < s.size; ++i) {
    if (charIsSpace(s.str[i])) {
      res.str[i] = s.str[i];
      ++i;
      res.str[i] = charToUpper(s.str[i]);
    } else {
      res.str[i] = charToLower(s.str[i]);
    }
  }
  
  return res;
}

fn StringStream strSplit(Arena *arena, String8 s, char ch) {
  StringStream res = {0};
  
  usize prev = 0;
  for (usize i = 0; i < s.size;) {
    if (s.str[i] == ch) {
      if (prev != i) {
        stringstreamAppend(arena, &res, strRange(s, prev, i));
      }
      
      do {
        prev = ++i;
      } while (s.str[i] == ch);
    } else {
      ++i;
    }
  }
  
  if (prev != s.size) {
    stringstreamAppend(arena, &res, strRange(s, prev, s.size));
  }
  
  return res;
}

fn usize strFindFirst(String8 s, char ch) {
  for (u8 *curr = s.str; curr < s.str + s.size + 1; ++curr) {
    if (*curr == ch) {
      return curr - s.str;
    }
  }

  return 0;
}

fn usize strFindFirstSubstr(String8 haystack, String8 needle) {
  if (haystack.size < needle.size) {
    return 0;
  }

  for (usize i = 0; i < haystack.size; ++i) {
    if (haystack.str[i] == needle.str[0]) {
      for (usize j = 0; i < needle.size; ++j) {
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

fn bool strContains(String8 s, char ch) {
  for (u8 *curr = s.str; curr < s.str + s.size + 1; ++curr) {
    if (*curr == ch) {
      return true;
    }
  }

  return false;
}

fn bool charIsSpace(u8 ch) { return ch == ' '; }
fn bool charIsSlash(u8 ch) { return ch == '/'; }
fn bool charIsUpper(u8 ch) { return ch >= 'A' && ch <= 'Z'; }
fn bool charIsLower(u8 ch) { return ch >= 'a' && ch <= 'z'; }
fn bool charIsDigit(u8 ch) { return ch >= '0' && ch <= '9'; }
fn bool charIsAlpha(u8 ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

fn bool charIsAlphanumeric(u8 ch) {
  return charIsDigit(ch) || charIsAlpha(ch);
}

fn u8 charToUpper(u8 ch) { return charIsLower(ch) ? ch - 32 : ch; }
fn u8 charToLower(u8 ch) { return charIsUpper(ch) ? ch + 32 : ch; }
fn u8 getCorrectPathSeparator() {
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
  
  for (usize i = 0; i < s1.size; ++i) {
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
  String16 result = { str, cstring16_length(str) };
  return result;
}

fn bool str32Eq(String32 s1, String32 s2) {
  if (s1.size != s2.size) {
    return false;
  }
  
  for (usize i = 0; i < s1.size; ++i) {
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
  usize res_size = 0, approx_size = in.size * 2;
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
  usize res_size = 0, approx_size = in.size * 2;
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
  usize res_size = 0, approx_size = in.size * 2;
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
  usize res_size = 0, approx_size = in.size * 2;
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
