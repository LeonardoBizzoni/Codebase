template <typename T>
DynArray<T>::DynArray(Arena *arena, usize initial_capacity) : size(initial_capacity) {
  Assert(initial_capacity > 0);
  first = last = New(arena, ArrayListNode<T>);
  first->block = Array<T>(arena, initial_capacity);
}

template <typename T>
T& DynArray<T>::operator[](usize i) {
  Assert(i < size);

  for (ArrayListNode<T> *it = first; it; it = it->next) {
    if (i < it->block.size) {
      return it->block[i];
    } else {
      i -= it->block.size;
    }
  }

  Assert(false);
  return first->block[0];
}

template <typename T>
DynArray<T>::Iterator DynArray<T>::begin() { return DynArray<T>::Iterator(first, 0); }

template <typename T>
DynArray<T>::Iterator DynArray<T>::end() { return DynArray<T>::Iterator(0, 0); }

// =============================================================================
// Dynarray iterator
template <typename T>
DynArray<T>::Iterator::Iterator(ArrayListNode<T> *ptr, usize idx) :
  current(ptr), idx(idx) {}

template <typename T>
T& DynArray<T>::Iterator::operator*() { return current->block[idx]; }

template <typename T>
DynArray<T>::Iterator& DynArray<T>::Iterator::operator++() {
  ++idx;
  if (idx >= current->block.size) {
    current = current->next;
    idx = 0;
  }

  return *this;
}

template <typename T>
bool DynArray<T>::Iterator::operator==(const Iterator& other) {
  return current == other.current && idx == other.idx;
}

template <typename T>
bool DynArray<T>::Iterator::operator!=(const Iterator& other) {
  return !(*this == other);
}

// =============================================================================
// Dynarray functions
template <typename T>
fn bool dynarray_expand(Arena *arena, DynArray<T> *arr, usize size_increment) {
  ArrayListNode<T> *new_block = New(arena, ArrayListNode<T>);
  if (!new_block) { return false; }
  arr->size += size_increment;
  new_block->block = Array<T>(arena, size_increment);
  DLLPushBack(arr->first, arr->last, new_block);
  return true;
}
