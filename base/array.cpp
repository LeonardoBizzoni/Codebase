template <typename T>
T& ArrayList<T>::operator[](usize i) const {
  Assert(i < capacity);

  for (Node *it = first; it; it = it->next) {
    if (i < it->size) {
      return (T&)it->block[i];
    } else {
      i -= it->size;
    }
  }

  Assert(false);
  return (T&)first->block[0];
}

template <typename T>
fn void arraylist_append(Arena *arena, ArrayList<T> *list, T *array, usize size) {
  if (!size || !array) { return; }
  list->node_count += 1;
  list->capacity += size;

  using Node = ArrayList<T>::Node;
  Node *node = New(arena, Node);
  node->size = size;
  node->block = array;

  DLLPushBack(list->first, list->last, node);
}

template <typename T>
fn void arraylist_append_new(Arena *arena, ArrayList<T> *list, usize size) {
  if (!size) { return; }
  list->node_count += 1;
  list->capacity += size;

  using Node = ArrayList<T>::Node;
  Node *node = New(arena, Node);
  node->size = size;
  node->block = New(arena, T, size);

  DLLPushBack(list->first, list->last, node);
}

template <typename T>
fn void arraylist_concat(ArrayList<T> *list, ArrayList<T> *other) {
  list->node_count += other->node_count;
  list->capacity += other->capacity;

  DLLPushBack(list->first, list->last, other->first);
}
