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

  bool perfect;
  usize size;
  usize (*hasher)(T);
  ArrayList<Slot> slots;

  inline U& operator[](const T &key);
  static HashMap<T, U> init(Arena *arena, usize (*hash_fn)(T),
                            usize size, bool perfect = false);
};

template <typename T, typename U>
fn bool hashmap_insert(Arena *arena, HashMap<T, U> *map,
                       const T &key, const U &value);

template <typename T, typename U>
fn U* hashmap_search(HashMap<T, U> *map, const T &key);

template <typename T, typename U>
fn U* hashmap_from_key(Arena *arena, HashMap<T, U> *map,
                       const T &key, const U &default_val = {});

template <typename T, typename U>
fn void hashmap_remove(HashMap<T, U> *map, const T &key);

template <typename T, typename U>
fn void hashmap_rehash(HashMap<T, U> *map);

#endif
