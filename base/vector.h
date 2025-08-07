#ifndef BASE_VECTOR_H
#define BASE_VECTOR_H

typedef struct {
  union {
    f32 values[2];
    struct {
      f32 x, y;
    };
  };
} Vec2f32;

#endif
