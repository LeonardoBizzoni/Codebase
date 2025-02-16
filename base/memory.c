#include "base.h"
#include "arena.h"

void *memCopy(void *dest, void *src, usize size) {
  if (!dest || !src) {
    return 0;
  } else if (size == 0) {
    return dest;
  }

  u8 *dest_byte = (u8 *)dest, *src_byte = (u8 *)src;
  for (usize i = 0; i < size; ++i) {
    dest_byte[i] = src_byte[i];
  }

  return dest;
}

void memZero(void *dest, usize size) {
  u8 *dest_bytes = (u8 *)dest;
  for (usize i = 0; i < size; ++i) {
    *(dest_bytes + i) = 0;
  }
}
