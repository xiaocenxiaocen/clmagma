/*
 *   -- clMAGMA (version 1.3.0) --
 *      Univ. of Tennessee, Knoxville
 *      Univ. of California, Berkeley
 *      Univ. of Colorado, Denver
 *      @date November 2014
 *
 * @author Mark Gates
 * @generated from zpotrf2_gpu.cpp normal z -> s, Sat Nov 15 00:21:37 2014
 */

#include "common_magma.h"


// using 2 queues, 1 for comm, 1 for comp.
extern cl_context     gContext;

// produces pointer and offset as two args to magmaBLAS routines
#define dA(i,j)  dA, ( (dA_offset) + (i) + (j)*ldda )

// produces pointer as single arg to BLAS routines
#define A(i,j)  &A[ (i) + (j)*lda ]

/*  Purpose   
    =======   
    SPOTRF computes the Cholesky factorization of a real symmetric   
    positive definite matrix dA.   

    The factorization has the form   
       dA = U\*\*H * U,  if UPLO = 'U', or   
       dA = L  * L\*\*H,  if UPLO = 'L',   
    where U is an upper triangular matrix and L is lower triangular.   

    This is the block version of the algorithm, calling Level 3 BLAS.   

    Arguments   
    =========   
    UPLO    (input) INTEGER
            = MagmaUpper:  Upper triangle of dA is stored;   
            = MagmaLower:  Lower triangle of dA is stored.   

    N       (input) INTEGER   
            The order of the matrix dA.  N >= 0.   

    dA      (input/output) REAL array on the GPU, dimension (LDDA,N)   
            On entry, the symmetric matrix dA.  If UPLO = 'U', the leading   
            N-by-N upper triangular part of dA contains the upper   
            triangular part of the matrix dA, and the strictly lower   
            triangular part of dA is not referenced.  If UPLO = 'L', the   
            leading N-by-N lower triangular part of dA contains the lower   
            triangular part of the matrix dA, and the strictly upper   
            triangular part of dA is not referenced.   

            On exit, if INFO = 0, the factor U or L from the Cholesky   
            factorization dA = U\*\*H*U or dA = L*L\*\*H.   

    LDDA    (input) INTEGER   
            The leading dimension of the array dA.  LDDA >= max(1,N).
            To benefit from coalescent memory accesses LDDA must be
            divisible by 16.

    INFO    (output) INTEGER   
            = 0:  successful exit   
            < 0:  if INFO = -i, the i-th argument had an illegal value   
            > 0:  if INFO = i, the leading minor of order i is not   
                  positive definite, and the factorization could not be   
                  completed.   
    =====================================================================   */

