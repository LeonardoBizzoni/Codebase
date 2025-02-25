fn Array<Array<Feature>> ai_chunk2features(Arena *arena, File dataset, isize offset,
                                           u32 n_features, u32 chunk_size,
                                           StringStream (*next_line)(Arena*, File,
                                                                     isize)) {
  Array<Array<Feature>> res(arena, chunk_size);
  for (u32 i = 0; i < chunk_size; ++i) {
    StringStream row = next_line(arena, dataset, offset);
    if (!row.node_count) {
      res.size = i;
      break;
    }
    res[i] = Array<Feature>(arena, n_features);
    offset += row.total_size + row.node_count;

    u32 j = 0;
    for (StringNode *curr = row.first; curr; curr = curr->next, ++j) {
      // Info(curr->value);
      if (strIsNumerical(curr->value)) {
        // Info(Strlit("numerical feature"));
        res[i][j].kind = FeatureKind_Continous;
        res[i][j].value = f64FromStr(curr->value);
      } else {
        // Info(Strlit("categorical feature"));
        res[i][j].kind = FeatureKind_Category;
        res[i][j].name = curr->value;
      }
    }
  }
  return res;
}

fn DTree ai_dtree_makeNode(Arena *arena, File dataset, isize offset, u32 n_features,
                           u32 target_feature_idx, f32 entropy_threshold, u32 chunk_size,
                           StringStream (*next_line)(Arena*, File, isize)) {
  Scratch scratch = ScratchBegin(&arena, 1);
  Array<Array<Feature>> features = ai_chunk2features(scratch.arena, dataset, offset,
                                                     n_features, chunk_size, next_line);

  OccMapList maps = {0};
  for (usize i = 0, map_i = 0; i < n_features; ++i) {
    if (features[0][i].kind != FeatureKind_Category || i == target_feature_idx)
      { continue; }
    HashMap<String8, Occurrence> *node_map = maps[map_i];
    if (!node_map) {
      OccMapListNode *new_node = New(scratch.arena, OccMapListNode);
      new_node->value = HashMap<String8, Occurrence>(scratch.arena, strHash),
      DLLPushBack(maps.first, maps.last, new_node);
      node_map = &new_node->value;
    }

    for (usize j = 0; j < features.size; ++j) {
      Occurrence *node = node_map->fromKey(scratch.arena, features[j][i].name,
                                           Occurrence(scratch.arena));
      node->count += 1;
      node->targets.fromKey(scratch.arena, features[j][target_feature_idx].name,
                            Occurrence(scratch.arena))->count += 1;
    }

    map_i += 1;
  }

  ScratchEnd(scratch);

  DTree res = {0};
  return res;
}

fn DTree ai_dtree_build(Arena *arena, File dataset, StringStream *header,
                        u32 n_features, u32 target_feature_idx, f32 entropy_threshold,
                        u32 chunk_size,
                        StringStream (*next_line)(Arena*, File, isize)) {
  if (!header) {
    header = New(arena, StringStream);
    *header = next_line(arena, dataset, 0);
  }

  isize offset = header->total_size + header->node_count;
  return ai_dtree_makeNode(arena, dataset, offset, n_features, target_feature_idx,
                           entropy_threshold, chunk_size, next_line);
}
