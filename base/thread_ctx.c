global threadvar TlsContext tls_ctx;

fn Arena *tlsGetScratch(Arena **conflicts, usize count) {
  if(tls_ctx.arenas[0] == 0) {
    for(usize i = 0; i < Arrsize(tls_ctx.arenas); ++i) {
      tls_ctx.arenas[i] = ArenaBuild(.reserve_size = TLS_CTX_SIZE);
    }
  }

  for(usize i = 0; i < Arrsize(tls_ctx.arenas); ++i) {
    bool has_conflict = false;
    for(usize j = 0; j < count; ++j) {
      if(tls_ctx.arenas[i] == conflicts[j]) {
        has_conflict = true;
        break;
      }
    }

    if(!has_conflict) {
      return tls_ctx.arenas[i];
    }
  }

  return 0;
}
