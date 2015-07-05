/*
 *   -- clMAGMA (version 1.1.0-beta2) --
 *      Univ. of Tennessee, Knoxville
 *      Univ. of California, Berkeley
 *      Univ. of Colorado, Denver
 *      @date November 2013
 *
 * @generated d Mon Nov 25 17:56:04 2013
 */

/*
   Matrix is divided into 64 x n block rows.
   Each block has 64 threads.
   Each thread copies one row, iterating across all columns.
   The bottom block of rows may be partially outside the matrix;
   if so, rows outside the matrix (row >= m) are disabled.

   @author Mark Gates
 */

#define PRECISION_d
#if defined(PRECISION_c) || defined(PRECISION_z)
typedef double double;
#endif

__kernel void dlacpy_kernel(
    int m, int n,
    __global double *A, int offset_A, int lda,
    __global double *B, int offset_B, int ldb)
{
    int row = get_group_id(0)*64 + get_local_id(0);
    if(row < m){
        A += (offset_A + row);
        B += (offset_B + row);
        __global double *Aend = A + lda*n;
        while(A < Aend){
            *B = *A;
            A += lda;
            B += ldb;
        }
    }
}
