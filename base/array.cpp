// =============================================================================
// Buffer
template <typename T, usize N>
Buffer<T, N>::Buffer() : values{0}, size(N) {}

template<typename T, usize N>
template<typename... Ts>
Buffer<T, N>::Buffer(Ts... args) : values{args...}, size(N) {}

template <typename T, usize N>
T& Buffer<T, N>::operator[](usize i) {
  Assert(i < size);
  return values[i];
}

template <typename T, usize N>
Buffer<T, N>::Iterator Buffer<T, N>::begin() { return Iterator(&values[0]); }

template <typename T, usize N>
Buffer<T, N>::Iterator Buffer<T, N>::end() { return Iterator(&values[N]); }

// =============================================================================
// Buffer iterator
template<typename T, usize N>
Buffer<T, N>::Iterator::Iterator(T *ptr) : current(ptr) {}

template<typename T, usize N>
T& Buffer<T, N>::Iterator::operator*() { return *current; }

template<typename T, usize N>
T* Buffer<T, N>::Iterator::operator->() { return current; }

template<typename T, usize N>
Buffer<T, N>::Iterator& Buffer<T, N>::Iterator::operator++() {
  current += 1;
  return *this;
}

template<typename T, usize N>
Buffer<T, N>::Iterator& Buffer<T, N>::Iterator::operator--() {
  current -= 1;
  return *this;
}

template<typename T, usize N>
Buffer<T, N>::Iterator& Buffer<T, N>::Iterator::operator+=(usize step) {
  current += step;
  return *this;
}

template<typename T, usize N>
Buffer<T, N>::Iterator& Buffer<T, N>::Iterator::operator-=(usize step) {
  current -= step;
  return *this;
}

template<typename T, usize N>
bool Buffer<T, N>::Iterator::operator==(const Iterator& other) {
  return current == other.current;
}

template<typename T, usize N>
bool Buffer<T, N>::Iterator::operator!=(const Iterator& other) {
  return current != other.current;
}

// =============================================================================
// Array
template <typename T>
Array<T>::Array(Arena *arena, usize size) : values(New(arena, T, size)), size(size) {
  Assert(size > 0);
}

template <typename T>
T& Array<T>::operator[](usize i) {
  Assert(i < size);
  return values[i];
}

template <typename T>
Array<T>::Iterator Array<T>::begin() { return Iterator(&values[0]); }

template <typename T>
Array<T>::Iterator Array<T>::end() { return Iterator(&values[size]); }

// =============================================================================
// Array iterator
template <typename T>
Array<T>::Iterator::Iterator(T *ptr) : current(ptr) {}

template <typename T>
T& Array<T>::Iterator::operator*() { return *current; }

template <typename T>
T* Array<T>::Iterator::operator->() { return current; }

template <typename T>
Array<T>::Iterator& Array<T>::Iterator::operator++() {
  current += 1;
  return *this;
}

template <typename T>
Array<T>::Iterator& Array<T>::Iterator::operator--() {
  current -= 1;
  return *this;
}

template <typename T>
Array<T>::Iterator& Array<T>::Iterator::operator+=(usize step) {
  current += step;
  return *this;
}

template <typename T>
Array<T>::Iterator& Array<T>::Iterator::operator-=(usize step) {
  current -= step;
  return *this;
}

template <typename T>
bool Array<T>::Iterator::operator==(const Iterator& other) {
  return current == other.current;
}

template <typename T>
bool Array<T>::Iterator::operator!=(const Iterator& other) {
  return current != other.current;
}

// =============================================================================
// Array functions

// =============================================================================
// ArrayList
template <typename T>
ArrayList<T>::Iterator ArrayList<T>::begin() { return Iterator(first, 0); }

template <typename T>
ArrayList<T>::Iterator ArrayList<T>::end() { return Iterator(last, last->block.size); }

// =============================================================================
// ArrayList iterator
template <typename T>
ArrayList<T>::Iterator::Iterator(ArrayListNode<T> *ptr, usize idx) :
  current(ptr), idx(idx) {}

template <typename T>
T& ArrayList<T>::Iterator::operator*() { return current->block[idx]; }

template <typename T>
ArrayList<T>::Iterator& ArrayList<T>::Iterator::operator++() {
  ++idx;
  if (idx >= current->block.size) {
    current = current->next;
    idx = 0;
  }

  return *this;
}

template <typename T>
ArrayList<T>::Iterator& ArrayList<T>::Iterator::operator--() {
  if (idx == 0) {
    current = current->prev;
    idx = current->block.size - 1;
  } else {
    --idx;
  }

  return *this;
}

template <typename T>
bool ArrayList<T>::Iterator::operator==(const Iterator& other) {
  return current == other.current && idx == other.idx;
}

template <typename T>
bool ArrayList<T>::Iterator::operator!=(const Iterator& other) {
  return !(*this == other);
}
