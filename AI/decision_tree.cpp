#include "../DataStructure/hashmap.hpp"

#include "decision_tree.h"

/* implement log2? */
#include <math.h>

struct Occurrence {
  String8 name;
  usize count;

  HashMap<String8, Occurrence> targets;

  Occurrence(Arena *arena) : count(0), targets(arena, strHash) {}
};

inline fn f64 ai_entropy(f64 val) { return val ? -val * log2(val) : 0; }

fn f64 ai_computeEntropy(HashMap<String8, Occurrence> *map,
                         usize row_count) {
  f64 res = 0;
  for (HashMap<String8, Occurrence>::Slot slot : map->slots) {
    for (HashMap<String8, Occurrence>::KVNode *curr = slot.first; curr;
         curr = curr->next) {
      res += ai_entropy((f64)curr->value.count / (f64)row_count);
    }
  }

  return res;
}

fn usize ai_maxInformationGain(HashMap<String8, Occurrence> *maps,
                               usize n_features, usize target_idx,
                               usize row_count, f64 threshold) {
  f64 max_gain = 0;
  usize max_gain_idx = 0;
  f64 target_entropy = ai_computeEntropy(&maps[target_idx], row_count);
#if DEBUG
  printf("\t\tTarget entropy: %.16lf\n", target_entropy);
#endif

  for (usize feature = 0; feature < n_features; ++feature) {
    if (feature == target_idx) {
      continue;
    }

    f64 entropy = 0;
#if DEBUG
    printf("\t");
#endif
    for (HashMap<String8, Occurrence>::Slot slot : maps[feature].slots) {
      HashMap<String8, Occurrence>::KVNode *curr = slot.first;
      for (; curr; curr = curr->next) {
#if DEBUG
        printf("%ld/%ld * entropy(", curr->value.count, row_count);
#endif

        for (HashMap<String8, Occurrence>::Slot fslot :
             curr->value.targets.slots) {
          for (HashMap<String8, Occurrence>::KVNode *currf = fslot.first;
               currf; currf = currf->next) {
            if (currf->value.count == 0) {
              continue;
            }
#if DEBUG
            printf("%ld/%ld ", currf->value.count, curr->value.count);
#endif
            entropy +=
                (f64)curr->value.count / (f64)row_count *
                ai_entropy((f64)currf->value.count / (f64)curr->value.count);
          }
        }

#if DEBUG
        printf("\b)");
#endif
      }
#if DEBUG
      if (curr) {
        printf(" + ");
      }
#endif
    }

    f64 gain = target_entropy - entropy;
#if DEBUG
    printf("\n\t\tentropy: %.16lf\n", entropy);
    printf("\t\tgain: %.16lf\n\n", gain);
#endif

    if (max_gain < gain) {
      max_gain = gain;
      max_gain_idx = feature;
    }
  }

#if DEBUG
  printf("\tMax gain: %.16lf\n", max_gain);
#endif
  return max_gain < threshold ? -1 : max_gain_idx;
}

