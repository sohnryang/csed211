/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include <stdio.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  int i, j, bi, bj, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  if (M == 32)
    for (i = 0; i < N; i += 8) {
      for (j = 0; j < M; j += 8) {
        if (i == j) {
          for (bi = 0; bi < 8; bi++)
            for (bj = bi + 1; bj < 8; bj++)
              B[j + bj][i + bi] = A[i + bi][j + bj];
          for (bj = 0; bj < 8; bj++)
            for (bi = bj; bi < 8; bi++)
              B[j + bj][i + bi] = A[i + bi][j + bj];
        } else
          for (bi = 0; bi < 8; bi++)
            for (bj = 0; bj < 8; bj++)
              B[j + bj][i + bi] = A[i + bi][j + bj];
      }
    }
  else if (M == 64)
    for (i = 0; i < N; i += 8) {
      for (j = 0; j < M; j += 8) {
        for (bi = 0; bi < 4; bi++) {
          tmp0 = A[i + bi][j + 0];
          tmp1 = A[i + bi][j + 1];
          tmp2 = A[i + bi][j + 2];
          tmp3 = A[i + bi][j + 3];
          tmp4 = A[i + bi][j + 4];
          tmp5 = A[i + bi][j + 5];
          tmp6 = A[i + bi][j + 6];
          tmp7 = A[i + bi][j + 7];
          B[j + 0][i + bi] = tmp0;
          B[j + 1][i + bi] = tmp1;
          B[j + 2][i + bi] = tmp2;
          B[j + 3][i + bi] = tmp3;
          B[j + 0][i + bi + 4] = tmp4;
          B[j + 1][i + bi + 4] = tmp5;
          B[j + 2][i + bi + 4] = tmp6;
          B[j + 3][i + bi + 4] = tmp7;
        }
        for (bi = 0; bi < 4; bi++) {
          tmp0 = A[i + 4][j + bi];
          tmp1 = A[i + 5][j + bi];
          tmp2 = A[i + 6][j + bi];
          tmp3 = A[i + 7][j + bi];
          tmp4 = A[i + 4][j + bi + 4];
          tmp5 = A[i + 5][j + bi + 4];
          tmp6 = A[i + 6][j + bi + 4];
          tmp7 = A[i + 7][j + bi + 4];

          bj = B[j + bi][i + 4];
          B[j + bi][i + 4] = tmp0;
          tmp0 = bj;
          bj = B[j + bi][i + 5];
          B[j + bi][i + 5] = tmp1;
          tmp1 = bj;
          bj = B[j + bi][i + 6];
          B[j + bi][i + 6] = tmp2;
          tmp2 = bj;
          bj = B[j + bi][i + 7];
          B[j + bi][i + 7] = tmp3;
          tmp3 = bj;

          B[j + bi + 4][i + 0] = tmp0;
          B[j + bi + 4][i + 1] = tmp1;
          B[j + bi + 4][i + 2] = tmp2;
          B[j + bi + 4][i + 3] = tmp3;
          B[j + bi + 4][i + 4] = tmp4;
          B[j + bi + 4][i + 5] = tmp5;
          B[j + bi + 4][i + 6] = tmp6;
          B[j + bi + 4][i + 7] = tmp7;
        }
      }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
  int i, j, tmp;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; j++) {
      tmp = A[i][j];
      B[j][i] = tmp;
    }
  }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
  /* Register your solution function */
  registerTransFunction(transpose_submit, transpose_submit_desc);

  /* Register any additional transpose functions */
  registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
  int i, j;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; ++j) {
      if (A[i][j] != B[j][i]) {
        return 0;
      }
    }
  }
  return 1;
}
