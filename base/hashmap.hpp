#ifndef BASE_HASHMAP
#define BASE_HASHMAP

#include <functional>

#define HIGH_LOAD_FACTOR 0.75f

template <typename T, typename U>
struct HashMap {
  struct KVNode {
    T key;
    U value;
    KVNode *next;
  };

  struct Slot {
    KVNode *first;
    KVNode *last;
    usize size;
  };

  usize capacity;
  usize size;
  ArrayList<Slot> slots;

  static HashMap<T, U> init(Arena *arena, usize size);

  inline U& operator[](std::function<usize(T)> hash_fn, const T &key);
};

template <typename T, typename U, typename FN>
fn U* hashmap_search(HashMap<T, U> *map, FN hash_fn, const T &key);

template <typename T, typename U, typename FN>
fn bool hashmap_insert(Arena *arena, HashMap<T, U> *map, FN hash_fn,
                       const T &key, const U &value);

#endif
