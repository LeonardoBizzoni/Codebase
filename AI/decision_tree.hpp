#ifndef AI_DECISION_TREE
#define AI_DECISION_TREE

#include <math.h>

#define DEFAULT_CHUNK_SIZE 4096
#define DEFAULT_ENTROPY 1e-4

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

struct Category {
  u64 freq;
  HashMap<String8, Category> target;

  Category(Arena *arena) : freq(0), target(arena, strHash) {}
};
using CategoryMap = HashMap<String8, Category>;

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

struct BranchCondition {
  f32 gini;
  isize split_idx;

  f32 continous_threshold;
};

fn Array<Array<Feature>> ai_chunk2features(Arena *arena, File dataset, isize offset,
					   u32 n_features, u32 chunk_size,
					   StringStream (*next_line)(Arena*, File,
								     isize));

fn DTree ai_dtree_makeNode(Arena *arena, File dataset, isize offset, u32 n_features,
			   u32 target_feature_idx, f32 entropy_threshold, u32 chunk_size,
			   StringStream (*next_line)(Arena*, File, isize));

fn DTree ai_dtree_build(Arena *arena, File dataset, StringStream *header, u32 n_features,
			u32 target_feature_idx, f32 entropy_threshold = DEFAULT_ENTROPY,
			u32 chunk_size = DEFAULT_CHUNK_SIZE,
			StringStream (*next_line)(Arena *arena, File dataset,
						  isize *offset) = csv_nextRow);

fn i32 ai_dtree_classify(DTree tree, StringStream input);

#endif
