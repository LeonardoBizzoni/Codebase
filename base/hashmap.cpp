template <typename T, typename U>
HashMap<T, U> HashMap<T, U>::init(Arena *arena, usize (*hash_fn)(T),
                                  usize size, bool perfect) {
  HashMap res = {};
  res.hasher = hash_fn;
  res.perfect = perfect;
  arraylist_append_new(arena, &res.slots, size);
  return res;
}

template <typename T, typename U>
inline U& HashMap<T, U>::operator[](const T &key) {
  return *hashmap_search(this, key);
}

template <typename T, typename U>
fn bool hashmap_insert(Arena *arena, HashMap<T, U> *map,
                       const T &key, const U &value) {
  using KVNode = HashMap<T, U>::KVNode;
  usize idx = map->hasher(key) % map->slots.capacity;

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

    if (!map->perfect &&
        (f32)map->size/(f32)map->slots.capacity >= HIGH_LOAD_FACTOR) {
      Warn(Strlit("High load factor"));
      hashmap_rehash(map);
    }
  }
  return true;
}

template <typename T, typename U>
fn U* hashmap_search(HashMap<T, U> *map, const T &key) {
  using KVNode = HashMap<T, U>::KVNode;
  usize idx = map->hasher(key) % map->slots.capacity;
  for (KVNode *curr = map->slots[idx].first; curr; curr = curr->next) {
    if (key == curr->key) {
      return &curr->value;
    }
  }
  return 0;
}

template <typename T, typename U>
fn U* hashmap_from_key(Arena *arena, HashMap<T, U> *map,
                       const T &key, const U &default_val) {
  using KVNode = HashMap<T, U>::KVNode;
  usize idx = map->hasher(key) % map->slots.capacity;
  KVNode *existing_node = 0;

  for(KVNode *n = map->slots[idx].first; n; n = n->next) {
    if (n->key == key) {
      existing_node = n;
      break;
    }
  }
  if(!existing_node) {
    map->size += 1;
    map->slots[idx].size += 1;
    existing_node = New(arena, KVNode);
    existing_node->key = key;
    existing_node->value = default_val;
    QueuePush(map->slots[idx].first, map->slots[idx].last, existing_node);

    if (!map->perfect &&
        (f32)map->size/(f32)map->slots.capacity >= HIGH_LOAD_FACTOR) {
      Warn(Strlit("High load factor"));
      hashmap_rehash(map);
    }
  }

  return &existing_node->value;
}

template <typename T, typename U>
fn void hashmap_remove(HashMap<T, U> *map, const T &key) {
  usize idx = map->hasher(key) % map->slots.capacity;
  for(typename HashMap<T, U>::KVNode *curr = map->slots[idx].first, *prev = 0;
       curr; prev = curr, curr = curr->next) {
    if (curr->key == key) {
      if (prev) { prev->next = curr->next; }
      else { map->slots[idx].first = curr->next; }
      break;
    }
  }
}

template <typename T, typename U>
fn void hashmap_rehash(HashMap<T, U> *map) {
  Scratch scratch = ScratchBegin(0, 0);
  using KVNode = HashMap<T, U>::KVNode;
  using Slot = HashMap<T, U>::Slot;

  KVNode *freelist = 0;
  ArrayList<Slot> slots_copy = {0};
  arraylist_append_new(scratch.arena, &slots_copy, map->slots.capacity);

  for (usize slot = 0; slot < map->slots.capacity; ++slot) {
    usize j = 0;
    KVNode *node_copies = New(scratch.arena, KVNode, map->slots[slot].size);
    for(KVNode *curr = map->slots[slot].first; curr; curr = curr->next) {
      node_copies[j].key = curr->key;
      node_copies[j].value = curr->value;
      QueuePush(slots_copy[slot].first, slots_copy[slot].last, &node_copies[j++]);
    }

    for(KVNode *curr = map->slots[slot].first, *next = 0; curr; curr = next) {
      next = curr->next;
      curr->next = 0;
      StackPush(freelist, curr);
    }
    map->slots[slot].size = 0;
    map->slots[slot].first = map->slots[slot].last = 0;
  }

  for (usize slot = 0; slot < slots_copy.capacity; ++slot) {
    for(KVNode *curr = slots_copy[slot].first; curr; curr = curr->next) {
      usize idx = map->hasher(curr->key) % slots_copy.capacity;

      KVNode *node = freelist;
      Assert(node && freelist);
      freelist = freelist->next;
      node->key = curr->key;
      node->value = curr->value;
      node->next = 0;
      QueuePush(map->slots[idx].first, map->slots[idx].last, node);

      map->slots[idx].size += 1;
    }
  }

  ScratchEnd(scratch);
}
