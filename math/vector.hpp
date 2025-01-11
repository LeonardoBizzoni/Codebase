#ifndef BASE_VECTOR
#define BASE_VECTOR

#include <math.h>
#include "../base.h"

template <typename T, usz D>
struct Vector {
  T values[D];

  inline T& x() {
    return values[0];
  }

  inline T& y() {
    return values[1];
  }

  inline T& z() {
    return values[2];
  }

  inline T& w() {
    return values[3];
  }

  T dot(Vector &other) {
    T res = 0;
    for (usz i = 0; i < D; ++i) {
      res += values[i] * other.values[i];
    }

    return res;
  }

  Vector<T, 3> cross(Vector<T, 3> &other) {
    Assert(D == 3);
    return (Vector<T, 3>) {
      .values = {
	y()*other.z()-z()*other.y(),
	z()*other.x()-x()*other.z(),
	x()*other.y()-y()*other.x(),
      },
    };
  }

  Vector mulElementWise(Vector &other) {
    Vector res = *this;
    for (usz i = 0; i < D; ++i) {
      res *= other[i];
    }

    return res;
  }

  // the `||vector||` thing
  f32 magnitude() {
    f32 res = 0.f;
    for (usz i = 0; i < D; ++i) {
      res += Abs(values[i]) * Abs(values[i]);
    }

    return sqrtf(res);
  }

  f64 magnitude64() {
    f64 res = 0.0;
    for (usz i = 0; i < D; ++i) {
      res += Abs(values[i]) * Abs(values[i]);
    }

    return sqrt(res);
  }

  // the `vector / ||vector||` thing
  Vector normalize() {
    Vector res = {0};

    f32 length = magnitude();
    if (!length) {
      return res;
    }

    for (usz i = 0; i < D; ++i) {
      res.values[i] = values[i] / length;
    }

    return res;
  }

  Vector proj(Vector &other) {
    return (*this) * (other.dot(*this) / dot(*this));
  }

  Vector operator+(Vector &other) {
    Vector res;
    for (usz i = 0; i < D; ++i) {
      res.values[i] = values[i] + other.values[i];
    }

    return res;
  }

  void operator+=(Vector &other) {
    for (usz i = 0; i < D; ++i) {
      values[i] += other.values[i];
    }
  }

  Vector operator-(Vector &other) {
    Vector res;
    for (usz i = 0; i < D; ++i) {
      res.values[i] = values[i] - other.values[i];
    }

    return res;
  }

  void operator-=(const Vector &other) {
    for (usz i = 0; i < D; ++i) {
      values[i] -= other.values[i];
    }
  }

  T operator*(Vector &other) {
    return dot(other);
  }

  Vector operator%(Vector &other) {
    return cross(other);
  }

  void operator%=(Vector<T, 3> &other) {
    *this = cross(other);
  }

  Vector operator*(T scalar) {
    Vector res;
    for (usz i = 0; i < D; ++i) {
      res.values[i] = values[i] * scalar;
    }

    return res;
  }

  void operator*=(T scalar) {
    for (usz i = 0; i < D; ++i) {
      values[i] *= scalar;
    }
  }

  T& operator[](usz i) {
    Assert(i < D);
    return values[i];
  }

  bool operator==(const Vector &other) {
    for (usz i = 0; i < D; ++i) {
      if (values[i] != other.values[i]) {
	return false;
      }
    }

    return true;
  }

  bool operator!=(const Vector &other) {
    return !(*this == other);
  }
};

#define Vec2D(TYPE) Vector<TYPE, 2>
#define Vec3D(TYPE) Vector<TYPE, 3>

#endif
