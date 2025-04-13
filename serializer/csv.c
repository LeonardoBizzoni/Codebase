#include "csv.h"

inline fn StringStream csv_header(Arena *arena, File file, isize *offset) {
  *offset = 0;
  return csv_nextRow(arena, file, offset);
}

fn StringStream csv_nextRow(Arena *arena, File file, isize *offset) {
  StringStream res = {0};
  String8 file_content = str8(file.content, file.prop.size);
  if (file_content.size <= *offset) { return res; }

  String8 content = str8_trim_left(str8_postfix(file_content, *offset));
  usize line_ends = str8_find_first_ch(content, '\n');
  String8 row = str8_trim(str8_prefix(content, line_ends));

  res = str8_split(arena, row, ',');
  *offset += res.total_size + res.node_count + (row.str[row.size] == '\r');
  return res;
}
