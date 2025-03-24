template <typename T, typename U>
HashMap<T, U> HashMap<T, U>::init(Arena *arena, usize size) {
  HashMap res = {};
  arraylist_append_new(arena, &res.slots, size);
  return res;
}

template <typename T, typename U>
inline U& HashMap<T, U>::operator[](std::function<usize(T)> hash_fn, const T &key) {
  return *hashmap_search(this, hash_fn, key);
}

template <typename T, typename U, typename FN>
fn bool hashmap_insert(Arena *arena, HashMap<T, U> *map, FN hash_fn,
                       const T &key, const U &value) {
  using KVNode = HashMap<T, U>::KVNode;
  usize idx = hash_fn(key) % map->slots.total_size;

  KVNode *existing_node = 0;
  for(KVNode *n = map->slots[idx].first; n; n = n->next) {
    if (key == n->key) {
      existing_node = n;
      existing_node->value = value;
      break;
    }
  }

  if(!existing_node) {
    existing_node = New(arena, KVNode);
    if(!existing_node) { return false; }
    map->size += 1;
    map->slots[idx].size += 1;
    existing_node->key = key;
    existing_node->value = value;
    QueuePush(map->slots[idx].first, map->slots[idx].last, existing_node);

    // if ((f32)size/(f32)slots.size >= HIGH_LOAD_FACTOR) { rehash(arena); }
  }
  return true;
}

template <typename T, typename U, typename FN>
fn U* hashmap_search(HashMap<T, U> *map, FN hash_fn, const T &key) {
  using KVNode = HashMap<T, U>::KVNode;
  usize idx = hash_fn(key) % map->slots.total_size;
  for (KVNode *curr = map->slots[idx].first; curr; curr = curr->next) {
    if (key == curr->key) {
      return &curr->value;
    }
  }
  return 0;
}
