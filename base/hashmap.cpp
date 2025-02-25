template <typename T, typename U>
HashMap<T, U>::HashMap(Arena *arena, usize (*hasher)(T), usize capacity) :
  size(0), hasher(hasher), slots(arena, capacity) {}

template <typename T, typename U>
bool HashMap<T, U>::insert(Arena *arena, const T &key, const U &value) {
  usize idx = hasher(key) % slots.size;

  KVNode *existing_node = 0;
  for(KVNode *n = slots[idx].first; n; n = n->next) {
    if (key == n->key) {
      existing_node = n;
      existing_node->value = value;
      break;
    }
  }
  if(!existing_node) {
    existing_node = New(arena, KVNode);
    if(!existing_node) { return false; }
    size += 1;
    slots[idx].size += 1;
    existing_node->key = key;
    existing_node->value = value;
    QueuePush(slots[idx].first, slots[idx].last, existing_node);

    if ((f32)size/(f32)slots.size >= HIGH_LOAD_FACTOR) { rehash(arena); }
  }
  return true;
}

template <typename T, typename U>
U* HashMap<T, U>::search(const T &key) {
  usize idx = hasher(key) % slots.size;
  for (KVNode *curr = slots[idx].first; curr; curr = curr->next) {
    if (key == curr->key) {
      return &curr->value;
    }
  }
  return 0;
}

template <typename T, typename U>
U* HashMap<T, U>::fromKey(Arena *arena, const T &key, const U &default_val) {
  usize idx = hasher(key) % slots.size;
  KVNode *existing_node = 0;
  for(KVNode *n = slots[idx].first; n; n = n->next) {
    if (n->key == key) {
      existing_node = n;
      break;
    }
  }
  if(!existing_node) {
    size += 1;
    slots[idx].size += 1;
    existing_node = New(arena, KVNode);
    existing_node->key = key;
    existing_node->value = default_val;
    QueuePush(slots[idx].first, slots[idx].last, existing_node);

    if ((f32)size/(f32)slots.size >= HIGH_LOAD_FACTOR) { rehash(arena); }
  }

  return &existing_node->value;
}

template <typename T, typename U>
void HashMap<T, U>::remove(const T &key) {
  usize idx = hasher(key) % slots.size;

  for(KVNode *curr = slots[idx].first, *prev = 0;
       curr;
       prev = curr, curr = curr->next) {
    if (curr->key == key) {
      prev->next = curr->next;
    }
  }
}

template <typename T, typename U>
void HashMap<T, U>::rehash(Arena *arena) {
  Scratch scratch = ScratchBegin(&arena, 1);
  Warn(strFormat(scratch.arena, "High load factor with capacity %ld.", slots.size));

  DynArray<Slot> copy(scratch.arena, slots.size);
  for (usize i = 0; i < slots.size; ++i) {
    if (!slots[i].first) { continue; }
    copy[i].size = slots[i].size;

    usize j = 0;
    KVNode *curr_copies = New(scratch.arena, KVNode, slots[i].size);
    for (KVNode *curr = slots[i].first; curr; curr = curr->next, ++j) {
      memCopy(&curr_copies[j], curr, sizeof(HashMap<T, U>::KVNode));
      QueuePush(copy[i].first, copy[i].last, &curr_copies[j]);
    }

    slots[i].size = 0;
    slots[i].first = slots[i].last = 0;
  }

  usize prev_cap = slots.size;
  Assert(dynarray_expand(arena, &slots, slots.size));

  for (usize i = 0; i < prev_cap; ++i) {
    usize j = 0;
    KVNode *curr_copies = New(arena, KVNode, copy[i].size);
    for (KVNode *curr = copy[i].first; curr; curr = curr->next, ++j) {
      usize idx = hasher(curr->key) % slots.size;
      memCopy(&curr_copies[j], curr, sizeof(KVNode) - sizeof(void*));
      QueuePush(slots[idx].first, slots[idx].last, &curr_copies[j]);
      slots[idx].size += 1;
    }
  }
  ScratchEnd(scratch);
}

template <typename T, typename U>
inline U& HashMap<T, U>::operator[](const T &key) {
  return *search(key);
}