extern "C" magma_int_t
magma_spotrf2_gpu(
    magma_uplo_t   uplo,
    magma_int_t    n,
    magmaFloat_ptr dA, size_t dA_offset, 
    magma_int_t    ldda,
    magma_queue_t* queue,
    magma_int_t*   info)
{
    magma_int_t j, jb, nb;
    float  z_one = MAGMA_S_MAKE(  1.0, 0.0 );
    float mz_one = MAGMA_S_MAKE( -1.0, 0.0 );
    float    one =  1.0;
    float  m_one = -1.0;
    float* work;
    magma_int_t err;
    
    *info = 0;
    if ( uplo != MagmaUpper && uplo != MagmaLower ) {
        *info = -1;
    } else if ( n < 0 ) {
        *info = -2;
    } else if ( ldda < max(1,n) ) {
        *info = -4;
    }
    if ( *info != 0 ) {
        magma_xerbla( __func__, -(*info) );
        return *info;
    }
    
    nb = magma_get_spotrf_nb( n );
/*
    err = magma_smalloc_cpu(  &work, nb*nb );
    if ( err != MAGMA_SUCCESS ) {
        *info = MAGMA_ERR_HOST_ALLOC;
        return *info;
    }
*/

    cl_mem buffer = clCreateBuffer(gContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(float)*nb*nb, NULL, NULL);
    work = (float*)clEnqueueMapBuffer(queue[0], buffer, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, nb*nb*sizeof(float), 0, NULL, NULL, NULL);

    if ((nb <= 1) || (nb >= n)) {
        // use unblocked code
        magma_sgetmatrix( n, n, dA, dA_offset, ldda, work, n, queue[1] );
        lapackf77_spotrf( lapack_uplo_const(uplo), &n, work, &n, info );
        magma_ssetmatrix( n, n, work, n, dA, dA_offset, ldda, queue[1] );
    }
    else {
        if ( uplo == MagmaUpper ) {
            // --------------------
            // compute Cholesky factorization A = U'*U
            // using the left looking algorithm
            for( j = 0; j < n; j += nb ) {
                // apply all previous updates to diagonal block
                jb = min( nb, n-j );
                if ( j > 0 ) {
                    magma_ssyrk( MagmaUpper, MagmaConjTrans, jb, j,
                        m_one, dA(0,j), ldda,
                          one, dA(j,j), ldda, queue[1]);
                }
                
                // apply all previous updates to block row right of diagonal block
                if ( j+jb < n && j > 0) {
                    magma_sgemm( MagmaConjTrans, MagmaNoTrans,
                        jb, n-j-jb, j,
                        mz_one, dA(0, j   ), ldda,
                                dA(0, j+jb), ldda,
                        z_one,  dA(j, j+jb), ldda, queue[1]);
                }
                
                // simultaneous with above sgemm, transfer data, factor
                // diagonal block on CPU, and test for positive definiteness
                magma_sgetmatrix_async( jb, jb, dA(j,j), ldda, work, jb, queue[0], NULL );
                magma_queue_sync(queue[0]);
                lapackf77_spotrf( MagmaUpperStr, &jb, work, &jb, info );
                if ( *info != 0 ) {
                    assert( *info > 0 );
                    *info += j;
                    break;
                }
                //clFinish(queue[0]);
                magma_ssetmatrix_async( jb, jb, work, jb, dA(j,j), ldda, queue[0], NULL );
                magma_queue_sync(queue[0]);
                // apply diagonal block to block row right of diagonal block
                if ( j+jb < n ) {
                    magma_strsm(
                        MagmaLeft, MagmaUpper, MagmaConjTrans, MagmaNonUnit,
                        jb, n-j-jb,
                        z_one, dA(j, j),    ldda,
                               dA(j, j+jb), ldda, queue[1]);
                }
            }
        }
        else {
            // --------------------
            // compute Cholesky factorization A = L*L'
            // using the left looking algorithm
            for( j = 0; j < n; j += nb ) {
                // apply all previous updates to diagonal block
                jb = min( nb, n-j );
                if(j>0){
                    magma_ssyrk( MagmaLower, MagmaNoTrans, jb, j,
                        m_one, dA(j, 0), ldda,
                        one, dA(j, j), ldda, queue[1]);
                }
                magma_queue_sync(queue[1]); 
                // apply all previous updates to block column below diagonal block
                if ( j+jb < n && j > 0) {
                    magma_sgemm( MagmaNoTrans, MagmaConjTrans,
                        n-j-jb, jb, j,
                        mz_one, dA(j+jb, 0), ldda,
                                dA(j,    0), ldda,
                        z_one,  dA(j+jb, j), ldda, queue[1]);
                }
                
                // simultaneous with above sgemm, transfer data, factor
                // diagonal block on CPU, and test for positive definiteness
                magma_sgetmatrix_async( jb, jb, dA(j,j), ldda, work, jb, queue[0], NULL );
                magma_queue_sync(queue[0]);
                lapackf77_spotrf( MagmaLowerStr, &jb, work, &jb, info );
                if ( *info != 0 ) {
                    assert( *info > 0 );
                    *info += j;
                    break;
                }
                magma_ssetmatrix_async( jb, jb, work, jb, dA(j,j), ldda, queue[0], NULL );
                magma_queue_sync(queue[0]);

                // apply diagonal block to block column below diagonal
                if ( j+jb < n ) {
                    magma_strsm(
                        MagmaRight, MagmaLower, MagmaConjTrans, MagmaNonUnit,
                        n-j-jb, jb,
                        z_one, dA(j, j   ), ldda,
                               dA(j+jb, j), ldda, queue[1]);
                }
            }
        }
    }
//  magma_free_cpu( work );
    magma_queue_sync(queue[0]);
    magma_queue_sync(queue[1]);
    clEnqueueUnmapMemObject(queue[0], buffer, work, 0, NULL, NULL);
    clReleaseMemObject(buffer);
    return *info;
}
