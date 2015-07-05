/*
    -- clMAGMA (version 1.0.0) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       September 2012

       @generated c Wed Oct 24 00:33:04 2012

*/

// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// includes, project
#include "magma.h"
#include "magma_lapack.h"
#include "testings.h"

/* ////////////////////////////////////////////////////////////////////////////
   -- Testing cgeev
*/
#define PRECISION_c

int main( int argc, char** argv) 
{
    real_Double_t	gpu_time, cpu_time;
    magmaFloatComplex *h_A, *h_R, *VL, *VR, *h_work, *w1, *w2;
    magmaFloatComplex *w1i, *w2i;
    magmaFloatComplex  c_neg_one = MAGMA_C_NEG_ONE;
    float          *rwork;
    float          matnorm, tnrm, result[8];

    /* Matrix size */
    magma_int_t N=0, n2, lda, nb, lwork;
    magma_int_t size[8] = {1024,2048,3072,4032,5184,6016,7040,8064};

    magma_int_t i, j, info, checkres, once = 0;
    magma_int_t ione     = 1;
    magma_int_t ISEED[4] = {0,0,0,1};

    magma_vec_t jobl = MagmaVec;
    magma_vec_t jobr = MagmaVec;

    if (argc != 1){
        for(i = 1; i<argc; i++){
            if (strcmp("-N", argv[i])==0) {
                N = atoi(argv[++i]);
                once = 1;
            }
            else if (strcmp("-LN", argv[i])==0)
                jobl = MagmaNoVec;
            else if (strcmp("-LV", argv[i])==0)
                jobl = MagmaVec;
            else if (strcmp("-RN", argv[i])==0)
                jobr = MagmaNoVec;
            else if (strcmp("-RV", argv[i])==0)
                jobr = MagmaVec;
        }
        if ( N > 0 )
            printf("  testing_cgeev -L[N|V] -R[N|V] -N %d\n\n", (int) N);
        else
        {
            printf("\nUsage: \n");
            printf("  testing_cgeev -L[N|V] -R[N|V] -N %d\n\n", 1024);
            exit(1);
        }
    }
    else {
        printf("\nUsage: \n");
        printf("  testing_cgeev -L[N|V] -R[N|V] -N %d\n\n", 1024);
        N = size[7];
    }

    checkres = getenv("MAGMA_TESTINGS_CHECK") != NULL;

    lda   = N;
    n2    = lda * N;
    nb    = magma_get_cgehrd_nb(N);

    lwork = N*(1+nb);

    // generous workspace - required by cget22
    lwork = max(lwork, N * ( 5 + 2*N));

    w1i   = NULL; 
    w2i   = NULL;
    rwork = NULL;

    TESTING_MALLOC( w1,  magmaFloatComplex, N );
    TESTING_MALLOC( w2,  magmaFloatComplex, N );

    TESTING_MALLOC( rwork, float, 2*N ); /* Why is it existing in real ??? */

    TESTING_MALLOC   ( h_A, magmaFloatComplex, n2);
    TESTING_MALLOC_HOST( h_R, magmaFloatComplex, n2);
    TESTING_MALLOC_HOST( VL , magmaFloatComplex, n2);
    TESTING_MALLOC_HOST( VR , magmaFloatComplex, n2);
    TESTING_MALLOC_HOST( h_work, magmaFloatComplex, lwork);
	
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

    printf("  N     CPU Time(s)    GPU Time(s)     ||R||_F / ||A||_F\n");
    printf("==========================================================\n");
    for(i=0; i<8; i++){
        if ( argc == 1 ){
            N = size[i];
        }
        
        lda = N;
        n2  = lda*N;

        /* Initialize the matrix */
        lapackf77_clarnv( &ione, ISEED, &n2, h_A );
        lapackf77_clacpy( MagmaUpperLowerStr, &N, &N, h_A, &lda, h_R, &lda );

        /* ====================================================================
           Performs operation using MAGMA
           =================================================================== */
        // warm-up
		magma_cgeev(jobl, jobr,
                    N, h_R, lda, w1, 
                    VL, lda, VR, lda,
                    h_work, lwork, rwork, &info, queue);
        
		lapackf77_clacpy( MagmaUpperLowerStr, &N, &N, h_A, &lda, h_R, &lda );
		gpu_time = get_time();
        magma_cgeev(jobl, jobr,
                    N, h_R, lda, w1, 
                    VL, lda, VR, lda,
                    h_work, lwork, rwork, &info, queue);

        gpu_time = get_time() - gpu_time;
        if (info < 0)
            printf("Argument %d of magma_cgeev had an illegal value.\n", (int) -info);

        /* =====================================================================
           Performs operation using LAPACK
           =================================================================== */
        cpu_time = get_time();
        lapackf77_cgeev(lapack_const(jobl), lapack_const(jobr),
                        &N, h_A, &lda, w2, 
                        VL, &lda, VR, &lda,
                        h_work, &lwork, rwork, &info);

        cpu_time = get_time() - cpu_time;
        if (info < 0)
            printf("Argument %d of cgeev had an illegal value.\n", (int) -info);

        /* =====================================================================
           Check the result compared to LAPACK
           =================================================================== */
        if ( checkres ) 
          {
            /* ===================================================================
               Check the result following LAPACK's [zcds]drvev routine.
               The following 7 tests are performed:
               *     (1)     | A * VR - VR * W | / ( n |A| )
               *
               *       Here VR is the matrix of unit right eigenvectors.
               *       W is a diagonal matrix with diagonal entries W(j).
               *
               *     (2)     | A**H * VL - VL * W**H | / ( n |A| )
               *
               *       Here VL is the matrix of unit left eigenvectors, A**H is the
               *       conjugate-transpose of A, and W is as above.
               *
               *     (3)     | |VR(i)| - 1 |   and whether largest component real
               *
               *       VR(i) denotes the i-th column of VR.
               *
               *     (4)     | |VL(i)| - 1 |   and whether largest component real
               *
               *       VL(i) denotes the i-th column of VL.
               *
               *     (5)     W(full) = W(partial)
               *
               *       W(full) denotes the eigenvalues computed when both VR and VL
               *       are also computed, and W(partial) denotes the eigenvalues
               *       computed when only W, only W and VR, or only W and VL are
               *       computed.
               *
               *     (6)     VR(full) = VR(partial)
               *
               *       VR(full) denotes the right eigenvectors computed when both VR
               *       and VL are computed, and VR(partial) denotes the result
               *       when only VR is computed.
               *
               *     (7)     VL(full) = VL(partial)
               *
               *       VL(full) denotes the left eigenvectors computed when both VR
               *       and VL are also computed, and VL(partial) denotes the result
               *       when only VL is computed.
               ================================================================= */
            
            int jj;
            float ulp, ulpinv, vmx, vrmx, vtst;

            magmaFloatComplex *LRE, DUM;
            TESTING_MALLOC_HOST( LRE , magmaFloatComplex, n2);

            ulp = lapackf77_slamch( "P" );
            ulpinv = 1./ulp;

            // Initialize RESULT
            for (j = 0; j < 8; j++)
              result[j] = -1.;

            lapackf77_clarnv( &ione, ISEED, &n2, h_A );
            lapackf77_clacpy( MagmaUpperLowerStr, &N, &N, h_A, &lda, h_R, &lda );

            magma_cgeev(MagmaVec, MagmaVec,
                        N, h_R, lda, w1,
                        VL, lda, VR, lda,
                        h_work, lwork, rwork, &info, queue);

            // Do test 1
            lapackf77_cget22("N", "N", "N", &N, h_A, &lda, VR, &lda, w1,
                             h_work, rwork, &result[0]);
            result[0] *= ulp;

            // Do test 2
            lapackf77_cget22("C", "N", "C", &N, h_A, &lda, VL, &lda, w1, 
                             h_work, rwork, &result[1]);
            result[1] *= ulp;

            // Do test 3 
            result[2] = -1.;
            for (j = 0; j < N; ++j) {
              tnrm = cblas_scnrm2(N, &VR[j * lda], ione);              
              result[2] = fmax(result[2], fmin(ulpinv, fabs(tnrm-1.)/ulp));

              vmx  = vrmx = 0.;
              for (jj = 0; jj <N; ++jj) {
                vtst = MAGMA_C_ABS(VR[jj + j * lda]);
                if (vtst > vmx)
                  vmx = vtst;
                
                if (MAGMA_C_IMAG(VR[jj + j*lda])==0. && 
                    fabs( MAGMA_C_REAL(VR[jj+j*lda]) ) > vrmx)
                  vrmx = fabs( MAGMA_C_REAL( VR[jj+j*lda] ) );
              }
              if (vrmx / vmx < 1. - ulp * 2.)
                result[2] = ulpinv;
            }
            result[2] *= ulp;

            // Do test 4 
            result[3] = -1.;
            for (j = 0; j < N; ++j) {
              tnrm = cblas_scnrm2(N, &VL[j * lda], ione);
              result[3] = fmax(result[3], fmin(ulpinv,fabs(tnrm - 1.)/ ulp));

              vmx = vrmx = 0.;
              for (jj = 0; jj < N; ++jj) {
                vtst = MAGMA_C_ABS(VL[jj + j * lda]);
                if (vtst > vmx)
                  vmx = vtst;
               
                if (MAGMA_C_IMAG(VL[jj + j*lda])==0. && 
                    fabs( MAGMA_C_REAL( VL[jj + j*lda] ) ) > vrmx)
                  vrmx = fabs( MAGMA_C_REAL( VL[jj+j*lda]) );
              }
              if (vrmx / vmx < 1. - ulp * 2.)
                result[3] = ulpinv;
            }
            result[3] *= ulp;

            // Compute eigenvalues only, and test them 
            lapackf77_clacpy( MagmaUpperLowerStr, &N, &N, h_A, &lda, h_R, &lda );
            magma_cgeev(MagmaNoVec, MagmaNoVec,
                        N, h_R, lda, w2,
                        &DUM, 1, &DUM, 1,
                        h_work, lwork, rwork, &info, queue);

            if (info != 0) {
              result[0] = ulpinv;
             
              info = abs(info);
              printf("Info = %d fo case N, N\n", (int) info);
            }

            // Do test 5 
            result[4] = 1;
            for (j = 0; j < N; ++j)
              if ( MAGMA_C_REAL( w1[j] ) != MAGMA_C_REAL(w2[j]) || 
                   MAGMA_C_IMAG( w1[j] ) != MAGMA_C_IMAG(w2[j]) )
                result[4] = 0;
            //if (result[4] == 0) printf("test 5 failed with N N\n");

            // Compute eigenvalues and right eigenvectors, and test them
            lapackf77_clacpy( MagmaUpperLowerStr, &N, &N, h_A, &lda, h_R, &lda );
            magma_cgeev(MagmaNoVec, MagmaVec,
                        N, h_R, lda, w2,
                        &DUM, 1, LRE, lda,
                        h_work, lwork, rwork, &info, queue);

            if (info != 0) {
              result[0] = ulpinv;

              info = abs(info);
              printf("Info = %d fo case N, V\n", (int) info);
            }

            // Do test 5 again
            result[4] = 1;
            for (j = 0; j < N; ++j)
              if ( MAGMA_C_REAL( w1[j] ) != MAGMA_C_REAL(w2[j]) ||
                   MAGMA_C_IMAG( w1[j] ) != MAGMA_C_IMAG(w2[j]) )
                result[4] = 0;
            //if (result[4] == 0) printf("test 5 failed with N V\n");

            // Do test 6
            result[5] = 1;
            for (j = 0; j < N; ++j)
              for (jj = 0; jj < N; ++jj)
                if ( MAGMA_C_REAL( VR[j+jj*lda] ) != MAGMA_C_REAL( LRE[j+jj*lda] ) ||
                     MAGMA_C_IMAG( VR[j+jj*lda] ) != MAGMA_C_IMAG( LRE[j+jj*lda] ) )
                result[5] = 0;
 

            // Compute eigenvalues and left eigenvectors, and test them
            lapackf77_clacpy( MagmaUpperLowerStr, &N, &N, h_A, &lda, h_R, &lda );
            magma_cgeev(MagmaVec, MagmaNoVec,
                        N, h_R, lda, w2,
                        LRE, lda, &DUM, 1, 
                        h_work, lwork, rwork, &info, queue);

            if (info != 0) {
              result[0] = ulpinv;

              info = abs(info);
              printf("Info = %d fo case V, N\n", (int) info);
            }

            // Do test 5 again
            result[4] = 1;
            for (j = 0; j < N; ++j)
              if ( MAGMA_C_REAL( w1[j] ) != MAGMA_C_REAL(w2[j]) ||
                   MAGMA_C_IMAG( w1[j] ) != MAGMA_C_IMAG(w2[j]) )
                result[4] = 0;
            //if (result[4] == 0) printf("test 5 failed with V N\n");

            // Do test 7 
            result[6] = 1;
            for (j = 0; j < N; ++j)
              for (jj = 0; jj < N; ++jj)
                if ( MAGMA_C_REAL( VL[j+jj*lda] ) != MAGMA_C_REAL( LRE[j+jj*lda] ) ||
                     MAGMA_C_IMAG( VL[j+jj*lda] ) != MAGMA_C_IMAG( LRE[j+jj*lda] ) )
                  result[6] = 0;

            printf("Test 1: | A * VR - VR * W | / ( n |A| ) = %e\n", result[0]);
            printf("Test 2: | A'* VL - VL * W'| / ( n |A| ) = %e\n", result[1]);
            printf("Test 3: |  |VR(i)| - 1    |             = %e\n", result[2]);
            printf("Test 4: |  |VL(i)| - 1    |             = %e\n", result[3]);
            printf("Test 5:   W (full)  ==  W (partial)     = %f\n", result[4]);
            printf("Test 6:  VR (full)  == VR (partial)     = %f\n", result[5]);
            printf("Test 7:  VL (full)  == VL (partial)     = %f\n", result[6]);

            //====================================================================

            matnorm = lapackf77_clange("f", &N, &ione, w1, &N, rwork);
            blasf77_caxpy(&N, &c_neg_one, w1, &ione, w2, &ione);

            result[7] = lapackf77_clange("f", &N, &ione, w2, &N, rwork) / matnorm;

            printf("%5d     %6.2f         %6.2f         %e\n",
                   (int) N, cpu_time, gpu_time, result[7]);

            TESTING_FREE_HOST( LRE );
          } 
        else 
          {
            printf("%5d     %6.2f         %6.2f\n",
                   (int) N, cpu_time, gpu_time);
          }

        if (argc != 1)
            break;
    }

    /* Memory clean up */
    TESTING_FREE(w1);
    TESTING_FREE(w2);

    TESTING_FREE(rwork);
    TESTING_FREE( h_A );
    TESTING_FREE_HOST(  h_R );
    TESTING_FREE_HOST(  VL  );
    TESTING_FREE_HOST(  VR  );
    TESTING_FREE_HOST(h_work);

    /* Shutdown */
    magma_queue_destroy( queue );
    magma_finalize();
}
