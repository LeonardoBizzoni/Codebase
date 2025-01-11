#ifndef BASE_DYNARRAY
#define BASE_DYNARRAY

#include "array.hpp"

template <typename T>
struct DynArray {
  usz size;

  ArrayListNode<T> *first = 0;
  ArrayListNode<T> *last = 0;

  DynArray(Arena *arena, usz initial_capacity = 8)
  : size(initial_capacity) {
    Assert(initial_capacity > 0);
    first = last = New(arena, ArrayListNode<T>);
    first->block = Array<T>(arena, initial_capacity);
  }

  T& operator[](usz i) {
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

  struct Iterator {
    ArrayListNode<T>* current;
    usz idx;

    Iterator(ArrayListNode<T> *ptr, usz idx) : current(ptr), idx(idx) {}

    T& operator*() { return current->block[idx]; }

    Iterator& operator++() {
      ++idx;
      if (idx >= current->block.size) {
        current = current->next;
        idx = 0;
      }

      return *this;
    }

    bool operator==(const Iterator& other) {
      return current == other.current && idx == other.idx;
    }

    bool operator!=(const Iterator& other) { return !(*this == other); }
  };

  Iterator begin() { return Iterator(first, 0); }
  Iterator end() { return Iterator(0, 0); }
};

template <typename T>
fn bool dynarrayExpand(Arena *arena, DynArray<T> *arr,
		       usz expansion_size);


#endif
