#include "csv.h"

inline fn StringStream csv_header(Arena *arena, CSV *config) {
  return csv_nextRow(arena, config);
}

fn StringStream csv_nextRow(Arena *arena, CSV *config) {
  if (config->file.content.size <= config->offset) {
    return (StringStream) {0};
  }

  String8 content = strPostfix(config->file.content, config->offset);
  usz line_ends = strFindFirst(content, '\n');
  String8 row = strPrefix(content, line_ends);

  config->offset += line_ends + 1;

  return strSplit(arena, row, config->delimiter);
}

// TODO: write to CSV file
fn void csv_write(String8 path, char delimiter, ...);
