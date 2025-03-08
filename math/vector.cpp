template <typename T>
Vector<T, 3> vec3_cross(Vector<T, 3> *lhs, Vector<T, 3> *rhs) {
  Vector<T, 3> res = {
    .values = {
      lhs->y * rhs->z - lhs->z * rhs->y,
      lhs->z * rhs->x - lhs->x * rhs->z,
      lhs->x * rhs->y - lhs->y * rhs->x,
    },
  };
  return res;
}

template <typename T, usize D>
bool vec_isZero(Vector<T, D> *vec) {
  for (usize i = 0; i < D; ++i) {
    if (vec->values[i]) { return false; }
  }
  return true;
}

template <typename T, usize D>
bool vec_isOrthogonal(Vector<T, D> *lhs, Vector<T, D> *rhs) {
  return Approx(vec_dot(lhs, rhs), 0);
}

template <typename T, usize D>
bool vec_isOrthonormal(Vector<T, D> *lhs, Vector<T, D> *rhs) {
  return vec_isOrthogonal(lhs, rhs) && Approx(vec_magnitude(lhs), 1);
}

template <typename T, usize D>
T vec_dot(Vector<T, D> *lhs, Vector<T, D> *rhs) {
  T res = 0;
  for (usize i = 0; i < D; ++i) {
    res += lhs->values[i] * rhs->values[i];
  }
  return res;
}

// the `vector / ||vector||` thing
template <typename T, usize D>
Vector<T, D> vec_normalize(Vector<T, D> *vec) {
  Vector<T, D> res = *vec;
  f32 length = vec_magnitude(vec);
  if (!length) { return res; }
  for (usize i = 0; i < D; ++i) { res.values[i] /= length; }
  return res;
}

template <typename T, usize D>
Vector<T, D> vec_proj(Vector<T, D> *lhs, Vector<T, D> *rhs) {
  return (*lhs) * (vec_dot(rhs, lhs) / vec_dot(lhs, lhs));
}

template <typename T, usize D>
Vector<T, D> vec_elementWiseProd(Vector<T, D> *lhs, Vector<T, D> *rhs) {
  Vector res = *lhs;
  for (usize i = 0; i < D; ++i) {
    res *= rhs->values[i];
  }
  return res;
}

template <typename T, usize D>
Vector<T, D> vec_elementWiseDiv(Vector<T, D> *lhs, Vector<T, D> *rhs) {
  Vector res = *lhs;
  for (usize i = 0; i < D; ++i) {
    res /= rhs->values[i];
  }
  return res;
}

// the `||vector||` thing
template <typename T, usize D>
f32 vec_magnitude(Vector<T, D> *vec) {
  f32 res = 0.f;
  for (usize i = 0; i < D; ++i) {
    res += Abs(vec->values[i]) * Abs(vec->values[i]);
  }
  return sqrtf(res);
}

template <typename T, usize D>
f64 vec_magnitude64(Vector<T, D> *vec) {
  f64 res = 0.f;
  for (usize i = 0; i < D; ++i) {
    res += Abs(vec->values[i]) * Abs(vec->values[i]);
  }
  return sqrt(res);
}

template <typename T, usize D>
Vector<T, D> operator+(const Vector<T, D> &lhs, const Vector<T, D> &rhs) {
  Vector<T, D> res;
  for (usize i = 0; i < D; ++i) {
    res.values[i] = lhs.values[i] + rhs.values[i];
  }
  return res;
}

template <typename T, usize D>
void operator+=(Vector<T, D> &lhs, const Vector<T, D> &rhs) {
  for (usize i = 0; i < D; ++i) {
    lhs.values[i] += rhs.values[i];
  }
}

template <typename T, usize D>
Vector<T, D> operator-(const Vector<T, D> &vec) {
  Vector<T, D> res;
  for (usize i = 0; i < D; ++i) {
    res.values[i] = -vec.values[i];
  }
  return res;
}

template <typename T, usize D>
Vector<T, D> operator-(const Vector<T, D> &lhs, const Vector<T, D> &rhs) {
  Vector<T, D> res = lhs;
  for (usize i = 0; i < D; ++i) {
    res.values[i] -= rhs.values[i];
  }
  return res;
}

template <typename T, usize D>
void operator-=(Vector<T, D> &lhs, const Vector<T, D> &rhs) {
  for (usize i = 0; i < D; ++i) {
    lhs.values[i] -= rhs.values[i];
  }
}

template <typename T, usize D>
Vector<T, D> operator*(const Vector<T, D> &lhs, T rhs) {
  Vector<T, D> res = lhs;
  for (usize i = 0; i < D; ++i) {
    res.values[i] *= rhs;
  }
  return res;
}

template <typename T, usize D>
void operator*=(Vector<T, D> &lhs, T rhs) {
  for (usize i = 0; i < D; ++i) {
    lhs.values[i] *= rhs;
  }
}

template <typename T, usize D>
Vector<T, D> operator/(const Vector<T, D> &lhs, T rhs) {
  Vector<T, D> res = lhs;
  for (usize i = 0; i < D; ++i) {
    res.values[i] /= rhs;
  }
  return res;
}

template <typename T, usize D>
void operator/=(Vector<T, D> &lhs, T rhs) {
  for (usize i = 0; i < D; ++i) {
    lhs.values[i] /= rhs;
  }
}

template <typename T, usize D>
bool operator==(const Vector<T, D> &lhs, const Vector<T, D> &rhs) {
  for (usize i = 0; i < D; ++i) {
    if (lhs[i] != rhs.values[i]) {
      return false;
    }
  }

  return true;
}

template <typename T, usize D>
bool operator!=(const Vector<T, D> &lhs, const Vector<T, D> &rhs) {
  return !(lhs == rhs);
}
