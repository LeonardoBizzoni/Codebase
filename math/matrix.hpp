#ifndef BASE_MATRIX
#define BASE_MATRIX

template <typename T, usize R, usize C>
struct Matrix {
  T values[R][C];
  T& operator[](usize r, usize c) const {
    Assert(r < R && c < C);
    return (T&)values[r][c];
  }

  Vector<T, R> operator[](usize c) const {
    Assert(c < C);
    return mat_col((Matrix<T, R, C>*)this, c);
  }
};


template <typename T, usize R, usize C>
fn Matrix<T, R, C> mat_identity();
template <typename T, usize R, usize C, typename... Ts>
fn Matrix<T, R, C> mat_init(Ts... args);

template <typename T, usize R, usize C>
fn Matrix<T, R, C> mat_elementWiseProd(Matrix<T, R, C> *lhs, Matrix<T, R, C> *rhs);
template <typename T, usize R, usize C>
fn Matrix<T, R, C> mat_elementWiseDiv(Matrix<T, R, C> *lhs, Matrix<T, R, C> *rhs);

template <typename T, usize R, usize C>
Matrix<T, C, R> mat_transpose(Matrix<T, R, C> *mat);

template <typename T, usize R, usize C>
Matrix<T, R-1, C-1> mat_minor(Matrix<T, R, C> *mat, usize r, usize c);
template <typename T, usize R, usize C, usize R1, usize C1>
Matrix<T, R1, C1> mat_shrink(Matrix<T, R, C> *mat);


Matrix<f32, 3, 3> mat_rot_x(f32 angle);
Matrix<f32, 3, 3> mat_rot_y(f32 angle);
Matrix<f32, 3, 3> mat_rot_z(f32 angle);

Matrix<f32, 3, 3> mat_rot_euler_zyz(f32 phi, f32 theta, f32 psi);
Matrix<f32, 3, 3> mat_rot_euler_rpy(f32 phi, f32 theta, f32 psi);

template <typename T, usize R, usize C>
u32 mat_rank(Matrix<T, R, C> *mat);
template <typename T, usize R>
f32 mat_det(Matrix<T, R, R> *mat);
template <typename T, usize R>
Matrix<T, R, R> mat_inverse(Matrix<T, R, R> *mat);

template <typename T, usize R>
T mat_trace(Matrix<T, R, R> *mat);

template <typename T, usize R, usize C>
Vector<T, C> mat_row(Matrix<T, R, C> *mat, usize i);
template <typename T, usize R, usize C>
Vector<T, R> mat_col(Matrix<T, R, C> *mat, usize i);

template <typename T, usize R>
Vector<T, R> mat_diagonal_main(Matrix<T, R, R> *mat);
template <typename T, usize R>
Vector<T, R> mat_diagonal_anti(Matrix<T, R, R> *mat);

template <typename T, usize R, usize C>
Matrix<T, R, C> mat_row_swap(Matrix<T, R, C> *mat, usize r1, usize r2);
template <typename T, usize R, usize C>
Matrix<T, R, C> mat_row_add(Matrix<T, R, C> *mat, usize target, usize r);
template <typename T, usize R, usize C>
Matrix<T, R, C> mat_row_mult(Matrix<T, R, C> *mat, usize target, T v);

template <typename T, usize R, usize C>
Matrix<T, R, C> mat_col_swap(Matrix<T, R, C> *mat, usize r1, usize r2);
template <typename T, usize R, usize C>
Matrix<T, R, C> mat_col_add(Matrix<T, R, C> *mat, usize target, usize c);
template <typename T, usize R, usize C>
Matrix<T, R, C> mat_col_mult(Matrix<T, R, C> *mat, usize target, T v);


template <typename T, usize R, usize C>
Matrix<T, R, C> operator+(const Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs);
template <typename T, usize R, usize C>
void operator+=(Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs);

template <typename T, usize R, usize C>
Matrix<T, R, C> operator-(const Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs);
template <typename T, usize R, usize C>
void operator-=(Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs);

template <typename T, usize R, usize C>
Matrix<T, R, C> operator*(const Matrix<T, R, C> &lhs, T scalar);
template <typename T, usize R, usize C>
Matrix<T, R, C> operator*=(const Matrix<T, R, C> &lhs, T scalar);
template <typename T, usize M, usize N, usize P>
Matrix<T, M, P> operator*(const Matrix<T, M, N> &lhs, const Matrix<T, N, P> &rhs);
template <typename T, usize M, usize N, usize P>
void operator*=(Matrix<T, M, N> &lhs, const Matrix<T, N, P> &rhs);

template <typename T, usize R, usize C>
bool operator==(const Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs);
template <typename T, usize R, usize C>
bool operator!=(const Matrix<T, R, C> &lhs, const Matrix<T, R, C> &rhs);

#endif
