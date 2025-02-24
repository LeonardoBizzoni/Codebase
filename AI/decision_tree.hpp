#ifndef AI_DECISION_TREE
#define AI_DECISION_TREE

#include <math.h>

struct DTree {
  String8 label;
  u64 feature2split_by;

  // Siblings
  DTree *next;
  DTree *prev;

  // Childrens
  DTree *first;
  DTree *last;
};

struct Occurrence {
  u64 count;
  HashMap<String8, Occurrence> targets;

  Occurrence(Arena *arena) : count(0), targets(arena, strHash) {}
};

typedef u8 FeatureKind;
enum {
    FeatureKind_Category,
    FeatureKind_Continous,
};

struct Feature {
  FeatureKind kind;
  union {
    String8 name;
    f64 value;
  };
};

struct OccMapListNode {
  HashMap<String8, Occurrence> value;
  OccMapListNode *next = 0;
  OccMapListNode *prev = 0;
};

struct OccMapList {
  OccMapListNode *first;
  OccMapListNode *last;

  HashMap<String8, Occurrence>* operator[](usize i) {
    OccMapListNode *curr = first;
    for (; curr && i > 0; curr = curr->next, --i);
    if (i > 0 || !curr) { return 0; }
    return &curr->value;
  }
};

fn Array<Array<Feature>> ai_chunk2features(Arena *arena, File dataset, isize offset,
					   u32 n_features, u32 chunk_size,
					   StringStream (*next_line)(Arena*, File, isize));

fn DTree ai_dtree_makeNode(Arena *arena, File dataset, isize offset, u32 n_features,
			   u32 target_feature_idx, f32 entropy_threshold, u32 chunk_size,
			   StringStream (*next_line)(Arena*, File, isize));

fn DTree ai_dtree_build(Arena *arena, File dataset, StringStream *header, u32 n_features,
			u32 target_feature_idx, f32 entropy_threshold = 1e-4,
			u32 chunk_size = 4096,
			StringStream (*next_line)(Arena *arena, File dataset,
						  isize offset) = csv_nextRow);

fn i32 ai_dtree_classify(DTree tree, StringStream input);

#endif
