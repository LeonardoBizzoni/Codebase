#ifndef BASE_ARRAY
#define BASE_ARRAY

template <typename T, usize N>
struct Buffer {
  struct Iterator {
    T *current;

    Iterator(T *ptr);

    T& operator*();
    T* operator->();
    Iterator& operator++();
    Iterator& operator--();
    Iterator& operator+=(usize step);
    Iterator& operator-=(usize step);
    bool operator==(const Iterator& other);
    bool operator!=(const Iterator& other);
  };

  T values[N];
  usize size;

  Buffer();

  template<typename... Ts>
  Buffer(Ts... args);

  T& operator[](usize i);

  Iterator begin();
  Iterator end();
};

template <typename T>
struct Array {
  struct Iterator {
    T *current;

    Iterator(T *ptr);

    T& operator*();
    T* operator->();
    Iterator& operator++();
    Iterator& operator--();
    Iterator& operator+=(usize step);
    Iterator& operator-=(usize step);
    bool operator==(const Iterator& other);
    bool operator!=(const Iterator& other);
  };

  T *values;
  usize size;

  Array(Arena *arena, usize size);

  T& operator[](usize i);

  Iterator begin();
  Iterator end();
};

template <typename T>
struct ArrayListNode {
  Array<T> block;
  ArrayListNode *next = 0;
  ArrayListNode *prev = 0;
};

template <typename T>
struct ArrayList {
  struct Iterator {
    ArrayListNode<T> *current;
    usize idx;

    Iterator(ArrayListNode<T> *ptr, usize idx);

    T& operator*();
    Iterator& operator++();
    Iterator& operator--();
    bool operator==(const Iterator& other);
    bool operator!=(const Iterator& other);
  };

  ArrayListNode<T> *first = 0;
  ArrayListNode<T> *last = 0;

  Iterator begin();
  Iterator end();
};

#endif
