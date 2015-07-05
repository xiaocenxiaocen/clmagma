/*
   -- clMAGMA (version 1.1.0-beta2) --
   Univ. of Tennessee, Knoxville
   Univ. of California, Berkeley
   Univ. of Colorado, Denver
   @date November 2013

   @precisions normal z -> s d c

 */

#include <stdio.h>
#include "common_magma.h"


#define dA(i, j) dA, (dA_offset + (j)*ldda + (i))

extern "C" magma_int_t
magma_zlauum_gpu(magma_uplo_t uplo, magma_int_t n,
        magmaDoubleComplex_ptr dA, size_t dA_offset, magma_int_t ldda, magma_int_t *info,
        magma_queue_t queue)
{
/*  -- MAGMA (version 1.1.0-beta2) --
    Univ. of Tennessee, Knoxville
    Univ. of California, Berkeley
    Univ. of Colorado, Denver
    @date November 2013

    Purpose
    =======

    ZLAUUM computes the product U * U' or L' * L, where the triangular
    factor U or L is stored in the upper or lower triangular part of
    the array dA.

    If UPLO = 'U' or 'u' then the upper triangle of the result is stored,
    overwriting the factor U in dA.
    If UPLO = 'L' or 'l' then the lower triangle of the result is stored,
    overwriting the factor L in dA.
    This is the blocked form of the algorithm, calling Level 3 BLAS.

    Arguments
    =========

    UPLO    (input) CHARACTER*1
    Specifies whether the triangular factor stored in the array dA
    is upper or lower triangular:
    = 'U':  Upper triangular
    = 'L':  Lower triangular

    N       (input) INTEGER
    The order of the triangular factor U or L.  N >= 0.

    dA       (input/output) DOUBLE PRECISION array on the GPU, dimension (LDDA,N)
    On entry, the triangular factor U or L.
    On exit, if UPLO = 'U', the upper triangle of dA is
    overwritten with the upper triangle of the product U * U';
    if UPLO = 'L', the lower triangle of dA is overwritten with
    the lower triangle of the product L' * L.

    LDDA     (input) INTEGER
    The leading dimension of the array A.  LDDA >= max(1,N).

    INFO    (output) INTEGER
    = 0: successful exit
    < 0: if INFO = -k, the k-th argument had an illegal value

    ===================================================================== */

    /* Local variables */
    magma_uplo_t uplo_ = uplo;
    magma_int_t         nb, i, ib;
    double              d_one = MAGMA_D_ONE;
    magmaDoubleComplex     c_one = MAGMA_Z_ONE;
    magmaDoubleComplex     *work;

    int upper  = lapackf77_lsame(lapack_const(uplo_), lapack_const(MagmaUpper));

    *info = 0;

    if ((! upper) && (! lapackf77_lsame(lapack_const(uplo_), lapack_const(MagmaLower))))
        *info = -1;
    else if (n < 0)
        *info = -2;
    else if (ldda < max(1,n))
        *info = -4;

    if (*info != 0) {
        magma_xerbla( __func__, -(*info) );
        return *info;
    }

    nb = magma_get_zpotrf_nb(n);

    if (MAGMA_SUCCESS != magma_zmalloc_cpu( &work, nb*nb )) {
        *info = MAGMA_ERR_HOST_ALLOC;
        return *info;
    }

    if (nb <= 1 || nb >= n) {
        magma_zgetmatrix( n, n, dA, dA_offset, ldda, work, 0, n, queue );
        lapackf77_zlauum(lapack_const(uplo_), &n, work, &n, info);
        magma_zsetmatrix( n, n, work, 0, n, dA, dA_offset, ldda, queue );
    }
    else {
        if (upper)
        {
            /* Compute inverse of upper triangular matrix */
            for (i=0; i<n; i =i+ nb) {
                ib = min(nb, (n-i));

                /* Compute the product U * U'. */
                magma_ztrmm( MagmaRight, MagmaUpper,
                        MagmaConjTrans, MagmaNonUnit, i, ib,
                        c_one, dA(i,i), ldda, dA(0, i),ldda, queue );

                magma_zgetmatrix( ib, ib,
                        dA(i, i), ldda,
                        work, 0, ib, queue );

                lapackf77_zlauum(MagmaUpperStr, &ib, work, &ib, info);

                magma_zsetmatrix( ib, ib,
                        work, 0, ib,
                        dA(i, i), ldda, queue );

                if(i+ib < n) {
                    magma_zgemm( MagmaNoTrans, MagmaConjTrans,
                            i, ib, (n-i-ib), c_one, dA(0,i+ib),
                            ldda, dA(i, i+ib), ldda, c_one,
                            dA(0,i), ldda, queue);


                    magma_zherk( MagmaUpper, MagmaNoTrans, ib,(n-i-ib),
                            d_one, dA(i, i+ib), ldda,
                            d_one, dA(i, i),    ldda, queue);
                }
            }
        } else {
            /* Compute the product L' * L. */
            for(i=0; i<n; i=i+nb) {
                ib=min(nb,(n-i));

                magma_ztrmm( MagmaLeft, MagmaLower,
                        MagmaConjTrans, MagmaNonUnit, ib,
                        i, c_one, dA(i,i), ldda,
                        dA(i, 0),ldda, queue);

                magma_zgetmatrix( ib, ib,
                        dA(i, i), ldda,
                        work, 0, ib, queue );

                lapackf77_zlauum(MagmaLowerStr, &ib, work, &ib, info);

                magma_zsetmatrix( ib, ib,
                        work, 0, ib,
                        dA(i, i), ldda, queue );


                if((i+ib) < n) {
                    magma_zgemm( MagmaConjTrans, MagmaNoTrans,
                            ib, i, (n-i-ib), c_one, dA( i+ib,i),
                            ldda, dA(i+ib, 0),ldda, c_one,
                            dA(i,0), ldda, queue);
                    magma_zherk( MagmaLower, MagmaConjTrans, ib, (n-i-ib),
                            d_one, dA(i+ib, i), ldda,
                            d_one, dA(i, i),    ldda, queue);
                }
            }
        }
    }


    magma_free_cpu( work );

    return *info;
}
