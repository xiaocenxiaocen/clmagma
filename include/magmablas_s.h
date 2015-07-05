/*
 *   -- clMAGMA (version 0.1) --
 *      Univ. of Tennessee, Knoxville
 *      Univ. of California, Berkeley
 *      Univ. of Colorado, Denver
 *      April 2012
 *
 * @author Mark Gates
 * @generated s Wed Apr  4 01:12:51 2012
 */

#ifndef MAGMA_BLAS_S_H
#define MAGMA_BLAS_S_H

#include "magma_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// copying sub-matrices (contiguous columns)
magma_err_t
magma_ssetmatrix(
	magma_int_t m, magma_int_t n,
	float const* hA_src, size_t hA_offset, magma_int_t ldha,
	magmaFloat_ptr    dA_dst, size_t dA_offset, magma_int_t ldda,
	magma_queue_t queue );

magma_err_t
magma_sgetmatrix(
	magma_int_t m, magma_int_t n,
	magmaFloat_const_ptr dA_src, size_t dA_offset, magma_int_t ldda,
	float*          hA_dst, size_t hA_offset, magma_int_t ldha,
	magma_queue_t queue );

magma_err_t
magma_ssetmatrix_async(
	magma_int_t m, magma_int_t n,
	float const* hA_src, size_t hA_offset, magma_int_t ldha,
	magmaFloat_ptr    dA_dst, size_t dA_offset, magma_int_t ldda,
	magma_queue_t queue, magma_event_t *event );

magma_err_t
magma_sgetmatrix_async(
	magma_int_t m, magma_int_t n,
	magmaFloat_const_ptr dA_src, size_t dA_offset, magma_int_t ldda,
	float*          hA_dst, size_t hA_offset, magma_int_t ldha,
	magma_queue_t queue, magma_event_t *event );


// ========================================
// matrix transpose and swapping functions
magma_err_t
magma_sinplace_transpose(
    magmaFloat_ptr dA, size_t dA_offset, int lda, int n,
    magma_queue_t queue );

magma_err_t
magma_stranspose2(
    magmaFloat_ptr odata, size_t odata_offset, int ldo,
    magmaFloat_ptr idata, size_t idata_offset, int ldi,
    int m, int n,
    magma_queue_t queue );

magma_err_t
magma_stranspose(
    magmaFloat_ptr odata, int offo, int ldo,
    magmaFloat_ptr idata, int offi, int ldi,
    int m, int n,
    magma_queue_t queue );

magma_err_t
magma_spermute_long2(
    magmaFloat_ptr dAT, size_t dAT_offset, int lda,
    int *ipiv, int nb, int ind,
    magma_queue_t queue );


// ========================================
// BLAS functions
magma_err_t
magma_sgemm(
	magma_trans_t transA, magma_trans_t transB,
	magma_int_t m, magma_int_t n, magma_int_t k,
	float alpha, magmaFloat_const_ptr dA, size_t dA_offset, magma_int_t lda,
	                          magmaFloat_const_ptr dB, size_t dB_offset, magma_int_t ldb,
	float beta,  magmaFloat_ptr       dC, size_t dC_offset, magma_int_t ldc,
	magma_queue_t queue );

magma_err_t
magma_sgemv(
	magma_trans_t transA,
	magma_int_t m, magma_int_t n,
	float alpha, magmaFloat_const_ptr dA, size_t dA_offset, magma_int_t lda,
	                          magmaFloat_const_ptr dx, size_t dx_offset, magma_int_t incx,
	float beta,  magmaFloat_ptr       dy, size_t dy_offset, magma_int_t incy,
	magma_queue_t queue );

magma_err_t
magma_ssymm(
	magma_side_t side, magma_uplo_t uplo,
	magma_int_t m, magma_int_t n,
	float alpha, magmaFloat_const_ptr dA, size_t dA_offset, magma_int_t lda,
	                          magmaFloat_const_ptr dB, size_t dB_offset, magma_int_t ldb,
	float beta,  magmaFloat_ptr       dC, size_t dC_offset, magma_int_t ldc,
	magma_queue_t queue );

magma_err_t
magma_ssymv(
	magma_uplo_t uplo,
	magma_int_t n,
	float alpha, magmaFloat_const_ptr dA, size_t dA_offset, magma_int_t lda,
	                          magmaFloat_const_ptr dx, size_t dx_offset, magma_int_t incx,
	float beta,  magmaFloat_ptr       dy, size_t dy_offset, magma_int_t incy,
	magma_queue_t queue );

magma_err_t
magma_ssyrk(
    magma_uplo_t uplo, magma_trans_t trans,
    magma_int_t n, magma_int_t k,
    float alpha, magmaFloat_const_ptr dA, size_t dA_offset, magma_int_t lda,
    float beta,  magmaFloat_ptr       dC, size_t dC_offset, magma_int_t ldc,
    magma_queue_t queue );

magma_err_t
magma_strsm(
    magma_side_t side, magma_uplo_t uplo, magma_trans_t trans, magma_diag_t diag,
    magma_int_t m, magma_int_t n,
    float alpha, magmaFloat_const_ptr dA, size_t dA_offset, magma_int_t lda,
                              magmaFloat_ptr       dB, size_t dB_offset, magma_int_t ldb,
    magma_queue_t queue );

magma_err_t
magma_strmm(
    magma_side_t side, magma_uplo_t uplo, magma_trans_t trans, magma_diag_t diag,
    magma_int_t m, magma_int_t n,
    float alpha, magmaFloat_const_ptr dA, size_t dA_offset, magma_int_t lda,
                              magmaFloat_ptr       dB, size_t dB_offset, magma_int_t ldb,
    magma_queue_t queue );

#ifdef __cplusplus
}
#endif

#endif        //  #ifndef MAGMA_BLAS_H
