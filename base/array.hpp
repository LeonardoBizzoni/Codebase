#ifndef BASE_ARRAY
#define BASE_ARRAY

template <typename T>
struct ArrayList {
  struct Node {
    T *block;
    usize size;
    Node *next = 0;
    Node *prev = 0;
  };

  usize node_count = 0;
  usize capacity = 0;
  Node *first = 0;
  Node *last = 0;

  T& operator[](usize i) const;
};

template <typename T>
fn void arraylist_append(Arena *arena, ArrayList<T> *list, T *array, usize size);

template <typename T>
fn void arraylist_append_new(Arena *arena, ArrayList<T> *list, usize size);

template <typename T>
fn void arraylist_concat(ArrayList<T> *list, ArrayList<T> *other);

#endif
