#ifndef BASE_DYNARRAY
#define BASE_DYNARRAY

template <typename T>
struct DynArray {
  struct Iterator {
    ArrayListNode<T>* current;
    usize idx;

    Iterator(ArrayListNode<T> *ptr, usize idx);

    T& operator*();
    Iterator& operator++();
    bool operator==(const Iterator& other);
    bool operator!=(const Iterator& other);
  };

  usize size;
  ArrayListNode<T> *first = 0;
  ArrayListNode<T> *last = 0;

  DynArray(Arena *arena, usize initial_capacity = 8);

  T& operator[](usize i);

  Iterator begin();
  Iterator end();
};

template <typename T>
fn bool dynarray_expand(Arena *arena, DynArray<T> *arr, usize size_increment);

#endif
