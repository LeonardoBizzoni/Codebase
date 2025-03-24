#ifndef BASE_ARRAY
#define BASE_ARRAY

template <typename T>
struct ArrayList {
  struct Node {
    T *block;
    usize size;
    ArrayList::Node *next = 0;
    ArrayList::Node *prev = 0;
  };

  usize node_count = 0;
  usize total_size = 0;
  Node *first = 0;
  Node *last = 0;

  T& operator[](usize i) const;
};

template <typename T>
fn void arraylist_append(Arena *arena, ArrayList<T> *list, usize size);

template <typename T>
fn void arraylist_append_new(Arena *arena, ArrayList<T> *list, usize size);

template <typename T>
fn void arraylist_concat(ArrayList<T> *list, ArrayList<T> *other);

#endif
