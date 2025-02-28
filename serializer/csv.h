#ifndef BASE_SERIALIZER_CSV
#define BASE_SERIALIZER_CSV

fn StringStream csv_header(Arena *arena, File file, isize *offset);
fn StringStream csv_nextRow(Arena *arena, File file, isize *offset);

#endif
