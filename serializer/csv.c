#include "csv.h"

inline fn StringStream csv_header(Arena *arena, File file) {
  return csv_nextRow(arena, file, 0);
}

fn StringStream csv_nextRow(Arena *arena, File file, isize offset) {
  StringStream res = {0};
  String8 file_content = str8(file.content, file.prop.size);
  if (file_content.size <= offset) { return res; }

  String8 content = strPostfix(file_content, offset);
  usize line_ends = strFindFirst(content, '\n');
  String8 row = strPrefix(content, line_ends);

  return strSplit(arena, row, ',');
}
