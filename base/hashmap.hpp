#ifndef BASE_HASHMAP
#define BASE_HASHMAP

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
  usize (*hasher)(T);
  DynArray<Slot> slots;

  HashMap(Arena *arena, usize (*hasher)(T), usize capacity = 16);

  bool insert(Arena *arena, const T &key, const U &value);
  U* search(const T &key);
  U* fromKey(Arena *arena, const T &key, const U &default_val = {});
  void remove(const T &key);
  void rehash(Arena *arena);

  inline U& operator[](const T &key);
};

#endif
