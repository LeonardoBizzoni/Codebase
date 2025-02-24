#ifndef BASE_HASHMAP
#define BASE_HASHMAP

template <typename T, typename U>
struct HashMap {
  usize capacity;
  usize size;
  usize (*hasher)(T);

  struct KVNode {
    T key;
    U value;
    KVNode *next;
  };

  struct Slot {
    KVNode *first;
    KVNode *last;
  };

  DynArray<Slot> slots;

  HashMap(Arena *arena, usize (*hasher)(T), usize capacity = 16) :
    capacity(capacity), hasher(hasher), slots(arena, capacity) {}

  bool insert(Arena *arena, const T &key, const U &value) {
    usize idx = hasher(key) % slots.size;

    KVNode *existing_node = 0;
    for(KVNode *n = slots[idx].first; n; n = n->next) {
      if (key == n->key) {
        existing_node = n;
	existing_node->value = value;
        break;
      }
    }
    if(existing_node == 0) {
      existing_node = New(arena, KVNode);
      if(existing_node == 0) { return false; }
      size += 1;
      existing_node->key = key;
      existing_node->value = value;
      QueuePush(slots[idx].first, slots[idx].last, existing_node);

      if ((f32)size/(f32)capacity >= 0.75f) { rehash(); }
    }
    return true;
  }

  U* search(const T &key) {
    usize idx = hasher(key) % slots.size;
    for (KVNode *curr = slots[idx].first; curr; curr = curr->next) {
      if (key == curr->key) {
	return &curr->value;
      }
    }
    return 0;
  }

  U* fromKey(Arena *arena, const T &key, const U &default_val = {}) {
    usize idx = hasher(key) % slots.size;
    KVNode *existing_node = 0;
    for(KVNode *n = slots[idx].first; n; n = n->next) {
      if (n->key == key) {
        existing_node = n;
        break;
      }
    }
    if(existing_node == 0) {
      size += 1;
      existing_node = New(arena, KVNode);
      existing_node->key = key;
      existing_node->value = default_val;
      QueuePush(slots[idx].first, slots[idx].last, existing_node);

      if ((f32)size/(f32)capacity >= 0.75f) { rehash(); }
    }

    return &existing_node->value;
  }

  void remove(const T &key) {
    usize idx = hasher(key) % slots.size;

    for(KVNode *curr = slots[idx].first, *prev = 0;
	curr;
	prev = curr, curr = curr->next) {
      if (curr->key == key) {
	prev->next = curr->next;
      }
    }
  }

  void rehash() {
  }

  inline U& operator[](const T &key) {
    return *search(key);
  }
};

#endif
