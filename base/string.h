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
inline fn Codepoint decodeUTF32(u32 *glyph_start);
fn u8 encodeUTF8(u8 *res, Codepoint cp);
fn u8 encodeUTF16(u16 *res, Codepoint cp);
inline fn u8 encodeUTF32(u32 *res, Codepoint cp);

// =============================================================================
// UTF-8 string

typedef struct String8 {
  u8 *str;
  isize size;

#if CPP
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

fn String8 str8FromStream(Arena *arena, StringStream stream);
fn void strstream_append_str(Arena *arena, StringStream *strlist, String8 other);
fn void strstream_append_stream(Arena *arena, StringStream *strlist, StringStream other);
fn String8 strstream_join_char(Arena *arena, StringStream strlist, char ch);
fn String8 strstream_join_str(Arena *arena, StringStream strlist, String8 str);

inline fn String8 str8(u8 *chars, isize len);
inline fn String8 strFromCstr(char *chars);
inline fn String8 strFromDateTime(Arena *arena, DateTime dt);
inline fn String8 strFromUnixTime(Arena *arena, u64 unix_timestamp);

fn char* cstrFromStr8(Arena *arena, String8 str);

fn bool strEq(String8 s1, String8 s2);
fn bool strEqCstr(String8 s, const char *cstr);
fn bool cstrEq(char *s1, char *s2);

fn bool strIsSignedInteger(String8 s);
fn bool strIsInteger(String8 s);
fn bool strIsFloating(String8 s);
inline fn bool strIsNumerical(String8 s);
fn i64 i64FromStr(String8 s);
fn u64 u64FromStr(String8 s);
fn f64 f64FromStr(String8 s);

fn usize strHash(String8 s);

fn String8 stringifyI64(Arena *arena, i64 n);
fn String8 stringifyU64(Arena *arena, u64 n);
fn String8 stringifyF64(Arena *arena, f64 n);

fn isize str8len(char *chars);

fn String8 strFormat(Arena *arena, const char *fmt, ...);
fn String8 strFormatVa(Arena *arena, const char *fmt, va_list args);

fn String8 strPrefix(String8 s, isize end);
fn String8 strPostfix(String8 s, isize start);
fn String8 substr(String8 s, isize end);
fn String8 strRange(String8 s, isize start, isize end);
fn bool strEndsWith(String8 s, char ch);
fn String8 longestCommonSubstring(Arena *arena, String8 s1, String8 s2);

fn String8 upperFromStr(Arena *arena, String8 s);
fn String8 lowerFromStr(Arena *arena, String8 s);
fn String8 capitalizeFromStr(Arena *arena, String8 s);

fn StringStream strSplit(Arena *arena, String8 s, char ch);
fn usize strFindFirst(String8 s, char ch);
fn usize strFindFirstSubstr(String8 s, String8 needle);
fn bool strContains(String8 s, char ch);

fn bool charIsSpace(u8 ch);
fn bool charIsSlash(u8 ch);
fn bool charIsUpper(u8 ch);
fn bool charIsLower(u8 ch);
fn bool charIsDigit(u8 ch);
fn bool charIsAlpha(u8 ch);
fn bool charIsAlphanumeric(u8 ch);

fn u8 charToUpper(u8 ch);
fn u8 charToLower(u8 ch);
fn u8 getCorrectPathSeparator();

fn String8 strLeftTrim(String8 s);
fn String8 strRightTrim(String8 s);
fn String8 strTrim(String8 s);

#if CPP
inline fn bool operator==(String8 s1, String8 s2) {
  return strEq(s1, s2);
}

inline fn bool operator!=(String8 s1, String8 s2) {
  return !strEq(s1, s2);
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
