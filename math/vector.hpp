#ifndef BASE_VECTOR
#define BASE_VECTOR

#include <functional>

// =============================================================================
// Vector definitions
template <typename T, usize D>
struct Vector {
  union { T values[D]; struct { T x, y, z, w; }; };
  T& operator[](usize i) const {
    Assert(i < D);
    return (T&)values[i];
  }
};

template <typename T>
struct Vector<T, 1> {
  union { T values[1]; struct { T x; }; };
  T& operator[](usize i) const {
    Assert(i < 1);
    return (T&)values[i];
  }
};

template <typename T>
struct Vector<T, 2> {
  union { T values[2]; struct { T x, y; }; };
  T& operator[](usize i) const {
    Assert(i < 2);
    return (T&)values[i];
  }
};

template <typename T>
struct Vector<T, 3> {
  union { T values[3]; struct { T x, y, z; }; };
  T& operator[](usize i) const {
    Assert(i < 3);
    return (T&)values[i];
  }
};

template <typename T>
struct Vector<T, 4> {
  union { T values[4]; struct { T x, y, z, w; }; };
  T& operator[](usize i) const {
    Assert(i < 4);
    return (T&)values[i];
  }
};

// =============================================================================
// Vec3 functions
template <typename T>
fn Vector<T, 3> vec3_cross(Vector<T, 3> *lhs, Vector<T, 3> *rhs);

// =============================================================================
// Generic vector functions
template <typename T, usize D>
fn Vector<T, D> vec_from_list(T(&args)[D]);
template <typename T, usize D>
fn Vector<T, D> vec_from_fn(std::function<T(usize)> init_fn);

template <typename T, usize D>
fn bool vec_isZero(Vector<T, D> *vec);

template <typename T, usize D>
fn bool vec_isOrthogonal(Vector<T, D> *lhs, Vector<T, D> *rhs);

template <typename T, usize D>
fn bool vec_isOrthonormal(Vector<T, D> *lhs, Vector<T, D> *rhs);

template <typename T, usize D>
fn T vec_dot(const Vector<T, D> *lhs, const Vector<T, D> *rhs);

template <typename T, usize D>
fn Vector<T, D> vec_normalize(Vector<T, D> *vec);

template <typename T, usize D>
fn Vector<T, D> vec_proj(Vector<T, D> *lhs, Vector<T, D> *rhs);

template <typename T, usize D>
fn Vector<T, D> vec_elemwise_mul(Vector<T, D> *lhs, Vector<T, D> *rhs);
template <typename T, usize D>
fn Vector<T, D> vec_elemwise_div(Vector<T, D> *lhs, Vector<T, D> *rhs);

template <typename T, usize D>
fn f32 vec_magnitude(Vector<T, D> *vec);
template <typename T, usize D>
fn f64 vec_magnitude64(Vector<T, D> *vec);

// =============================================================================
// Generic vector operators
template <typename T, usize D>
fn Vector<T, D> operator+(const Vector<T, D> &lhs, const Vector<T, D> &rhs);
template <typename T, usize D>
fn void operator+=(Vector<T, D> &lhs, const Vector<T, D> &rhs);

template <typename T, usize D>
fn Vector<T, D> operator-(const Vector<T, D> &vec);
template <typename T, usize D>
fn Vector<T, D> operator-(const Vector<T, D> &lhs, const Vector<T, D> &rhs);
template <typename T, usize D>
fn void operator-=(Vector<T, D> &lhs, const Vector<T, D> &rhs);

template <typename T, usize D>
fn Vector<T, D> operator*(const Vector<T, D> &lhs, T rhs);
template <typename T, usize D>
fn void operator*=(Vector<T, D> &lhs, T rhs);

template <typename T, usize D>
fn Vector<T, D> operator/(const Vector<T, D> &lhs, T rhs);
template <typename T, usize D>
fn void operator/=(Vector<T, D> &lhs, T rhs);

template <typename T, usize D>
fn bool operator==(const Vector<T, D> &lhs, const Vector<T, D> &rhs);
template <typename T, usize D>
fn bool operator!=(const Vector<T, D> &lhs, const Vector<T, D> &rhs);

#endif
