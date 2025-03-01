#include <functional>

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
      if (strIsNumerical(curr->value)) {
        res[j][i].kind = FeatureKind_Continous;
        res[j][i].value = f64FromStr(curr->value);
      } else {
        res[j][i].kind = FeatureKind_Category;
        res[j][i].name = curr->value;
      }
    }
  }
  return res;
}

fn f32 ai_gini_category(CategoryMap &map, Array<Feature> &feature) {
  f32 res = 0;
  for (CategoryMap::Slot slot : map.slots) {
    for (CategoryMap::KVNode *curr = slot.first; curr; curr = curr->next) {
      f32 gini_branch = 1;
      for (CategoryMap::Slot fslot : curr->value.target.slots) {
        for (CategoryMap::KVNode *currf = fslot.first; currf; currf = currf->next) {
          gini_branch -= powf((f32)currf->value.freq / (f32)curr->value.freq, 2);
        }
      }
      res += ((f32)curr->value.freq / (f32)feature.size) * gini_branch;
    }
  }
  return res;
}

fn BranchCondition ai_gini_continous(Array<Feature> &feature, Array<Feature> &target) {
  // sort feature in ascending order
  local std::function<void(isize, isize)> quicksort =
    [&feature](isize low, isize high) {
      std::function<isize(isize, isize)> partition =
        [&feature](isize low, isize high) -> isize {
          isize pivot_idx = Random(low, high);
          Feature pivot = feature[pivot_idx];

          isize i = low - 1;
          for (isize j = low; j < high; ++j) {
            if (feature[j].value < pivot.value) {
              i += 1;
              Feature tmp = feature[i];
              feature[i] = feature[j];
              feature[j] = tmp;
            }
          }

          i += 1;
          Feature tmp = feature[i];
          feature[i] = feature[pivot_idx];
          feature[pivot_idx] = tmp;
          return i;
        };

      if (low >= high) { return; }
      isize pi = partition(low, high);
      quicksort(low, pi - 1);
      quicksort(pi + 1, high);
    };

  quicksort(0, feature.size - 1);

  // compute the average for each pair
  Scratch scratch = ScratchBegin(0, 0);
  Array<f32> average = Array<f32>(scratch.arena, feature.size - 1);
  for (usize i = 0; i < feature.size - 1; ++i) {
    average[i] = (feature[i].value + feature[i + 1].value) / 2.f;
  }

  // for each average compute the gini impurity for: < average, >= average
  BranchCondition res = {0};
  res.gini = 1.f;
  for (usize i = 0; i < average.size; ++i) {
    usize less_count = 0;
    HashMap<String8, usize> less_map(scratch.arena, strHash);
    HashMap<String8, usize> more_map(scratch.arena, strHash);
    for (usize j = 0; j < feature.size; ++j) {
      if (feature[j].value < average[i]) {
        less_count += 1;
        *less_map.fromKey(scratch.arena, target[j].name, 0) += 1;
      } else {
        *more_map.fromKey(scratch.arena, target[j].name, 0) += 1;
      }
    }

    // Actually compute gini
    f32 less_branch = 1.f, more_branch = 1.f;
    for (HashMap<String8, usize>::Slot slot : less_map.slots) {
      for (HashMap<String8, usize>::KVNode *curr = slot.first; curr; curr = curr->next) {
        less_branch -= powf((f32)curr->value / (f32)less_count, 2);
      }
    }
    for (HashMap<String8, usize>::Slot slot : more_map.slots) {
      for (HashMap<String8, usize>::KVNode *curr = slot.first; curr; curr = curr->next) {
        more_branch -= powf((f32)curr->value / (f32)(feature.size - less_count), 2);
      }
    }

    f32 gini = ((f32)less_count / (f32)feature.size) * less_branch +
               ((f32)(feature.size - less_count) / (f32)feature.size) * more_branch;
    if (gini < res.gini) {
      res.gini = gini;
      res.continous_threshold = average[i];
    }
  }
  ScratchEnd(scratch);

  return res;
}

fn BranchCondition ai_gini(u32 target_feature_idx, f32 threshold,
                           Array<Array<Feature>> &features, CategoryMap *maps) {
  BranchCondition res = {0};
  res.gini = 1.f;
  for (usize i = 0; i < features.size; ++i) {
    if (i == target_feature_idx) { continue; }

    BranchCondition cond = {0};
    switch (features[i][0].kind) {
      case FeatureKind_Category: {
        cond.gini = ai_gini_category(maps[i], features[i]);
      } break;
      case FeatureKind_Continous: {
        cond = ai_gini_continous(features[i], features[target_feature_idx]);
      } break;
    }

    if (cond.gini < res.gini) {
      res = cond;
      res.split_idx = i;
    }
  }

  if (res.gini <= threshold) { res.split_idx = -1; }
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

fn DTree* ai_dtree_makeNode(Arena *arena, StringStream *header, File dataset,
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

#if 0 && DEBUG
  log_category_map(header, maps, &features, target_feature_idx);
#endif

  DTree *res = New(arena, DTree);
  res->cond = ai_gini(target_feature_idx, entropy_threshold,
                      features, maps);
  if (res->cond.split_idx == -1) {
    usize count = 0;
    String8 most_frequent = Strlit("");

    for (CategoryMap::Slot slot : maps[target_feature_idx].slots) {
      for (CategoryMap::KVNode *curr = slot.first; curr; curr = curr->next) {
        if (curr->value.freq > count) {
          count = curr->value.freq;
          most_frequent = curr->key;
        }
      }
    }

    res->label.str = New(arena, u8, most_frequent.size);
    res->label.size = most_frequent.size;
    memCopy(res->label.str, most_frequent.str, most_frequent.size);
  } else {
    Info(strFormat(scratch.arena, "Split by: `%.*s` (gini: %.3lf)",
                   Strexpand((*header)[res->cond.split_idx]), res->cond.gini));
    Info(strFormat(scratch.arena, "Will split into %ld branches",
                   maps[res->cond.split_idx].size));

    HashMap<String8, File> filemap(scratch.arena, strHash);
    for (usize i = 0; i < features[0].size; ++i) {
      File *file = filemap.fromKey(scratch.arena,
                                   features[target_feature_idx][i].name,
                                   fs_fopenTmp(scratch.arena, OS_acfAppend));
      Info(file->path);
      for (isize f = 0; f < (isize)features.size; ++f) {
        if (f == res->cond.split_idx) { continue; }
        switch (features[f][i].kind) {
          case FeatureKind_Category: {
            fs_write(file->file_handle, features[f][i].name);
          } break;
          case FeatureKind_Continous: {
            fs_write(file->file_handle,
                     stringifyF64(scratch.arena, features[f][i].value));
          } break;
        }

        if ((f + 1 == res->cond.split_idx && f + 2 < (isize)features.size) ||
            (f + 1 < (isize)features.size)) {
          fs_write(file->file_handle, Strlit(","));
        } else {
          fs_write(file->file_handle, Strlit("\n"));
        }
      }
    }
  }

  ScratchEnd(scratch);
  return res;
}

fn DTree* ai_dtree_build(Arena *arena, File dataset, StringStream *header,
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
