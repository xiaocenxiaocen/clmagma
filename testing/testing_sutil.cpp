/*
    -- MAGMA (version 1.1.0-beta2) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date November 2013

       @generated s Mon Nov 25 17:56:10 2013

       @author Mark Gates

       Utilities for testing.
*/

#include "testings.h"

#define A(i,j)  A[i + j*lda]

// --------------------
// Make a matrix symmetric/symmetric.
// Makes diagonal real.
// Sets Aji = conj( Aij ) for j < i, that is, copy lower triangle to upper triangle.
extern "C"
void magma_smake_hermitian( magma_int_t N, float* A, magma_int_t lda )
{
    magma_int_t i, j;
    for( i=0; i<N; ++i ) {
        A(i,i) = MAGMA_S_MAKE( MAGMA_S_REAL( A(i,i) ), 0. );
        for( j=0; j<i; ++j ) {
            A(j,i) = MAGMA_S_CNJG( A(i,j) );
        }
    }
}


// --------------------
// Make a matrix symmetric/symmetric positive definite.
// Increases diagonal by N, and makes it real.
// Sets Aji = conj( Aij ) for j < i, that is, copy lower triangle to upper triangle.
extern "C"
void magma_smake_hpd( magma_int_t N, float* A, magma_int_t lda )
{
    magma_int_t i, j;
    for( i=0; i<N; ++i ) {
        A(i,i) = MAGMA_S_MAKE( MAGMA_S_REAL( A(i,i) ) + N, 0. );
        for( j=0; j<i; ++j ) {
            A(j,i) = MAGMA_S_CNJG( A(i,j) );
        }
    }
}