fn DecisionTreeNode *ai_makeDTNode(Arena *arena, Arena *map_arena, CSV config,
                                   StringStream header,
                                   HashMap<String8, Occurrence> *maps,
                                   usize n_features, usize target_idx,
                                   f64 threshold) {
#if DEBUG
  printf("File: %.*s\n", Strexpand(config.file.path));
#endif
  usize data_row_start_at = config.offset;
  usize row_count = 0;

  /* Occurrences counter */
  for (StringStream row = csv_nextRow(map_arena, &config); row.size != 0;
       row = csv_header(map_arena, &config), ++row_count) {
    usize i = 0;
    String8 *row_entries = New(map_arena, String8, row.size);
    for (StringNode *r = row.first; r && i < n_features; r = r->next, ++i) {
      row_entries[i] = r->value;
    }

    for (i = 0; i < n_features; ++i) {
      Occurrence *node = maps[i].fromKey(map_arena, row_entries[i], Occurrence(map_arena));
      node->count += 1;

      if (i != target_idx) {
        Occurrence *node = maps[i].search(row_entries[i]);
        Occurrence *tnode = node->targets.fromKey(map_arena, row_entries[target_idx],
						  Occurrence(map_arena));
        tnode->count += 1;
      }
    }
  }

#if DEBUG
  /* Print table */
  for (usize i = 0; i < n_features; ++i) {
    for (HashMap<String8, Occurrence>::Slot slot : maps[i].slots) {
      if (slot.next == 0) {
        continue;
      }

      for (HashMap<String8, Occurrence>::KVNode *curr = slot.first; curr;
           curr = curr->next) {
        printf("`%.*s`: %ld\t", Strexpand(curr->key), curr->value.count);

        for (HashMap<String8, Occurrence>::Slot fslot :
             curr->value.targets.slots) {
          if (fslot.next == 0) {
            continue;
          }

          for (HashMap<String8, Occurrence>::KVNode *currf = fslot.first;
               currf; currf = currf->next) {
            printf("`%.*s`: %ld, ", Strexpand(currf->key), currf->value.count);
          }
        }

        printf("\n");
      }
    }
    printf("\n");
  }
#endif

  usize feature2split_by =
      ai_maxInformationGain(maps, n_features, target_idx, row_count, threshold);
#if DEBUG
  printf("\tFeature to split by: %ld\n", feature2split_by);
#endif
  if (feature2split_by == -1) {
    auto res = New(arena, DecisionTreeNode);
    usize count = 0;
    for (HashMap<String8, Occurrence>::Slot &slot :
         maps[target_idx].slots) {
      for (HashMap<String8, Occurrence>::KVNode *curr = slot.first; curr;
           curr = curr->next) {
        if (curr->value.count > count) {
          count = curr->value.count;
          res->label = curr->key;
        }
      }
    }
#if DEBUG
    printf("\tThe dataset will NOT be split further.\n\n");
    printf("\t============================================================\n\n");
#endif

    res->should_split_by = -1;
    return res;
  }

  usize branches = 0;
  for (HashMap<String8, Occurrence>::Slot &slot :
       maps[feature2split_by].slots) {
    for (HashMap<String8, Occurrence>::KVNode *curr = slot.first; curr;
         curr = curr->next) {
      branches += 1;
    }
  }

#if DEBUG
  printf("\tThe dataset will be split into %ld branches\n\n", branches);
  printf(
      "\t==============================================================\n\n");
#endif

  /* Iterator over the entire CSV file and write into the corresponding tmp */
  /*   file the CSV row. */
  config.offset = data_row_start_at;
  HashMap<String8, File> file_map(map_arena, strHash, branches);
  for (StringStream row = csv_nextRow(map_arena, &config); row.size != 0;
       row = csv_header(map_arena, &config), ++row_count) {
    usize i = 0;
    String8 *row_entries = New(map_arena, String8, row.size);
    for (StringNode *r = row.first; r && i < n_features; r = r->next, ++i) {
      row_entries[i] = r->value;
    }

    // Get the correct tmp file
    File *file = file_map.fromKey(map_arena, row_entries[feature2split_by], fs_openTmp(arena));

    i = (feature2split_by == 0 ? 1 : 0);
    fs_fileWrite(file, row_entries[i++]);
    for (; i < row.size; ++i) {
      if (i == feature2split_by) {
        continue;
      }
      fs_fileWrite(file, Strlit(","));
      fs_fileWrite(file, row_entries[i]);
    }
    fs_fileWrite(file, Strlit("\n"));
  }

  /* Call recursively to create the decision tree child nodes. */
  auto dt = New(arena, DecisionTreeNode);
  dt->should_split_by = feature2split_by;

  usize i = 0;
  for (StringNode *curr = header.first; curr; curr = curr->next, ++i) {
    // Skip the column used to split
    if (i == feature2split_by) {
      dt->label = curr->value;
      curr->prev->next = curr->next;
      curr->next->prev = curr->prev;
      --header.size;
      break;
    }
  }

  for (HashMap<String8, File>::Slot &slot : file_map.slots) {
    for (HashMap<String8, File>::KVNode *curr = slot.first; curr;
         curr = curr->next) {
      fs_fileForceSync(&curr->value);
    }
  }

  --n_features;
  --target_idx;

  using HashMap = HashMap<String8, Occurrence>;
  auto new_maps = New(
      map_arena, HashMap, n_features);

  for (HashMap::Slot &slot : maps[feature2split_by].slots) {
    for (HashMap::KVNode *curr = slot.first; curr; curr = curr->next) {
      for (usize i = 0; i < n_features; ++i) {
        new_maps[i] = HashMap(map_arena, strHash);
      }

      DecisionTreeNode *child =
          ai_makeDTNode(arena, map_arena,
                        (CSV){
                            .delimiter = ',',
                            .file = file_map[curr->key],
                        },
                        header, new_maps, n_features, target_idx, threshold);
      DLLPushBack(dt->first, dt->last, child);
    }
  }

  return dt;
}

fn DecisionTreeNode *ai_buildDecisionTree(Arena *arena, Arena *map_arena,
                                          CSV config, StringStream header,
                                          usize n_features,
                                          usize target_feature, f64 threshold) {
  Assert(target_feature <= n_features && target_feature > 0 && n_features > 1);

  if (!header.first) {
    header = csv_header(arena, &config);
  }

  using HashMap = HashMap<String8, Occurrence>;
  auto maps = New(arena, HashMap, n_features);
  for (usize i = 0; i < n_features; ++i) {
    maps[i] = HashMap(arena, strHash);
  }

  return ai_makeDTNode(arena, map_arena, config, header, maps, n_features,
                       --target_feature, threshold);
}
