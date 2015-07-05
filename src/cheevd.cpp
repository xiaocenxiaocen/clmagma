/*
    -- clMAGMA (version 1.1.0-beta2) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date November 2013

       @author Stan Tomov
       @author Raffaele Solca

       @generated c Mon Nov 25 17:56:00 2013

*/

#include <stdio.h>
#include "common_magma.h"

extern "C" magma_int_t
magma_cheevd(magma_vec_t jobz, magma_uplo_t uplo,
             magma_int_t n,
             magmaFloatComplex *a, magma_int_t lda,
             float *w,
             magmaFloatComplex *work, magma_int_t lwork,
             float *rwork, magma_int_t lrwork,
             magma_int_t *iwork, magma_int_t liwork,
             magma_int_t *info, magma_queue_t queue)
{
/*  -- clMAGMA (version 1.1.0-beta2) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       @date November 2013

    Purpose
    =======
    CHEEVD computes all eigenvalues and, optionally, eigenvectors of a
    complex Hermitian matrix A.  If eigenvectors are desired, it uses a
    divide and conquer algorithm.

    The divide and conquer algorithm makes very mild assumptions about
    floating point arithmetic. It will work on machines with a guard
    digit in add/subtract, or on those binary machines without guard
    digits which subtract like the Cray X-MP, Cray Y-MP, Cray C-90, or
    Cray-2. It could conceivably fail on hexadecimal or decimal machines
    without guard digits, but we know of none.

    Arguments
    =========
    JOBZ    (input) CHARACTER*1
            = 'N':  Compute eigenvalues only;
            = 'V':  Compute eigenvalues and eigenvectors.

    UPLO    (input) CHARACTER*1
            = 'U':  Upper triangle of A is stored;
            = 'L':  Lower triangle of A is stored.

    N       (input) INTEGER
            The order of the matrix A.  N >= 0.

    A       (input/output) COMPLEX array, dimension (LDA, N)
            On entry, the Hermitian matrix A.  If UPLO = 'U', the
            leading N-by-N upper triangular part of A contains the
            upper triangular part of the matrix A.  If UPLO = 'L',
            the leading N-by-N lower triangular part of A contains
            the lower triangular part of the matrix A.
            On exit, if JOBZ = 'V', then if INFO = 0, A contains the
            orthonormal eigenvectors of the matrix A.
            If JOBZ = 'N', then on exit the lower triangle (if UPLO='L')
            or the upper triangle (if UPLO='U') of A, including the
            diagonal, is destroyed.

    LDA     (input) INTEGER
            The leading dimension of the array A.  LDA >= max(1,N).

    W       (output) DOUBLE PRECISION array, dimension (N)
            If INFO = 0, the eigenvalues in ascending order.

    WORK    (workspace/output) COMPLEX array, dimension (MAX(1,LWORK))
            On exit, if INFO = 0, WORK[0] returns the optimal LWORK.

    LWORK   (input) INTEGER
            The length of the array WORK.
            If N <= 1,                LWORK must be at least 1.
            If JOBZ  = 'N' and N > 1, LWORK must be at least N + N*NB.
            If JOBZ  = 'V' and N > 1, LWORK must be at least 2*N + N**2.
            NB can be obtained through magma_get_chetrd_nb(N).

            If LWORK = -1, then a workspace query is assumed; the routine
            only calculates the optimal sizes of the WORK, RWORK and
            IWORK arrays, returns these values as the first entries of
            the WORK, RWORK and IWORK arrays, and no error message
            related to LWORK or LRWORK or LIWORK is issued by XERBLA.

    RWORK   (workspace/output) DOUBLE PRECISION array, dimension (LRWORK)
            On exit, if INFO = 0, RWORK[0] returns the optimal LRWORK.

    LRWORK  (input) INTEGER
            The dimension of the array RWORK.
            If N <= 1,                LRWORK must be at least 1.
            If JOBZ  = 'N' and N > 1, LRWORK must be at least N.
            If JOBZ  = 'V' and N > 1, LRWORK must be at least 1 + 5*N + 2*N**2.

            If LRWORK = -1, then a workspace query is assumed; the
            routine only calculates the optimal sizes of the WORK, RWORK
            and IWORK arrays, returns these values as the first entries
            of the WORK, RWORK and IWORK arrays, and no error message
            related to LWORK or LRWORK or LIWORK is issued by XERBLA.

    IWORK   (workspace/output) INTEGER array, dimension (MAX(1,LIWORK))
            On exit, if INFO = 0, IWORK[0] returns the optimal LIWORK.

    LIWORK  (input) INTEGER
            The dimension of the array IWORK.
            If N <= 1,                LIWORK must be at least 1.
            If JOBZ  = 'N' and N > 1, LIWORK must be at least 1.
            If JOBZ  = 'V' and N > 1, LIWORK must be at least 3 + 5*N.

            If LIWORK = -1, then a workspace query is assumed; the
            routine only calculates the optimal sizes of the WORK, RWORK
            and IWORK arrays, returns these values as the first entries
            of the WORK, RWORK and IWORK arrays, and no error message
            related to LWORK or LRWORK or LIWORK is issued by XERBLA.

    INFO    (output) INTEGER
            = 0:  successful exit
            < 0:  if INFO = -i, the i-th argument had an illegal value
            > 0:  if INFO = i and JOBZ = 'N', then the algorithm failed
                  to converge; i off-diagonal elements of an intermediate
                  tridiagonal form did not converge to zero;
                  if INFO = i and JOBZ = 'V', then the algorithm failed
                  to compute an eigenvalue while working on the submatrix
                  lying in rows and columns INFO/(N+1) through
                  mod(INFO,N+1).

    Further Details
    ===============
    Based on contributions by
       Jeff Rutter, Computer Science Division, University of California
       at Berkeley, USA

    Modified description of INFO. Sven, 16 Feb 05.
    =====================================================================   */

    magma_uplo_t uplo_ = uplo;
    magma_vec_t jobz_ = jobz;
    magma_int_t ione = 1;
    magma_int_t izero = 0;
    float d_one = 1.;
    
    float d__1;
    
    float eps;
    magma_int_t inde;
    float anrm;
    magma_int_t imax;
    float rmin, rmax;
    float sigma;
    magma_int_t iinfo, lwmin;
    magma_int_t lower;
    magma_int_t llrwk;
    magma_int_t wantz;
    magma_int_t indwk2, llwrk2;
    magma_int_t iscale;
    float safmin;
    float bignum;
    magma_int_t indtau;
    magma_int_t indrwk, indwrk, liwmin;
    magma_int_t lrwmin, llwork;
    float smlnum;
    magma_int_t lquery;
    
    magmaDouble_ptr dwork;
    
    wantz = lapackf77_lsame(lapack_const(jobz_), MagmaVecStr);
    lower = lapackf77_lsame(lapack_const(uplo_), MagmaLowerStr);
    lquery = lwork == -1 || lrwork == -1 || liwork == -1;
    
    *info = 0;
    if (! (wantz || lapackf77_lsame(lapack_const(jobz_), MagmaNoVecStr))) {
        *info = -1;
    } else if (! (lower || lapackf77_lsame(lapack_const(uplo_), MagmaUpperStr))) {
        *info = -2;
    } else if (n < 0) {
        *info = -3;
    } else if (lda < max(1,n)) {
        *info = -5;
    }
    
    magma_int_t nb = magma_get_chetrd_nb( n );
    if ( n <= 1 ) {
        lwmin  = 1;
        lrwmin = 1;
        liwmin = 1;
    }
    else if ( wantz ) {
        lwmin  = 2*n + n*n;
        lrwmin = 1 + 5*n + 2*n*n;
        liwmin = 3 + 5*n;
    }
    else {
        lwmin  = n + n*nb;
        lrwmin = n;
        liwmin = 1;
    }
    // multiply by 1+eps to ensure length gets rounded up,
    // if it cannot be exactly represented in floating point.
    work[0]  = MAGMA_C_MAKE( lwmin * (1. + lapackf77_slamch("Epsilon")), 0.);
    rwork[0] = lrwmin * (1. + lapackf77_slamch("Epsilon"));
    iwork[0] = liwmin;
    
    if ((lwork < lwmin) && !lquery) {
        *info = -8;
    } else if ((lrwork < lrwmin) && ! lquery) {
        *info = -10;
    } else if ((liwork < liwmin) && ! lquery) {
        *info = -12;
    }
    
    if (*info != 0) {
        magma_xerbla( __func__, -(*info) );
        return *info;
    }
    else if (lquery) {
        return *info;
    }
    
    /* Quick return if possible */
    if (n == 0) {
        return *info;
    }
    
    if (n == 1) {
        w[0] = MAGMA_C_REAL(a[0]);
        if (wantz) {
            a[0] = MAGMA_C_ONE;
        }
        return *info;
    }
    
    /* Get machine constants. */
    safmin = lapackf77_slamch("Safe minimum");
    eps    = lapackf77_slamch("Precision");
    smlnum = safmin / eps;
    bignum = 1. / smlnum;
    rmin = magma_ssqrt(smlnum);
    rmax = magma_ssqrt(bignum);
    
    /* Scale matrix to allowable range, if necessary. */
    anrm = lapackf77_clanhe("M", lapack_const(uplo_), &n, a, &lda, rwork);
    iscale = 0;
    if (anrm > 0. && anrm < rmin) {
        iscale = 1;
        sigma = rmin / anrm;
    } else if (anrm > rmax) {
        iscale = 1;
        sigma = rmax / anrm;
    }
    if (iscale == 1) {
        lapackf77_clascl(lapack_const(uplo_), &izero, &izero, &d_one, &sigma, &n, &n, a,
                         &lda, info);
    }
    
    /* Call CHETRD to reduce Hermitian matrix to tridiagonal form. */
    // chetrd rwork: e (n)
    // cstedx rwork: e (n) + llrwk (1 + 4*N + 2*N**2)  ==>  1 + 5n + 2n^2
    inde   = 0;
    indrwk = inde + n;
    llrwk  = lrwork - indrwk;
    
    // chetrd work: tau (n) + llwork (n*nb)  ==>  n + n*nb
    // cstedx work: tau (n) + z (n^2)
    // cunmtr work: tau (n) + z (n^2) + llwrk2 (n or n*nb)  ==>  2n + n^2, or n + n*nb + n^2
    indtau = 0;
    indwrk = indtau + n;
    indwk2 = indwrk + n*n;
    llwork = lwork - indwrk;
    llwrk2 = lwork - indwk2;
    
//#define ENABLE_TIMER
#ifdef ENABLE_TIMER
    magma_timestr_t start, end;
    start = get_current_time();
#endif
    
    magma_chetrd(lapack_const(uplo)[0], n, a, lda, w, &rwork[inde],
                 &work[indtau], &work[indwrk], llwork, &iinfo, queue);
    
#ifdef ENABLE_TIMER
    end = get_current_time();
    printf("time chetrd = %6.2f\n", GetTimerValue(start,end)/1000.);
#endif
    
    /* For eigenvalues only, call SSTERF.  For eigenvectors, first call
     CSTEDC to generate the eigenvector matrix, WORK(INDWRK), of the
     tridiagonal matrix, then call CUNMTR to multiply it to the Householder
     transformations represented as Householder vectors in A. */
    if (! wantz) {
        lapackf77_ssterf(&n, w, &rwork[inde], info);
    } else {
        
#ifdef ENABLE_TIMER
        start = get_current_time();
#endif
        
        if (MAGMA_SUCCESS != magma_smalloc( &dwork, (3*n*(n/2 + 1) ) )) {
            *info = MAGMA_ERR_DEVICE_ALLOC;
            return *info;
        }
        
        magma_cstedx(MagmaAllVec, n, 0., 0., 0, 0, w, &rwork[inde],
                     &work[indwrk], n, &rwork[indrwk],
                     llrwk, iwork, liwork, dwork, info, queue);
        
        magma_free( dwork );
        
#ifdef ENABLE_TIMER
        end = get_current_time();
        printf("time cstedx = %6.2f\n", GetTimerValue(start,end)/1000.);
        
        start = get_current_time();
#endif
        
        magma_cunmtr(MagmaLeft, uplo, MagmaNoTrans, n, n, a, lda, &work[indtau],
                     &work[indwrk], n, &work[indwk2], llwrk2, &iinfo, queue);
        
        lapackf77_clacpy("A", &n, &n, &work[indwrk], &n, a, &lda);
        
#ifdef ENABLE_TIMER
        end = get_current_time();
        printf("time cunmtr + copy = %6.2f\n", GetTimerValue(start,end)/1000.);
#endif
    }
    
    /* If matrix was scaled, then rescale eigenvalues appropriately. */
    if (iscale == 1) {
        if (*info == 0) {
            imax = n;
        } else {
            imax = *info - 1;
        }
        d__1 = 1. / sigma;
        blasf77_sscal(&imax, &d__1, w, &ione);
    }
    
    work[0]  = MAGMA_C_MAKE( lwmin * (1. + lapackf77_slamch("Epsilon")), 0.);  // round up
    rwork[0] = lrwmin * (1. + lapackf77_slamch("Epsilon"));
    iwork[0] = liwmin;
    
    return *info;
} /* magma_cheevd */
