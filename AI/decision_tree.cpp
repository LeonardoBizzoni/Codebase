fn Array<Array<Feature>> ai_chunk2features(Arena *arena, File dataset, isize offset,
                                           u32 n_features, u32 chunk_size,
                                           StringStream (*next_line)(Arena*, File,
                                                                     isize*)) {
  Array<Array<Feature>> res(arena, n_features);
  for (usize i = 0; i < n_features; ++i) {
    res[i] = Array<Feature>(arena, chunk_size);
  }

  for (u32 i = 0; i < chunk_size; ++i) {
    StringStream row = next_line(arena, dataset, &offset);
    if (!row.node_count) {
      for (Array<Feature> &it : res) {
	it.size = i;
      }
      break;
    }

    u32 j = 0;
    for (StringNode *curr = row.first; curr; curr = curr->next, ++j) {
      // Info(curr->value);
      if (strIsNumerical(curr->value)) {
        // Info(Strlit("numerical feature"));
        res[j][i].kind = FeatureKind_Continous;
        res[j][i].value = f64FromStr(curr->value);
      } else {
        // Info(Strlit("categorical feature"));
        res[j][i].kind = FeatureKind_Category;
        res[j][i].name = curr->value;
      }
    }
  }
  return res;
}

inline fn f32 ai_entropy(f32 px) { return -px * log2f(px); }

fn f32 ai_entropy_target(CategoryMap *catfeature, usize rows) {
  f32 res = 0.f;
  for (CategoryMap::Slot slot : catfeature->slots) {
    for (CategoryMap::KVNode *curr = slot.first; curr; curr = curr->next) {
      res += ai_entropy((f32)curr->value.freq / (f32)rows);
    }
  }

  Scratch scratch = ScratchBegin(0, 0);
  Info(strFormat(scratch.arena, "Entropy: %f", res));
  ScratchEnd(scratch);
  return res;
}

fn f32 ai_entropy_feature(CategoryMap *catfeature, usize rows) {
  f32 res = 0.f;
  for (CategoryMap::Slot slot : catfeature->slots) {
    for (CategoryMap::KVNode *curr = slot.first; curr; curr = curr->next) {
      for (CategoryMap::Slot fslot : curr->value.target.slots) {
        for (CategoryMap::KVNode *currf = fslot.first; currf; currf = currf->next) {
          res += (f32)curr->value.freq / (f32)rows *
                 ai_entropy((f32)currf->value.freq / (f32)curr->value.freq);
        }
      }
    }
  }

  Scratch scratch = ScratchBegin(0, 0);
  Info(strFormat(scratch.arena, "Entropy: %f", res));
  ScratchEnd(scratch);
  return res;
}

fn usize ai_maxInformationGain(u32 target_feature_idx, Array<Array<Feature>> &features,
			       CategoryMap *maps) {
  usize res = -1;
  f32 max_infogain = 0.f;
  f32 target_entropy = ai_entropy_target(&maps[target_feature_idx], features[0].size);

  printf("\n");
  Scratch scratch = ScratchBegin(0, 0);
  for (usize i = 0; i < features.size; ++i) {
    if (i == target_feature_idx) { continue; }

    f32 entropy;
    Info(strFormat(scratch.arena, "Heading idx: %ld", i));
    if (features[i][0].kind == FeatureKind_Category) {
      Info(Strlit("Categoric feature"));
      entropy = ai_entropy_feature(&maps[i], features[0].size);
    } else {
      Info(Strlit("Numeric feature\n"));
      continue;
    }

    f32 gain = target_entropy - entropy;
    Info(strFormat(scratch.arena, "Information gain: %f\n", gain));
    if (gain > max_infogain) {
      max_infogain = gain;
      res = i;
    }
  }
  Info(strFormat(scratch.arena, "Max information gain: %f", max_infogain));
  Info(strFormat(scratch.arena, "Split by feature at: %ld", res));
  ScratchEnd(scratch);
  return res;
}

fn void log_category_map(StringStream *header, CategoryMap *maps,
			 Array<Array<Feature>> *features, usize target_feature_idx) {
  for (usize i = 0; i < features->size; ++i) {
    if (i == target_feature_idx) { continue; }
    printf("Feature `%.*s`:\n", Strexpand((*header)[i]));
    if ((*features)[i][0].kind != FeatureKind_Category) {
      printf("\tContinous feature.\n\n");
      goto log_end;
    }

    for (CategoryMap::Slot slot : maps[i].slots) {
      for (CategoryMap::KVNode *curr = slot.first; curr; curr = curr->next) {
	printf("\t`%.*s`: %d/%d\n", Strexpand(curr->key), curr->value.freq,
	       (*features)[0].size);

	for (CategoryMap::Slot fslot : curr->value.target.slots) {
          for (CategoryMap::KVNode *currf = fslot.first; currf; currf = currf->next) {
	    printf("\t\t`%.*s`: %d/%d\n", Strexpand(currf->key),
		   currf->value.freq, curr->value.freq);
          }
	}
	if (curr->value.target.slots.first) {
	  printf("\n");
	}
      }
    }

    if (maps[i].slots.first) {
    log_end:
      printf("============================\n\n");
    }
  }
}

fn DTree ai_dtree_makeNode(Arena *arena, StringStream *header, File dataset,
			   isize offset, u32 n_features, u32 target_feature_idx,
			   f32 entropy_threshold, u32 chunk_size,
                           StringStream (*next_line)(Arena*, File, isize*)) {
  Scratch scratch = ScratchBegin(&arena, 1);
  Array<Array<Feature>> features = ai_chunk2features(scratch.arena, dataset, offset,
                                                     n_features, chunk_size, next_line);
  Assert(features.size == n_features);
#if DEBUG
  for (Array<Feature> &it : features) {
    Assert(it.size <= chunk_size);
  }
#endif

  CategoryMap *maps = New(scratch.arena, CategoryMap, n_features);
  for (usize i = 0; i < features.size; ++i) {
    if (features[i][0].kind != FeatureKind_Category) {continue;}
    maps[i] = CategoryMap(scratch.arena, strHash);
    for (usize j = 0; j < features[i].size; ++j) {
      Category *node = maps[i].fromKey(scratch.arena, features[i][j].name,
                                       Category(scratch.arena));
      node->freq += 1;
      node->target.fromKey(scratch.arena, features[target_feature_idx][j].name,
                           Category(scratch.arena))->freq += 1;
    }
  }

#if 1 && DEBUG
  log_category_map(header, maps, &features, target_feature_idx);
#endif

  usize best_split = ai_maxInformationGain(target_feature_idx, features, maps);
  ScratchEnd(scratch);

  DTree res = {0};
  return res;
}

fn DTree ai_dtree_build(Arena *arena, File dataset, StringStream *header,
                        u32 n_features, u32 target_feature_idx, f32 entropy_threshold,
                        u32 chunk_size,
                        StringStream (*next_line)(Arena*, File, isize*)) {
  isize offset = 0;
  if (!header) {
    header = New(arena, StringStream);
    *header = next_line(arena, dataset, &offset);
  }

  return ai_dtree_makeNode(arena, header, dataset, offset, n_features,
			   target_feature_idx, entropy_threshold, chunk_size, next_line);
}
