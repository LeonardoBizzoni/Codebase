#ifndef BASE_STRING
#define BASE_STRING

#include <stdarg.h>
#include <stdio.h>

#define Strlit(STR) str8((u8 *)(STR), sizeof(STR) - 1)
#define StrlitInit(STR) { (u8 *)(STR), sizeof(STR) - 1, }
#define Strexpand(STR) (i32)((STR).size), (char *)((STR).str)

// =============================================================================
// Unicode codepoint
typedef struct {
  u32 codepoint;
  u8 size;
} Codepoint;

fn Codepoint decodeUTF8(u8 *glyph_start);
fn Codepoint decodeUTF16(u16 *glyph_start);
fn Codepoint decodeUTF32(u32 *glyph_start);
fn u8 encodeUTF8(u8 *res, Codepoint cp);
fn u8 encodeUTF16(u16 *res, Codepoint cp);
fn u8 encodeUTF32(u32 *res, Codepoint cp);

// =============================================================================
// UTF-8 string

typedef struct String8 {
  u8 *str;
  isize size;

#if CPP
    String8() = default;
    template<isize N>
    constexpr String8(const char (&s)[N]) :
      str{(u8 *)s}, size{N-1} {}
    constexpr String8(u8 *chars, isize size) :
      str{chars}, size{size} {}

  inline char operator[](usize idx) {
    return (char)str[idx];
  }
#endif
} String8;

typedef struct StringNode {
  String8 value;
  struct StringNode *next;
  struct StringNode *prev;
} StringNode;

typedef struct StringStream {
  StringNode *first;
  StringNode *last;
  isize node_count;
  isize total_size;

#if CPP
  String8 operator[](usize i) {
    StringNode *curr = first;
    for (; curr && i > 0; curr = curr->next, --i);
    Assert(curr);
    return curr->value;
  }
#endif
} StringStream;

fn void strstream_append_str(Arena *arena, StringStream *strlist, String8 other);
fn void strstream_append_stream(StringStream *strlist, StringStream other);
fn String8 strstream_join_char(Arena *arena, StringStream strlist, char ch);
fn String8 strstream_join_str(Arena *arena, StringStream strlist, String8 str);

fn String8 str8(u8 *chars, isize len);
fn String8 str8_from_stream(Arena *arena, StringStream stream);
fn String8 str8_from_char(Arena *arena, char ch);
fn String8 str8_from_cstr(char *chars);
fn String8 str8_from_i64(Arena *arena, i64 n);
fn String8 str8_from_u64(Arena *arena, u64 n);
fn String8 str8_from_f64(Arena *arena, f64 n);
fn String8 str8_from_datetime(Arena *arena, DateTime dt);
fn String8 str8_from_unixtime(Arena *arena, u64 timestamp);
fn String8 str8_from_time64(Arena *arena, time64 timestamp);
fn String8 str8_prefix(String8 s, isize end);
fn String8 str8_postfix(String8 s, isize start);
fn String8 str8_substr(String8 s, isize first, isize opl);
fn String8 str8_range(u8 *first, u8 *opl);
fn String8 str8_lcs(Arena *arena, String8 s1, String8 s2);
fn String8 str8_to_upper(Arena *arena, String8 s);
fn String8 str8_to_lower(Arena *arena, String8 s);
fn String8 str8_to_capitalized(Arena *arena, String8 s);
fn String8 str8_trim(String8 s);
fn String8 str8_trim_left(String8 s);
fn String8 str8_trim_right(String8 s);
fn String8 str8_format(Arena *arena, const char *fmt, ...);
fn String8 _str8_format(Arena *arena, const char *fmt, va_list args);
fn StringStream str8_split(Arena *arena, String8 s, char ch);
fn usize str8_find_first_ch(String8 s, char ch);
fn usize str8_find_first_str8(String8 s, String8 needle);
fn usize str8_hash(String8 s);
fn isize str8_len(char *chars);
fn bool str8_eq(String8 s1, String8 s2);
fn bool str8_eq_cstr(String8 s, const char *cstr);
fn bool str8_ends_with_ch(String8 s, char ch);
fn bool str8_ends_with_str8(String8 s, String8 needle);
fn bool str8_contains(String8 s, char ch);
fn bool str8_is_integer(String8 s);
fn bool str8_is_integer_signed(String8 s);
fn bool str8_is_floating(String8 s);
fn bool str8_is_numerical(String8 s);

fn char* cstr_from_str8(Arena *arena, String8 str);
fn bool cstr_eq(const char *s1, const char *s2);

fn i64 i64_from_str8(String8 s);
fn u64 u64_from_str8(String8 s);
fn f64 f64_from_str8(String8 s);

fn bool char_is_whitespace(u8 ch);
fn bool char_is_space(u8 ch);
fn bool char_is_slash(u8 ch);
fn bool char_is_upper(u8 ch);
fn bool char_is_lower(u8 ch);
fn bool char_is_digit(u8 ch);
fn bool char_is_octal(u8 ch);
fn bool char_is_hex(u8 ch);
fn bool char_is_alphabetic(u8 ch);
fn bool char_is_alphanumeric(u8 ch);
fn u8 char_to_upper(u8 ch);
fn u8 char_to_lower(u8 ch);

fn u8 path_separator(void);

#if CPP
inline fn bool operator==(String8 s1, String8 s2) {
  return str8_eq(s1, s2);
}

inline fn bool operator!=(String8 s1, String8 s2) {
  return !str8_eq(s1, s2);
}
#endif

// =============================================================================
// Other UTF strings

typedef struct {
  u16 *str;
  isize size;
} String16;

typedef struct {
  u32 *str;
  isize size;
} String32;

fn bool str16Eq(String16 s1, String16 s2);
fn bool str32Eq(String32 s1, String32 s2);
fn usize cstring16_length(u16 *str);
fn String16 str16_cstr(u16 *str);
// =============================================================================
// UTF string conversion

fn String8 UTF8From16(Arena *arena, String16 in);
fn String8 UTF8From32(Arena *arena, String32 in);

fn String16 UTF16From8(Arena *arena, String8 in);
fn String16 UTF16From32(Arena *arena, String32 in);

fn String32 UTF32From8(Arena *arena, String8 in);
fn String32 UTF32From16(Arena *arena, String16 in);

// =============================================================================

#endif
