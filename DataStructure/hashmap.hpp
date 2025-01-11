#ifndef BASE_HASHMAP
#define BASE_HASHMAP

#include "../base.h"

#include "dynarray.hpp"

template <typename T, typename U>
struct HashMap {
  usz size;
  usz (*hasher)(T);

  struct KVNode {
    T key;
    U value;
    KVNode *next;
  };

  struct Slot {
    KVNode *first;
    KVNode *last;
  };

  DynArray<Slot> slots;

  HashMap(Arena *arena, usz (*hasher)(T), usz size = 16) :
    size(size), hasher(hasher), slots(arena, size) {}

  bool insert(Arena *arena, const T &key, const U &value) {
    usz idx = hasher(key) % slots.size;

    KVNode *existing_node = 0;
    for(KVNode *n = slots[idx].first; n; n = n->next) {
      if (key == n->key) {
        existing_node = n;
	existing_node->value = value;
        break;
      }
    }
    if(existing_node == 0) {
      existing_node = New(arena, KVNode);
      if(existing_node == 0) { return false; }
      existing_node->key = key;
      existing_node->value = value;
      QueuePush(slots[idx].first, slots[idx].last, existing_node);
    }
    return true;
  }

  U* search(const T &key) {
    usz idx = hasher(key) % slots.size;
    for (KVNode *curr = slots[idx].first; curr; curr = curr->next) {
      if (key == curr->key) {
	return &curr->value;
      }
    }
    return 0;
  }

  U* fromKey(Arena *arena, const T &key, const U &default_val = {}) {
    usz idx = hasher(key) % slots.size;
    KVNode *existing_node = 0;
    for(KVNode *n = slots[idx].first; n; n = n->next) {
      if (n->key == key) {
        existing_node = n;
        break;
      }
    }
    if(existing_node == 0) {
      existing_node = New(arena, KVNode);
      existing_node->key = key;
      existing_node->value = default_val;
      QueuePush(slots[idx].first, slots[idx].last, existing_node);
    }

    return &existing_node->value;
  }

  void remove(const T &key) {
    usz idx = hasher(key) % slots.size;

    for(KVNode *curr = slots[idx].first, *prev = 0;
	curr;
	prev = curr, curr = curr->next) {
      if (curr->key == key) {
	prev->next = curr->next;
      }
    }
  }

  inline U& operator[](const T &key) {
    return *search(key);
  }
};

#endif
