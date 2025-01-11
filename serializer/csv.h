#ifndef BASE_SERIALIZER_CSV
#define BASE_SERIALIZER_CSV

#include "../base.h"
#include "../arena.h"
#include "../string.h"
#include "../OS/file.h"

typedef struct {
  char delimiter;
  File file;

  usz offset;
} CSV;

fn StringStream csv_header(Arena *arena, CSV *config);
fn StringStream csv_nextRow(Arena *arena, CSV *config);

#endif
