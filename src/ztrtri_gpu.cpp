/*
    -- clMAGMA (version 1.3.0) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date November 2014

       @precisions normal z -> s d c

*/
#include "common_magma.h"

#define dA(i, j) dA, (dA_offset + (j)*ldda + (i))

extern "C" magma_int_t
magma_ztrtri_gpu(
    magma_uplo_t uplo, magma_diag_t diag, magma_int_t n,
    magmaDoubleComplex_ptr dA, size_t dA_offset, magma_int_t ldda,
    magma_queue_t queues[2],
    magma_int_t *info)
{
/*  -- clMAGMA (version 1.3.0) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date November 2014

    Purpose
    =======
    ZTRTRI computes the inverse of a real upper or lower triangular
    matrix dA.

    This is the Level 3 BLAS version of the algorithm.

    Arguments
    =========
    UPLO    (input) CHARACTER*1
            = 'U':  A is upper triangular;
            = 'L':  A is lower triangular.

    DIAG    (input) CHARACTER*1
            = 'N':  A is non-unit triangular;
            = 'U':  A is unit triangular.

    N       (input) INTEGER
            The order of the matrix A.  N >= 0.

    dA      (input/output) COMPLEX_16 array ON THE GPU, dimension (LDDA,N)
            On entry, the triangular matrix A.  If UPLO = 'U', the
            leading N-by-N upper triangular part of the array dA contains
            the upper triangular matrix, and the strictly lower
            triangular part of A is not referenced.  If UPLO = 'L', the
            leading N-by-N lower triangular part of the array dA contains
            the lower triangular matrix, and the strictly upper
            triangular part of A is not referenced.  If DIAG = 'U', the
            diagonal elements of A are also not referenced and are
            assumed to be 1.
            On exit, the (triangular) inverse of the original matrix, in
            the same storage format.

    LDDA    (input) INTEGER
            The leading dimension of the array dA.  LDDA >= max(1,N).

    INFO    (output) INTEGER
            = 0: successful exit
            < 0: if INFO = -i, the i-th argument had an illegal value
            > 0: if INFO = i, dA(i,i) is exactly zero.  The triangular
                    matrix is singular and its inverse cannot be computed.
                 (Singularity check is currently disabled.)

    ===================================================================== */

    /* Local variables */
    magma_int_t nb, nn, j, jb;
    //magmaDoubleComplex c_zero     = MAGMA_Z_ZERO;
    magmaDoubleComplex c_one      = MAGMA_Z_ONE;
    magmaDoubleComplex c_neg_one  = MAGMA_Z_NEG_ONE;
    magmaDoubleComplex *work;

    int upper  = (uplo == MagmaUpper);
    int nounit = (diag == MagmaNonUnit);

    *info = 0;

    if (! upper && uplo != MagmaLower)
        *info = -1;
    else if (! nounit && diag != MagmaUnit)
        *info = -2;
    else if (n < 0)
        *info = -3;
    else if (ldda < max(1,n))
        *info = -5;

    if (*info != 0) {
        magma_xerbla( __func__, -(*info) );
        return *info;
    }

    /* Check for singularity if non-unit */
    /* cannot do here with matrix dA on GPU -- need kernel */
    /*
    if (nounit) {
        for (j=0; j < n; ++j) {
            if ( MAGMA_Z_EQUAL( *dA(j,j), c_zero )) {
                *info = j+1;  // Fortran index
                return *info;
            }
        }
    }
    */

    /* Determine the block size for this environment */
    nb = magma_get_zpotrf_nb(n);

    /* Create Queues */
    //magma_queue_t  queues[2];
    //magma_device_t device[MagmaMaxGPUs];
    //magma_int_t num = 0;
    //magma_int_t err;
    //
    //err = magma_getdevices( device, MagmaMaxGPUs, &num );
    //if ( err != 0 || num < 1 ) {
    //    fprintf( stderr, "magma_getdevices failed: %d\n", err );
    //    exit(-1);
    //}
    //err = magma_queue_create( device[0], &queues[0] );
    //if ( err != 0 ) {
    //    fprintf( stderr, "magma_queue_create 0 failed: %d\n", err );
    //    exit(-1);
    //}
    //err = magma_queue_create( device[0], &queues[1] );
    //if ( err != 0 ) {
    //    fprintf( stderr, "magma_queue_create 1 failed: %d\n", err );
    //    exit(-1);
    //}

    if (MAGMA_SUCCESS != magma_zmalloc_cpu( &work, nb*nb )) {
        *info = MAGMA_ERR_HOST_ALLOC;
        return *info;
    }

    if (nb <= 1 || nb >= n) {
        magma_zgetmatrix( n, n, dA, dA_offset, ldda, work, n, queues[0] );
        lapackf77_ztrtri( lapack_const(uplo), lapack_const(diag), &n, work, &n, info );
        magma_zsetmatrix( n, n, work, n, dA, dA_offset, ldda, queues[0] );
    }
    else {
        if (upper) {
            /* Compute inverse of upper triangular matrix */
            for (j=0; j < n; j += nb){
                jb = min(nb, (n-j));

                /* Compute rows 1:j-1 of current block column */
                magma_ztrmm( MagmaLeft, MagmaUpper,
                             MagmaNoTrans, MagmaNonUnit, j, jb,
                             c_one, dA(0,0), ldda, dA(0, j), ldda,
                             queues[0] );

                magma_ztrsm( MagmaRight, MagmaUpper,
                             MagmaNoTrans, MagmaNonUnit, j, jb,
                             c_neg_one, dA(j,j), ldda, dA(0, j), ldda,
                             queues[0] );

                magma_zgetmatrix_async( jb, jb,
                                        dA(j, j), ldda,
                                        work, jb, queues[1], NULL );

                magma_queue_sync( queues[1] );

                /* Compute inverse of current diagonal block */
                lapackf77_ztrtri( MagmaUpperStr, lapack_const(diag), &jb, work, &jb, info );
                /*
                magma_zsetmatrix_async( jb, jb,
                                        work, 0, jb,
                                        dA(j, j), ldda, queues[0], NULL );
                */
                magma_zsetmatrix( jb, jb,
                                  work, jb,
                                  dA(j, j), ldda, queues[0] );
            }
        }
        else {
            /* Compute inverse of lower triangular matrix */
            nn = ((n-1)/nb)*nb+1;

            for(j=nn-1; j >= 0; j -= nb){
                jb = min(nb,(n-j));

                if((j+jb) < n){
                    /* Compute rows j+jb:n of current block column */
                    magma_ztrmm( MagmaLeft, MagmaLower,
                                 MagmaNoTrans, MagmaNonUnit, (n-j-jb), jb,
                                 c_one, dA(j+jb,j+jb), ldda, dA(j+jb, j), ldda,
                                 queues[0] );

                    magma_ztrsm( MagmaRight, MagmaLower,
                                 MagmaNoTrans, MagmaNonUnit, (n-j-jb), jb,
                                 c_neg_one, dA(j,j), ldda, dA(j+jb, j), ldda,
                                 queues[0] );
                }
                magma_zgetmatrix_async( jb, jb,
                                        dA(j, j), ldda,
                                        work, jb, queues[1], NULL );

                magma_queue_sync( queues[1] );

                /* Compute inverse of current diagonal block */
                lapackf77_ztrtri( MagmaLowerStr, lapack_const(diag), &jb, work, &jb, info );
                /*
                magma_zsetmatrix_async( jb, jb,
                        work, 0, jb,
                        dA(j, j), ldda, queues[0], NULL );
                */
                magma_zsetmatrix( jb, jb,
                                  work, jb,
                                  dA(j, j), ldda, queues[0] );
            }
        }
    }

    //magma_queue_destroy( queues[0] );
    //magma_queue_destroy( queues[1] );
    magma_free_cpu( work );

    return *info;
}
