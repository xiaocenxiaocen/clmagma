/*
    -- clMAGMA (version 0.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       April 2012
*/

#include "common_magma.h"

#if defined( _WIN32 ) || defined( _WIN64 )
#  include <time.h>
#  include <sys/timeb.h>
#  if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#    define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#  else
#    define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#  endif
#else
#  include <sys/time.h>
#endif

#if defined(ADD_)
#    define magma_gettime_f        magma_gettime_f_
#    define magma_gettimervalue_f  magma_gettimervalue_f_
#elif defined(NOCHANGE)
#endif


/* ////////////////////////////////////////////////////////////////////////////
   -- Used by chk() macro to print error message.
*/
void chk_helper( int err, const char* func, const char* file, int line )
{
    if ( err != 0 ) {
        printf( "error in %s at %s:%d: %s (%d)\n",
                func, file, line, magma_strerror(err), err );
    }
}


/* ////////////////////////////////////////////////////////////////////////////
   -- Get current time
*/ 
#if defined( _WIN32 ) || defined( _WIN64 )
struct timezone
{
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};

extern "C"
int gettimeofday(struct timeval* tv, struct timezone* tz)
{
    FILETIME         ft;
    unsigned __int64 tmpres = 0;
    static int       tzflag;

    if (NULL != tv) {
        GetSystemTimeAsFileTime(&ft);
        tmpres |=  ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |=  ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres /= 10;  /*convert into microseconds*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;

        tv->tv_sec  = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }
    if (NULL != tz) {
        if (!tzflag) {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime     = _daylight;
    }
    return 0;
}
#endif

extern "C"
magma_timestr_t get_current_time(void)
{
    static struct timeval time_val;

    magma_timestr_t time;

    //cudaDeviceSynchronize();
    gettimeofday(&time_val, NULL);

    time.sec  = time_val.tv_sec;
    time.usec = time_val.tv_usec;
    return (time);
}

extern "C"
void magma_gettime_f(unsigned int *time)
{
    magma_timestr_t tmp = get_current_time();
    time[0] = tmp.sec;
    time[1] = tmp.usec;
}

/* ////////////////////////////////////////////////////////////////////////////
   -- End elapsed time
*/ 
extern "C"
double GetTimerValue(magma_timestr_t time_1, magma_timestr_t time_2)
{
    int sec, usec;

    sec  = time_2.sec  - time_1.sec;
    usec = time_2.usec - time_1.usec;

    return (1000.*(double)(sec) + (double)(usec) * 0.001);
}

extern "C"
void magma_gettimervalue_f(unsigned int *start, unsigned int *end, double *result) {
    magma_timestr_t time1, time2;
    time1.sec  = start[0];
    time1.usec = start[1];
    time2.sec  = end[0];
    time2.usec = end[1];
    *result = GetTimerValue(time1, time2);
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Return time in seconds since arbitrary point (e.g., unix epoch).
*/
double get_time( void )
{
	struct timeval t;
	gettimeofday( &t, NULL );
	return t.tv_sec + t.tv_usec*1e-6;
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Print the available GPU devices
*/
extern "C"
void printout_devices( )
{
#if 0  // CUDA
    int ndevices;
    cuDeviceGetCount( &ndevices );
    for( int idevice = 0; idevice < ndevices; idevice++ ) {
        char name[200];
#if CUDA_VERSION > 3010 
        size_t totalMem;
#else
        unsigned int totalMem;
#endif

        int clock;
        int major, minor;
        CUdevice dev;

        cuDeviceGet( &dev, idevice );
        cuDeviceGetName( name, sizeof(name), dev );
        cuDeviceComputeCapability( &major, &minor, dev );
        cuDeviceTotalMem( &totalMem, dev );
        cuDeviceGetAttribute( &clock,
                              CU_DEVICE_ATTRIBUTE_CLOCK_RATE, dev );
        printf( "device %d: %s, %.1f MHz clock, %.1f MB memory, capability %d.%d\n",
                idevice, name, clock/1000.f, totalMem/1024.f/1024.f, major, minor );
    }
#endif // CUDA
}


/* ////////////////////////////////////////////////////////////////////////////
   -- If uplo is MagmaUpper, put 0s in the upper triangular part of a panel
      and 1s on the diagonal.
      If uplo is MagmaLower, put 0s in the lower triangular part of a panel
      and 1s on the diagonal.
      Save the original values in work, to be restored by q_to_panel.
      This is an auxiliary function used in geqrf and geqlf.  
*/
extern "C"
void spanel_to_q( int uplo, int ib, float *a, int lda, float *work )
{
  int i, j, k = 0;
  float *col;

  if ( uplo == MagmaUpper ){
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=0; j<i; j++){
        work[k++] = col[j];
        col[j] = 0.;
      }
      work[k++] = col[i];
      col[j] = 1.;
    }
  }
  else {
    for(i=0; i<ib; i++){
      col = a + i*lda;
      work[k++] = col[i];
      col[i] = 1.;
      for(j=i+1; j<ib; j++){
        work[k++] = col[j];
        col[j] = 0.;
      }
    }
  }
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Restores a panel after call to panel_to_q.
      This is an auxiliary function used in geqrf and geqlf.
*/
extern "C"
void sq_to_panel( int uplo, int ib, float *a, int lda, float *work )
{
  int i, j, k = 0;
  float *col;

  if ( uplo == MagmaUpper ){
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=0; j<=i; j++)
        col[j] = work[k++];
    }
  }
  else {
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=i; j<ib; j++)
        col[j] = work[k++];
    }
  }
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Put 0s in the upper triangular part of a panel (and 1s on the diagonal)
*/
extern "C"
void cpanel_to_q( int uplo, int ib, magmaFloatComplex *a, int lda, magmaFloatComplex *work )
{
  int i, j, k = 0;
  magmaFloatComplex *col;

  if ( uplo == MagmaUpper ){
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=0; j<i; j++){
        work[k  ].x = col[j].x;
        work[k++].y = col[j].y;
        col[j].x = col[j].y = 0.;
      }
      work[k  ].x = col[i].x;
      work[k++].y = col[i].y;
      col[j].x = 1.;
      col[j].y = 0.;
    }
  }
  else {
    for(i=0; i<ib; i++){
      col = a + i*lda;
      work[k  ].x = col[i].x;
      work[k++].y = col[i].y;
      col[i].x = 1.;
      col[i].y = 0.;
      for(j=i+1; j<ib; j++){
        work[k  ].x = col[j].x;
        work[k++].y = col[j].y;
        col[j].x = col[j].y = 0.;
      }
    }
  }
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Restores a panel (after call to "panel_to_q")
*/
extern "C"
void cq_to_panel( int uplo, int ib, magmaFloatComplex *a, int lda, magmaFloatComplex *work )
{
  int i, j, k = 0;
  magmaFloatComplex *col;

  if ( uplo == MagmaUpper ){
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=0; j<=i; j++){
        col[j].x = work[k  ].x;
        col[j].y = work[k++].y;
      }
    }
  }
  else {
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=i; j<ib; j++){
        col[j].x = work[k  ].x;
        col[j].y = work[k++].y;
      }
    }
  }
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Put 0s in the upper triangular part of a panel (and 1s on the diagonal)
*/
extern "C"
void dpanel_to_q( int uplo, int ib, double *a, int lda, double *work )
{
  int i, j, k = 0;
  double *col;

  if ( uplo == MagmaUpper ){
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=0; j<i; j++){
        work[k++] = col[j];
        col[j] = 0.;
      }
      work[k++] = col[i];
      col[j] = 1.;
    }
  }
  else {
    for(i=0; i<ib; i++){
      col = a + i*lda;
      work[k++] = col[i];
      col[i] = 1.;
      for(j=i+1; j<ib; j++){
        work[k++] = col[j];
        col[j] = 0.;
      }
    }
  } 
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Restores a panel (after call to "panel_to_q")
*/
extern "C"
void dq_to_panel( int uplo, int ib, double *a, int lda, double *work )
{
  int i, j, k = 0;
  double *col;

  if ( uplo == MagmaUpper ){
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=0; j<=i; j++)
        col[j] = work[k++];
    }
  }
  else {
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=i; j<ib; j++)
        col[j] = work[k++];
    }
  }
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Put 0s in the upper triangular part of a panel (and 1s on the diagonal)
*/
extern "C"
void zpanel_to_q( int uplo, int ib, magmaDoubleComplex *a, int lda, magmaDoubleComplex *work )
{
  int i, j, k = 0;
  magmaDoubleComplex *col;

  if ( uplo == MagmaUpper ){
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=0; j<i; j++){
        work[k  ].x = col[j].x;
        work[k++].y = col[j].y;
        col[j].x = col[j].y = 0.;
      }
      work[k  ].x = col[i].x;
      work[k++].y = col[i].y;
      col[j].x = 1.;
      col[j].y = 0.;
    }
  }
  else {
    for(i=0; i<ib; i++){
      col = a + i*lda;
      work[k  ].x = col[i].x;
      work[k++].y = col[i].y;
      col[i].x = 1.;
      col[i].y = 0.;
      for(j=i+1; j<ib; j++){
        work[k  ].x = col[j].x;
        work[k++].y = col[j].y;
        col[j].x = col[j].y = 0.;
      }
    }
  }
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Restores a panel (after call to "panel_to_q")
*/
extern "C"
void zq_to_panel( int uplo, int ib, magmaDoubleComplex *a, int lda, magmaDoubleComplex *work )
{
  int i, j, k = 0;
  magmaDoubleComplex *col;

  if ( uplo == MagmaUpper ){
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=0; j<=i; j++){
        col[j].x = work[k  ].x;
        col[j].y = work[k++].y;
      }
    }
  }
  else {
    for(i=0; i<ib; i++){
      col = a + i*lda;
      for(j=i; j<ib; j++){
        col[j].x = work[k  ].x;
        col[j].y = work[k++].y;
      }
    }
  }
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Auxiliary function: ipiv(i) indicates that row i has been swapped with 
      ipiv(i) from top to bottom. This function rearranges ipiv into newipiv
      where row i has to be moved to newipiv(i). The new pivoting allows for
      parallel processing vs the original one assumes a specific ordering and
      has to be done sequentially.
