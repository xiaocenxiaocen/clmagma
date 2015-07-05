/*
 *  -- clMAGMA (version 1.0.0) --
 *     Univ. of Tennessee, Knoxville
 *     Univ. of California, Berkeley
 *     Univ. of Colorado, Denver
 *     April 2012
 *
 * @generated s Wed Oct 24 00:33:04 2012
 *
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "flops.h"
#include "magma.h"
#include "magma_lapack.h"
#include "testings.h"

// Flops formula
#define PRECISION_s
#if defined(PRECISION_z) || defined(PRECISION_c)
#define FLOPS(m, n, k) ( 6. * FMULS_GEMM(m, n, k) + 2. * FADDS_GEMM(m, n, k))
#else
#define FLOPS(m, n, k) (      FMULS_GEMM(m, n, k) +      FADDS_GEMM(m, n, k))
#endif

int main( int argc, char** argv)
{
    real_Double_t gflops, gpu_perf, cpu_perf, gpu_time, cpu_time;
    float      error, work[1];
    int         transA = MagmaNoTrans;
    int         transB = MagmaNoTrans;

    magma_int_t istart = 1024;
    magma_int_t iend   = 6240;
    magma_int_t M, M0 = 0;
    magma_int_t N, N0 = 0;
    magma_int_t K, K0 = 0;
    magma_int_t i;
    magma_int_t Am, An, Bm, Bn;
    magma_int_t szeA, szeB, szeC;
    magma_int_t lda, ldb, ldc, ldda, lddb, lddc;
    magma_int_t ione     = 1;
    magma_int_t ISEED[4] = {0,0,0,1};
    
    float *h_A, *h_B, *h_C, *h_C2;
    magmaFloat_ptr d_A, d_B, d_C;
    float mzone = MAGMA_S_NEG_ONE;
    float alpha = MAGMA_S_MAKE(  0.29, -0.86 );
    float beta  = MAGMA_S_MAKE( -0.48,  0.38 );

    if (argc != 1){
        for(i=1; i<argc; i++){
            if ( strcmp("-N", argv[i]) == 0 ){
                N0 = atoi(argv[++i]);
            }
            else if ( strcmp("-M", argv[i]) == 0 ){
                M0 = atoi(argv[++i]);
            }
            else if ( strcmp("-K", argv[i]) == 0 ){
                K0 = atoi(argv[++i]);
            }
            else if (strcmp("-NN", argv[i])==0){
                transA = transB = MagmaNoTrans;
            }
            else if (strcmp("-TT", argv[i])==0){
                transA = transB = MagmaTrans;
            }
            else if (strcmp("-NT", argv[i])==0){
                transA = MagmaNoTrans;
                transB = MagmaTrans;
            }
            else if (strcmp("-TN", argv[i])==0){
                transA = MagmaTrans;
                transB = MagmaNoTrans;
            }
#if defined(PRECISION_z) || defined(PRECISION_c)
            else if (strcmp("-NC", argv[i])==0){
                transA = MagmaNoTrans;
                transB = MagmaTrans;
            }
            else if (strcmp("-TC", argv[i])==0){
                transA = MagmaTrans;
                transB = MagmaTrans;
            }
            else if (strcmp("-CN", argv[i])==0){
                transA = MagmaTrans;
                transB = MagmaNoTrans;
            }
            else if (strcmp("-CT", argv[i])==0){
                transA = MagmaTrans;
                transB = MagmaTrans;
            }
            else if (strcmp("-CC", argv[i])==0){
                transA = transB = MagmaTrans;
            }
#endif
        }
    }

    if ( (M0 != 0) && (N0 != 0) && (K0 != 0) )
        iend = istart + 1;
    
    M = N = K = iend;
    if ( M0 != 0 ) M = M0;
    if ( N0 != 0 ) N = N0;
    if ( K0 != 0 ) K = K0;
    
    if( transA == MagmaNoTrans ) {
        Am = M;
        An = K;
    }  else {
        Am = K;
        An = M;
    }
    
    if( transB == MagmaNoTrans ) {
        Bm = K;
        Bn = N;
    }  else {
        Bm = N;
        Bn = K;
    }
    
    /* Initialize */
    magma_queue_t  queue;
    magma_device_t device;
    int num = 0;
    magma_err_t err;
    magma_init();
    err = magma_get_devices( &device, 1, &num );
    if ( err != 0 || num < 1 ) {
      fprintf( stderr, "magma_get_devices failed: %d\n", err );
      exit(-1);
    }
    err = magma_queue_create( device, &queue );
    if ( err != 0 ) {
      fprintf( stderr, "magma_queue_create failed: %d\n", err );
      exit(-1);
    }

    lda = ldc = M;
    ldb = Bm;
    
    ldda = lddc = ((M+31)/32)*32;
    lddb = ((ldb+31)/32)*32;

    K+=32;
    M+=32;
    N +=32;

    TESTING_MALLOC( h_A,  float, lda*K );
    TESTING_MALLOC( h_B,  float, ldb*Bn );
    TESTING_MALLOC( h_C,  float, ldc*N );
    TESTING_MALLOC( h_C2, float, ldc*N );

    TESTING_MALLOC_DEV( d_A, float, ldda*K );
    TESTING_MALLOC_DEV( d_B, float, lddb*Bn );
    TESTING_MALLOC_DEV( d_C, float, lddc*N );

    printf("\nUsage: \n");
    printf("  testing_sgemm [-NN|NT|TN|TT] [-N %d] \n\n", 1024);

    printf("\n");
    printf("Testing transA = %c  transB = %c\n", transA, transB);
    printf("    M    N    K   clAmdBlas GFLop/s (sec)    CPU GFlop/s (sec)     error\n");
    printf("===========================================================================\n");
    for(i=istart; i<iend; i = (int)(i*1.25) )
    {
        M = N = K = i;
        if ( M0 != 0 ) M = M0;
        if ( N0 != 0 ) N = N0;
        if ( K0 != 0 ) K = K0;

        if( transA == MagmaNoTrans ) {
            lda = Am = M;
            An = K;
        }  else {
            lda = Am = K;
            An = M;
        }

        if( transB == MagmaNoTrans ) {
            ldb = Bm = K;
            Bn = N;
        }  else {
            ldb = Bm = N;
            Bn = K;
        }
        gflops = FLOPS( (float)M, (float)N, (float)K ) * 1e-9;
        ldc = M;

        ldda = ((lda+31)/32)*32;
        lddb = ((ldb+31)/32)*32;
        lddc = ((ldc+31)/32)*32;

        szeA = lda * An;
        szeB = ldb * Bn;
        szeC = ldc * N;

        /* Initialize the matrices */
        lapackf77_slarnv( &ione, ISEED, &szeA, h_A );
        lapackf77_slarnv( &ione, ISEED, &szeB, h_B );
        lapackf77_slarnv( &ione, ISEED, &szeC, h_C );
        
        /* =====================================================================
           Performs operation using MAGMA-BLAS
           =================================================================== */
	magma_ssetmatrix( Am, An, h_A, 0, lda, d_A, 0, ldda, queue );
	magma_ssetmatrix( Bm, Bn, h_B, 0, ldb, d_B, 0, lddb, queue );
	magma_ssetmatrix( M, N, h_C, 0, ldc, d_C, 0, lddc, queue ); 
	
	magma_sgemm( transA, transB, M, N, K,
                     alpha, d_A, 0, ldda,
                     d_B, 0, lddb,
                     beta,  d_C, 0, lddc, queue );
	magma_ssetmatrix( M, N, h_C, 0, ldc, d_C, 0, lddc, queue );
	magma_queue_sync( queue );

        gpu_time = get_time();
        magma_sgemm( transA, transB, M, N, K, 
		     alpha, d_A, 0, ldda,
		     d_B, 0, lddb,
		     beta,  d_C, 0, lddc, queue );
	magma_queue_sync( queue);
        gpu_time = get_time() - gpu_time;
        gpu_perf = gflops / gpu_time;
        
        magma_sgetmatrix( M, N, d_C, 0, lddc, h_C2, 0, ldc, queue );
        
        /* =====================================================================
           Performs operation using CUDA-BLAS
           =================================================================== */

        cpu_time = get_time();
        blasf77_sgemm( lapack_const(transA), lapack_const(transB),
		       &M, &N, &K, 
		       &alpha, h_A, &lda,
		       h_B, &ldb,
		       &beta,  h_C, &ldc );
        cpu_time = get_time() - cpu_time; 
        cpu_perf = gflops / cpu_time;
        
        /* =====================================================================
           Error Computation and Performance Compariosn
           =================================================================== */
        blasf77_saxpy(&szeC, &mzone, h_C, &ione, h_C2, &ione);
        error = lapackf77_slange("M", &M, &N, h_C2, &ldc, work);
        printf("%5d %5d %5d    %8.2f (%6.2f)    %6.2f (%6.2f)    %e\n",
               M, N, K, gpu_perf, gpu_time, cpu_perf, cpu_time, error);
    }

    /* Memory clean up */
    TESTING_FREE( h_A );
    TESTING_FREE( h_B );
    TESTING_FREE( h_C );
    TESTING_FREE( h_C2 );

    TESTING_FREE_DEV( d_A );
    TESTING_FREE_DEV( d_B );
    TESTING_FREE_DEV( d_C );

    magma_queue_destroy( queue );
    magma_finalize();
}
