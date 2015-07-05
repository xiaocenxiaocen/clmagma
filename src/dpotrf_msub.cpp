/*
    -- MAGMA (version 1.1.0-beta2) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date November 2013

       @generated d Mon Nov 25 17:56:00 2013

*/

#include <stdio.h>
#include "common_magma.h"

extern "C" magma_err_t
magma_dpotrf_msub(int num_subs, int num_gpus, magma_uplo_t uplo, magma_int_t n, 
                  magmaDouble_ptr *d_lA, size_t dA_offset, 
                  magma_int_t ldda, magma_int_t *info, 
                  magma_queue_t *queues)
{
/*  -- clMAGMA (version 1.1.0-beta2) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date November 2013

    Purpose   
    =======   
    DPOTRF computes the Cholesky factorization of a real symmetric   
    positive definite matrix dA.   

    The factorization has the form   
       dA = U**T * U,  if UPLO = 'U', or   
       dA = L  * L**T,  if UPLO = 'L',   
    where U is an upper triangular matrix and L is lower triangular.   

    This is the block version of the algorithm, calling Level 3 BLAS.   

    Arguments   
    =========   
    UPLO    (input) CHARACTER*1   
            = 'U':  Upper triangle of dA is stored;   
            = 'L':  Lower triangle of dA is stored.   

    N       (input) INTEGER   
            The order of the matrix dA.  N >= 0.   

    dA      (input/output) DOUBLE_PRECISION array on the GPU, dimension (LDDA,N)   
            On entry, the symmetric matrix dA.  If UPLO = 'U', the leading   
            N-by-N upper triangular part of dA contains the upper   
            triangular part of the matrix dA, and the strictly lower   
            triangular part of dA is not referenced.  If UPLO = 'L', the   
            leading N-by-N lower triangular part of dA contains the lower   
            triangular part of the matrix dA, and the strictly upper   
            triangular part of dA is not referenced.   

            On exit, if INFO = 0, the factor U or L from the Cholesky   
            factorization dA = U**T * U or dA = L * L**T.   

    LDDA     (input) INTEGER   
            The leading dimension of the array dA.  LDDA >= max(1,N).
            To benefit from coalescent memory accesses LDDA must be
            dividable by 16.

    INFO    (output) INTEGER   
            = 0:  successful exit   
            < 0:  if INFO = -i, the i-th argument had an illegal value   
            > 0:  if INFO = i, the leading minor of order i is not   
                  positive definite, and the factorization could not be   
                  completed.   
    =====================================================================   */


    int tot_subs = num_subs * num_gpus;
    magma_err_t err;
    magma_int_t j, nb, d, lddp, h;
    double *work;
    magmaDouble_ptr dwork[MagmaMaxGPUs];

    *info = 0;
    nb = magma_get_dpotrf_nb(n);
    if ( uplo != MagmaUpper && uplo != MagmaLower ) {
        *info = -1;
    } else if (n < 0) {
        *info = -2;
    } else if (uplo != MagmaUpper) {
        lddp = nb*(n/(nb*tot_subs));
        if( n%(nb*tot_subs) != 0 ) lddp+=min(nb,n-tot_subs*lddp);
        if( ldda < lddp ) *info = -4;
    } else if( ldda < n ) {
        *info = -4;
    }
    if (*info != 0) {
        magma_xerbla( __func__, -(*info) );
        return *info;
    }

    if (num_gpus == 1 && ((nb <= 1) || (nb >= n)) ) {
        /*  Use unblocked code. */
        err = magma_dmalloc_cpu( &work, n*nb );
        if (err != MAGMA_SUCCESS) {
            *info = MAGMA_ERR_HOST_ALLOC;
            return *info;
        }
        magma_dgetmatrix( n, n, d_lA[0], 0, ldda, work, 0, n, queues[0] );
        lapackf77_dpotrf(lapack_uplo_const(uplo), &n, work, &n, info);
        magma_dsetmatrix( n, n, work, 0, n, d_lA[0], 0, ldda, queues[0] );
        magma_free_cpu( work );
    } else {
        lddp = 32*((n+31)/32);
        for( d=0; d<num_gpus; d++ ) {
            if (MAGMA_SUCCESS != magma_dmalloc( &dwork[d], num_gpus*nb*lddp )) {
                for( j=0; j<d; j++ ) magma_free( dwork[j] );
                *info = MAGMA_ERR_DEVICE_ALLOC;
                return *info;
            }
        }
        h = 1; //num_gpus; //(n+nb-1)/nb;
        if (MAGMA_SUCCESS != magma_dmalloc_cpu( &work, n*nb*h )) {
            *info = MAGMA_ERR_HOST_ALLOC;
            return *info;
        }
        if (uplo == MagmaUpper) {
            /* with two queues for each device */
            magma_dpotrf2_msub(num_subs, num_gpus, uplo, n, n, 0, 0, nb, d_lA, 0, ldda, 
                               dwork, lddp, work, n, h, info, queues);
            /* with three streams */
            //magma_dpotrf3_msub(num_gpus, uplo, n, n, 0, 0, nb, d_lA, ldda, dwork, lddp, work, n,  
            //                   h, stream, event, info);
        } else {
            /* with two queues for each device */
            magma_dpotrf2_msub(num_subs, num_gpus, uplo, n, n, 0, 0, nb, d_lA, 0, ldda, 
                               dwork, lddp, work, nb*h, h, info, queues);
            /* with three streams */
            //magma_dpotrf3_msub(num_gpus, uplo, n, n, 0, 0, nb, d_lA, ldda, dwork, lddp, work, nb*h, 
            //                   h, stream, event, info);
        }

        /* clean up */
        for( d=0; d<num_gpus; d++ ) magma_free( dwork[d] );
        magma_free_cpu( work );
    } /* end of not lapack */

    return *info;
} /* magma_dpotrf_msub */
