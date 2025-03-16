fn f64 minMaxNewtonMethod(f64 (*func)(f64), f64 x0, f64 epsilon) {
  f64 xk;
  for (;; x0 = xk) {
    xk = x0 - (derive(func, x0) / secondDerivative(func, x0));
    if (Abs(xk - x0) <= epsilon) {
      return xk;
    }
  }
}

template <usize Constraints, usize Variables>
fn Vector<f32, Variables> simplex(Vector<f32, Variables> objective_terms,
                                  Matrix<f32, Constraints, Variables> constraints,
                                  Vector<f32, Constraints> known_terms) {
  Buffer<usize, Constraints> basis;
  Matrix<f32, Constraints+1, Constraints+1 + Variables+1> A = {};
  constexpr usize B = Constraints + 1 + Variables;

  // Copy `objective` into `A`
  for (usize i = 0; i < Constraints; ++i) {
    for (usize j = 0; j < Variables; ++j) {
      A[i+1, j+1] = constraints[i, j];
    }
  }

  // Copy `bounds` into `B` leaving space for the simplex solution Z
  for (usize i = 0; i < Constraints; ++i) {
    A[i+1, B] = known_terms[i];
  }

  // Setup the first row with `1` follow by `-objective`
  A[0, 0] = 1;
  for (usize j = 0; j < Variables; ++j) {
    A[0, j+1] = -objective_terms[j];
  }

  // Setup the identity for the slack variables
  for (usize i = 1; i < Constraints+1; ++i) {
    for (usize j = Variables+1; j < Constraints+1 + Variables; ++j) {
      if (j - Variables == i) {
        A[i, j] = 1;
      }
    }
  }

  // Set the basis variables
  for (usize i = 0; i < Constraints; ++i) {
    basis[i] = i + 1 + Variables;
  }

  // =========================================================================
  // The actual simplex algorithm
  while (1) {
    // Finding pivot element
    usize pivot_column = 0, pivot_row = 0;
    for (usize j = 1; j < Constraints + 1 + Variables; ++j) {
      if (A[0, j] < A[0, pivot_column] && A[0, j] < 0) {
        pivot_column = j;
      }
    }

    if (pivot_column == 0) { break; }

    for (usize i = 1; i < Constraints+1; ++i) {
      if (!pivot_row ||
          (A[i, pivot_column] > 0 &&
           A[i, B]/A[i, pivot_column] < A[pivot_row, B]/A[pivot_row, pivot_column])) {
        pivot_row = i;
      }
    }

    if (pivot_row == 0) { break; }
    f32 pivot = A[pivot_row, pivot_column];
    basis[pivot_row - 1] = pivot_column;

    for (usize j = 0; j <= B; ++j) {
      A[pivot_row, j] /= pivot;
    }

    for (usize i = 0; i < Constraints+1; ++i) {
      if (i == pivot_row) { continue; }

      f32 update_val = -A[i, pivot_column];
      for (usize j = 0; j < Constraints+1 + Variables+1; ++j) {
        A[i, j] += A[pivot_row, j] * update_val;
      }
    }
  }

  Vector<f32, Variables> res = {};
  for (usize i = 0; i < Constraints; ++i) {
    if (basis[i] <= Variables) {
      res[basis[i] - 1] = A[i+1, B];
    }
  }

  return res;
}
