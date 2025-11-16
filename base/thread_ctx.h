#ifndef BASE_TLS_H
#define BASE_TLS_H

typedef struct {
  Arena *arenas[2];
  i32 id;
} TlsContext;

fn Arena *tls_get_scratch(Arena **conflicts, usize count);

#define ScratchBegin(conflicts, count) tmp_begin(tls_get_scratch((conflicts), (count)))
#define ScratchEnd(scratch) tmp_end((scratch))

#endif