*/
extern "C"
void swp2pswp( int trans, int n, int *ipiv, int *newipiv){
  int i, newind, ind;

  for(i=0; i<n; i++)
    newipiv[i] = -1;
  
  if ( trans == MagmaNoTrans ){
    for(i=0; i<n; i++){
      newind = ipiv[i] - 1;
      if (newipiv[newind] == -1) {
        if (newipiv[i]==-1){
          newipiv[i] = newind;
          if (newind>i)
            newipiv[newind]= i;
        }
        else
          {
            ind = newipiv[i];
            newipiv[i] = newind;
            if (newind>i)
              newipiv[newind]= ind;
          }
      }
      else {
        if (newipiv[i]==-1){
          if (newind>i){
            ind = newipiv[newind];
            newipiv[newind] = i;
            newipiv[i] = ind;
          }
          else
            newipiv[i] = newipiv[newind];
        }
        else{
          ind = newipiv[i];
          newipiv[i] = newipiv[newind];
          if (newind > i)
            newipiv[newind] = ind;
        }
      }
    }
  } else {
    for(i=n-1; i>=0; i--){
      newind = ipiv[i] - 1;
      if (newipiv[newind] == -1) {
        if (newipiv[i]==-1){
          newipiv[i] = newind;
          if (newind>i)
            newipiv[newind]= i;
        }
        else
          {
            ind = newipiv[i];
            newipiv[i] = newind;
            if (newind>i)
              newipiv[newind]= ind;
          }
      }
      else {
        if (newipiv[i]==-1){
          if (newind>i){
            ind = newipiv[newind];
            newipiv[newind] = i;
            newipiv[i] = ind;
          }
          else
            newipiv[i] = newipiv[newind];
        }
        else{
          ind = newipiv[i];
          newipiv[i] = newipiv[newind];
          if (newind > i)
            newipiv[newind] = ind;
        }
      }
    }
  }
}

/* ////////////////////////////////////////////////////////////////////////////
   -- Auxiliary function sp_cat
*/
extern "C"
int sp_cat(char *lp, char *rpp[], magma_int_t *rnp, magma_int_t*np, magma_int_t ll)
{
  magma_int_t i, n, nc;
  char *f__rp;

  n = (int)*np;
  for(i = 0 ; i < n ; ++i)
    {
      nc = ll;
      if(rnp[i] < nc)
        nc = rnp[i];
      ll -= nc;
      f__rp = rpp[i];
      while(--nc >= 0)
        *lp++ = *f__rp++;
    }
  while(--ll >= 0)
    *lp++ = ' ';

  return 0;
}