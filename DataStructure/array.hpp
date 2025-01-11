#ifndef BASE_ARRAY
#define BASE_ARRAY

#include "../base.h"

template <typename T, usz N>
struct Buffer {
  T values[N];
  usz size;

  Buffer() : values{0}, size(N) {}

  template<typename... Ts>
  Buffer(Ts... args) : values{args...}, size(N) {}

  T& operator[](usz i) {
    Assert(i < size);
    return values[i];
  }

  struct Iterator {
    T *current;

    explicit Iterator(T *ptr) : current(ptr) {}

    T& operator*() { return *current; }
    T* operator->() { return current; }

    Iterator& operator++() {
      current += 1;
      return *this;
    }

    Iterator& operator--() {
      current -= 1;
      return *this;
    }

    Iterator& operator+=(usz step) {
      current += step;
      return *this;
    }

    Iterator& operator-=(usz step) {
      current -= step;
      return *this;
    }

    bool operator==(const Iterator& other) { return current == other.current; }
    bool operator!=(const Iterator& other) { return current != other.current; }
  };

  Iterator begin() { return Iterator(&values[0]); }
  Iterator end() { return Iterator(&values[N]); }
};

template <typename T>
struct Array {
  T *values;
  usz size;

  Array(Arena *arena, usz size) :
    values((New(arena, T, size)), size(size) {
    Assert(size > 0);
  }

  T& operator[](usz i) {
    Assert(i < size);
    return values[i];
  }

  struct Iterator {
    T *current;

    explicit Iterator(T *ptr) : current(ptr) {}

    T& operator*() { return *current; }
    T* operator->() { return current; }

    Iterator& operator++() {
      current += 1;
      return *this;
    }

    Iterator& operator--() {
      current -= 1;
      return *this;
    }

    Iterator& operator+=(usz step) {
      current += step;
      return *this;
    }

    Iterator& operator-=(usz step) {
      current -= step;
      return *this;
    }

    bool operator==(const Iterator& other) { return current == other.current; }
    bool operator!=(const Iterator& other) { return current != other.current; }
  };

  Iterator begin() { return Iterator(&values[0]); }
  Iterator end() { return Iterator(&values[size]); }
};

template <typename T>
struct ArrayListNode {
  Array<T> block;
  ArrayListNode *next = 0;
  ArrayListNode *prev = 0;
};

template <typename T>
struct ArrayList {
  ArrayListNode<T> *first = 0;
  ArrayListNode<T> *last = 0;

  struct Iterator {
    ArrayListNode<T> *current;
    usz idx;

    explicit Iterator(ArrayListNode<T> *ptr, usz idx) :
      current(ptr), idx(idx) {}

    Iterator& operator++() {
      ++idx;
      if (idx >= current->block.size) {
        current = current->next;
        idx = 0;
      }

      return *this;
    }

    Iterator& operator--() {
      if (idx == 0) {
        current = current->prev;
        idx = current->block.size - 1;
      } else {
        --idx;
      }

      return *this;
    }

    T& operator*() { return current->block[idx]; }

    bool operator==(const Iterator& other) {
      return current == other.current && idx == other.idx;
    }

    bool operator!=(const Iterator& other) {
      return !(*this == other);
    }
  };

  Iterator begin() { return Iterator(first, 0); }
  Iterator end() { return Iterator(last, last->block.size); }

};

#endif
