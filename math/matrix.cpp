template <typename T, usize R, usize C, typename... Ts>
fn Matrix<T, R, C> mat_init(Ts... args) {
  Matrix<T, R, C> res = {
    .values = { (T)(args)... },
  };
  return res;
}

template <typename T, usize R, usize C>
fn Matrix<T, R, C> mat_identity() {
  Matrix<T, R, C> res = {0};
  for (usize r = 0; r < R; ++r) {
    res[r, r] = 1;
  }
  return res;
}

template <typename T, usize R, usize C>
fn Matrix<T, R, C> mat_elementWiseProd(Matrix<T, R, C> *lhs, Matrix<T, R, C> *rhs) {
  Matrix<T, R, C> res = *lhs;
  for (usize r = 0; r < R; ++r) {
    for (usize c = 0; c < C; ++c) {
      res[r, c] *= (*rhs)[r, c];
    }
  }
  return res;
}

template <typename T, usize R, usize C>
fn Matrix<T, R, C> mat_elementWiseDiv(Matrix<T, R, C> *lhs, Matrix<T, R, C> *rhs) {
  Matrix<T, R, C> res = *lhs;
  for (usize r = 0; r < R; ++r) {
    for (usize c = 0; c < C; ++c) {
      res[r, c] /= (*rhs)[r, c];
    }
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, C, R> mat_transpose(Matrix<T, R, C> *mat) {
  Matrix<T, C, R> res = {0};
  for (usize i = 0; i < R; ++i) {
    for (usize j = 0; j < C; ++j) {
      res[j, i] = (*mat)[i, j];
    }
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, R-1, C-1> mat_minor(Matrix<T, R, C> *mat, usize r, usize c) {
  Assert(r < R && c < C);
  Matrix<T, R-1, C-1> res = {0};
  for (usize i = 0, a = 0; a < R; ++a) {
    if (a == r) { continue; }
    for (usize j = 0, b = 0; b < C; ++b) {
      if (b == c) { continue; }
      res[i, j] = (*mat)[a, b];
      ++j;
    }
    ++i;
  }
  return res;
}

template <typename T, usize R, usize C, usize R1, usize C1>
Matrix<T, R1, C1> mat_shrink(Matrix<T, R, C> *mat) {
  if constexpr (R1 == R && C1 == C) {
    return *mat;
  } else {
    Assert(R1 <= R && C1 <= C);
    Matrix<T, R1, C1> res = {0};
    for (usize i = 0; i < R1; ++i) {
      for (usize j = 0; j < C1; ++j) {
	res[i, j] = (*mat)[i, j];
      }
    }
    return res;
  }
}

template <typename T, usize R>
i32 mat_det(Matrix<T, R, R> *mat) {
  if constexpr (R == 1) {
    return (*mat)[0, 0];
  } else if constexpr (R == 2) {
    return ((*mat)[0, 0] * (*mat)[1, 1]) -
	   ((*mat)[0, 1] * (*mat)[1, 0]);
  } else {
    i32 res = 0;
    for (usize i = 0; i < R; ++i) {
      res += pow(-1, i) * (*mat)[0, i] * mat_det(mat_minor(mat, 0, i));
    }
    return res;
  }
}

template <typename T, usize R, usize C>
u32 mat_rank(Matrix<T, R, C> *mat) {
  constexpr local f64 eps (1E-9);
  Matrix<T, R, R> tmp = *mat;
  u32 rank = 0;
  u32 selected = 0;
  for (usize i = 0; i < C; ++i) {
    usize j = 0;
    for (; j < R; ++j) {
      if (!GetBit(selected, j) && Abs((tmp[j, i])) > eps) { break; }
    }
    if (j == R) { continue; }

    ++rank;
    SetBit(selected, j, 1);
    for (usize p = i + 1; p < C; ++p) { tmp[j, p] /= tmp[j, i]; }
    for (usize k = 0; k < R; ++k) {
      if (k != j && Abs((tmp[k, i])) > eps) {
	for (usize p = i + 1; p < C; ++p) {
	  tmp[k, p] -= tmp[j, p] * tmp[k, i];
	}
      }
    }
  }
  return rank;
}

template <typename T, usize R>
Matrix<T, R, R> mat_inverse(Matrix<T, R, R> *mat) {
  i32 det = mat_det(mat);
  Assert(det != 0);

  if constexpr (R == 2) {
    Matrix<T, 2, 2> res = {0};
    res[0, 0] = (*mat)[1, 1] / det;
    res[0, 1] = -(*mat)[0, 1] / det;
    res[1, 0] = -(*mat)[1, 0] / det;
    res[1, 1] = (*mat)[0, 0] / det;
    return res;
  } else {
    Matrix<T, R, R> cofactors = {0};
    for (usize r = 0; r < R; ++r) {
      for (usize c = 0; c < R; ++c) {
	Matrix<T, R-1, R-1> minor = mat_minor(mat, r, c);
	cofactors[c, r] = (pow(-1, r + c) * mat_det(&minor)) / det;
      }
    }
    return cofactors;
  }
}

template <typename T, usize R>
T mat_trace(Matrix<T, R, R> *mat) {
  T res = 0;
  for (usize i = 0; i < R; ++i) {
    res += (*mat)[i, i];
  }
  return res;
}

template <typename T, usize R, usize C>
Vector<T, C> mat_row(Matrix<T, R, C> *mat, usize i) {
  Assert(i < C);
  Vector<T, C> res = {0};
  for (usize j = 0; j < C; ++j) {
    res[j] = (*mat)[i, j];
  }
  return res;
}

template <typename T, usize R, usize C>
Vector<T, R> mat_col(Matrix<T, R, C> *mat, usize i) {
  Assert(i < R);
  Vector<T, R> res = {0};
  for (usize j = 0; j < R; ++j) {
    res[j] = (*mat)[j, i];
  }
  return res;
}

template <typename T, usize R>
Vector<T, R> mat_diagonal_main(Matrix<T, R, R> *mat) {
  Vector<T, R> res = {0};
  for (usize j = 0; j < R; ++j) {
    res[j] = (*mat)[j, j];
  }
  return res;
}

template <typename T, usize R>
Vector<T, R> mat_diagonal_anti(Matrix<T, R, R> *mat) {
  Vector<T, R> res = {0};
  for (usize j = 0; j < R; ++j) {
    res[j] = (*mat)[(R - 1) - j, j];
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, R, C> mat_row_swap(Matrix<T, R, C> *mat, usize r1, usize r2) {
  Assert(r1 < R && r2 < R);
  Matrix<T, R, C> res = {0};
  for (usize i = 0, old_i = 0, k = 0; i < R; ++i, ++k) {
    if (i == r1) {
      old_i = i;
      i = r2;
    } else if (i == r2) {
      old_i = i;
      i = r1;
    }
    for (usize j = 0; j < C; ++j) { res[i, j] = (*mat)[k, j]; }
    i = old_i++;
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, R, C> mat_row_add(Matrix<T, R, C> *mat, usize target, usize r) {
  Assert(target < R && r < R);
  Matrix<T, R, C> res = {0};
  for (usize i = 0; i < R; ++i) {
    for (usize j = 0; j < C; ++j) {
      res[i, j] = (*mat)[i, j] + ((i == target) ? (*mat)[r, j] : 0);
    }
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, R, C> mat_row_mult(Matrix<T, R, C> *mat, usize target, T v) {
  Assert(target < R && v != 0);
  Matrix<T, R, C> res = {0};
  for (usize i = 0; i < R; ++i) {
    for (usize j = 0; j < C; ++j) {
      res[i, j] = (*mat)[i, j] * (i == target ? v : 1);
    }
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, R, C> mat_col_swap(Matrix<T, R, C> *mat, usize r1, usize r2) {
  Assert(r1 < C && r2 < C);
  Matrix<T, R, C> res = {0};
  for (usize i = 0, old_j = 0; i < R; ++i) {
    for (usize j = 0, k = 0; j < C; ++j, ++k) {
      if (j == r1) {
	old_j = j;
	j = r2;
      } else if (j == r2) {
	old_j = j;
	j = r1;
      }
      res[i, j] = (*mat)[i, k];
      j = old_j++;
    }
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, R, C> mat_col_add(Matrix<T, R, C> *mat, usize target, usize c) {
  Assert(target < C && c < C);
  Matrix<T, R, C> res = {0};
  for (usize i = 0; i < R; ++i) {
    for (usize j = 0; j < C; ++j) {
      res[i, j] = (*mat)[i, j] + (j == target ? (*mat)[i, c] : 0);
    }
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, R, C> mat_col_mult(Matrix<T, R, C> *mat, usize target, T v) {
  Assert(target < C && v != 0);
  Matrix<T, R, C> res = {0};
  for (usize i = 0; i < R; ++i) {
    for (usize j = 0; j < C; ++j) {
      res[i, j] = (*mat)[i, j] * (j == target ? v : 1);
    }
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, R, C> operator+(const Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs) {
  Matrix<T, R, C> res = lhs;
  for (usize r = 0; r < R; ++r) {
    for (usize c = 0; c < C; ++c) { res[r, c] += rhs[r, c]; }
  }
  return res;
}

template <typename T, usize R, usize C>
void operator+=(Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs) {
  for (usize r = 0; r < R; ++r) {
    for (usize c = 0; c < C; ++c) { lhs[r, c] += rhs[r, c]; }
  }
}

template <typename T, usize R, usize C>
Matrix<T, R, C> operator-(const Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs) {
  Matrix<T, R, C> res = lhs;
  for (usize r = 0; r < R; ++r) {
    for (usize c = 0; c < C; ++c) { res[r, c] -= rhs[r, c]; }
  }
  return res;
}

template <typename T, usize R, usize C>
void operator-=(Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs) {
  for (usize r = 0; r < R; ++r) {
    for (usize c = 0; c < C; ++c) { lhs[r, c] -= rhs[r, c]; }
  }
}

template <typename T, usize R, usize C>
Matrix<T, R, C> operator*(const Matrix<T, R, C> &lhs, T scalar) {
  Matrix<T, R, C> res = lhs;
  for (usize r = 0; r < R; ++r) {
    for (usize c = 0; c < C; ++c) { res[r, c] *= scalar; }
  }
  return res;
}

template <typename T, usize R, usize C>
Matrix<T, R, C> operator*=(const Matrix<T, R, C> &lhs, T scalar) {
  for (usize r = 0; r < R; ++r) {
    for (usize c = 0; c < C; ++c) { lhs[r, c] *= scalar; }
  }
}

template <typename T, usize M, usize N, usize P>
Matrix<T, M, P> operator*(const Matrix<T, M, N> &lhs, const Matrix<T, N, P> &rhs) {
  Matrix<T, M, P> res = {0};
  for (usize i = 0; i < M; ++i) {
    for (usize j = 0; j < P; ++j) {
      for (usize r = 0; r < N; ++r) {
	res[i, j] += lhs[i, r] * rhs[r, j];
      }
    }
  }
  return res;
}

template <typename T, usize M, usize N, usize P>
void operator*=(Matrix<T, M, N> &lhs, const Matrix<T, N, P> &rhs) {
  lhs = lhs * rhs;
}

template <typename T, usize R, usize C>
bool operator==(const Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs) {
  for (usize i = 0; i < R; ++i) {
    for (usize j = 0; j < C; ++j) {
      if (lhs[i, j] != rhs[i, j]) { return false; }
    }
  }
  return true;
}

template <typename T, usize R, usize C>
bool operator!=(const Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs) {
  return !(lhs == rhs);
}
