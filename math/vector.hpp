#ifndef BASE_VECTOR
#define BASE_VECTOR

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

template <typename T>
Vector<T, 3> vec3_cross(Vector<T, 3> *lhs, Vector<T, 3> *rhs);

template <typename T, usize D>
bool vec_isZero(Vector<T, D> *vec);

template <typename T, usize D>
bool vec_isOrthogonal(Vector<T, D> *lhs, Vector<T, D> *rhs);

template <typename T, usize D>
bool vec_isOrthonormal(Vector<T, D> *lhs, Vector<T, D> *rhs);

template <typename T, usize D>
T vec_dot(const Vector<T, D> *lhs, const Vector<T, D> *rhs);

template <typename T, usize D>
Vector<T, D> vec_normalize(Vector<T, D> *vec);

template <typename T, usize D>
Vector<T, D> vec_proj(Vector<T, D> *lhs, Vector<T, D> *rhs);

template <typename T, usize D>
Vector<T, D> vec_elementWiseProduct(Vector<T, D> *lhs, Vector<T, D> *rhs);
template <typename T, usize D>
Vector<T, D> vec_elementWiseDiv(Vector<T, D> *lhs, Vector<T, D> *rhs);

template <typename T, usize D>
f32 vec_magnitude(Vector<T, D> *vec);
template <typename T, usize D>
f64 vec_magnitude64(Vector<T, D> *vec);

template <typename T, usize D>
Vector<T, D> operator+(const Vector<T, D> &lhs, const Vector<T, D> &rhs);
template <typename T, usize D>
void operator+=(Vector<T, D> &lhs, const Vector<T, D> &rhs);

template <typename T, usize D>
Vector<T, D> operator-(const Vector<T, D> &vec);
template <typename T, usize D>
Vector<T, D> operator-(const Vector<T, D> &lhs, const Vector<T, D> &rhs);
template <typename T, usize D>
void operator-=(Vector<T, D> &lhs, const Vector<T, D> &rhs);

template <typename T, usize D>
Vector<T, D> operator*(const Vector<T, D> &lhs, T rhs);
template <typename T, usize D>
void operator*=(Vector<T, D> &lhs, T rhs);

template <typename T, usize D>
Vector<T, D> operator/(const Vector<T, D> &lhs, T rhs);
template <typename T, usize D>
void operator/=(Vector<T, D> &lhs, T rhs);

template <typename T, usize D>
bool operator==(const Vector<T, D> &lhs, const Vector<T, D> &rhs);
template <typename T, usize D>
bool operator!=(const Vector<T, D> &lhs, const Vector<T, D> &rhs);

#endif
