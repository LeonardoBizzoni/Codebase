#pragma once

#include "arena.h"
#include "base.h"

#include <stdarg.h>
#include <stdio.h>

#define Strlit(STR) {.str = (u8 *)(STR), .size = sizeof(STR) - 1}
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
typedef struct {
  u8 *str;
  size_t size;
} String8;

typedef struct StringNode {
  struct StringNode *next;
  String8 value;
} StringNode;

typedef struct {
  StringNode *first;
  StringNode *last;
  size_t size;
} StringStream;

fn String8 str8FromStream(Arena *arena, StringStream *stream);
fn void stringstreamAppend(Arena *arena, StringStream *strlist, String8 other);

fn String8 str8(char *chars, size_t len);
fn String8 strFromCstr(char *chars);

fn bool strEq(String8 s1, String8 s2);
fn bool strEqCstr(String8 s, const char *cstr);

fn bool strIsSignedInteger(String8 s);
fn bool strIsInteger(String8 s);
fn bool strIsFloating(String8 s);
fn i64 i64FromStr(String8 s);
fn u64 u64FromStr(String8 s);
fn f64 f64FromStr(String8 s);

fn String8 stringifyI64(Arena *arena, i64 n);
fn String8 stringifyU64(Arena *arena, u64 n);
fn String8 stringifyF64(Arena *arena, f64 n);

fn size_t strlen(char *chars);

fn String8 strFormat(Arena *arena, const char *fmt, ...);
fn String8 strFormatVa(Arena *arena, const char *fmt, va_list args);

fn String8 strPrefix(String8 s, size_t end);
fn String8 strPostfix(String8 s, size_t start);
fn String8 substr(String8 s, size_t end);
fn String8 strRange(String8 s, size_t start, size_t end);
fn bool strEndsWith(String8 s, char ch);
fn String8 longestCommonSubstring(Arena *arena, String8 s1, String8 s2);

fn String8 upperFromStr(Arena *arena, String8 s);
fn String8 lowerFromStr(Arena *arena, String8 s);
fn String8 capitalizeFromStr(Arena *arena, String8 s);

fn StringStream strSplit(Arena *arena, String8 s, char ch);
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

// =============================================================================
// Other UTF strings

typedef struct {
  u16 *str;
  size_t size;
} String16;

typedef struct {
  u32 *str;
  size_t size;
} String32;

fn bool str16Eq(String16 s1, String16 s2);
fn bool str32Eq(String32 s1, String32 s2);

// =============================================================================
// UTF string conversion

fn String8 UTF8From16(Arena *arena, String16 *in);
fn String8 UTF8From32(Arena *arena, String32 *in);

fn String16 UTF16From8(Arena *arena, String8 *in);
fn String16 UTF16From32(Arena *arena, String32 *in);

fn String32 UTF32From8(Arena *arena, String8 *in);
fn String32 UTF32From16(Arena *arena, String16 *in);

// =============================================================================