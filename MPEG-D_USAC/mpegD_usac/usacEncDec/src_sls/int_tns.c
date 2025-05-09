#include <assert.h>

#include "all.h"

#include "stdinc.h"
#include "int_mdct_defs.h"
#include "port.h"
#include "int_tns.h"

#define SHIFT_INTTNS 21


#ifndef min
#define min(x, y)   ( ((x) < (y) ) ? (x) : (y) )
#endif

#define	sfb_offset(x) ( ((x) > 0) ? sfb_top[(x)-1] : 0 )



/* INTEGER-TNS: */


static int tnsInvQuantCoefFixedPoint(int coef_res, int coef) 
{
  int intTnsCoef_res3[8] = {-2065292, -1816187, -1348023, -717268, 
                            0, 909920, 1639620, 2044572}; /* [-4,...,3] */
  int intTnsCoef_res4[16] = {-2088206, -2017095, -1877294, -1673563, 
                             -1412842, -1104008, -757579, -385351, 
                             0, 436022, 852989, 1232675, 
                             1558488, 1816187, 1994510, 2085664}; /* [-8,...,7] */

  if (coef_res == 3) {
    return intTnsCoef_res3[4+coef];
  }

  if (coef_res == 4) {
    return intTnsCoef_res4[8+coef];
  }

  assert(0);  /* mh: never shall get there */
  return 0;
}

/* Decoder transmitted coefficients for one TNS filter */
static void
int_tns_decode_coef( int order, int coef_res, short *coef, int *a )
{
  int i, m;
  int tmp[TNS_MAX_ORDER+1], b[TNS_MAX_ORDER+1];


  /* Inverse quantization */
  for (i=0; i<order; i++)  {
    tmp[i+1] = tnsInvQuantCoefFixedPoint(coef_res, coef[i]);
  }
  /* Conversion to LPC coefficients
   * Markel and Gray,  pg. 95
   */

  /* worst case for order == 12 and all coefficients == 1: 
     6th coefficient raised by 12!/(6!*6!) = 924 
     -> 10 bits headroom required */

  a[0] = 1<<SHIFT_INTTNS;
  for (m=1; m<=order; m++)  {
    b[0] = a[0];
    for (i=1; i<m; i++)  {
	    b[i] = a[i] + ((((((INT64)tmp[m])*a[m-i])>>(SHIFT_INTTNS-1))+1)>>1);
    }
    b[m] = tmp[m];
    for (i=0; i<=m; i++)  {
	    a[i] = b[i];
    }
  }  
}


static void int_tns_ar_filter(int *spec, int size, int inc, int *lpc, int order )
{
  /*
   *	- Simple all-pole filter of order "order" defined by
   *	  y(n) =  x(n) - a(2)*y(n-1) - ... - a(order+1)*y(n-order)
   *
   *	- The state variables of the filter are initialized to zero every time
   *
   *	- The output data is written over the input data ("in-place operation")
   *
   *	- An input vector of "size" samples is processed and the index increment
   *	  to the next data sample is given by "inc"
   */
  int i, j;
  int y, state[TNS_MAX_ORDER];
  INT64 temp_accu;

  for (i=0; i<order; i++)
    state[i] = 0;

  if (inc == -1)
    spec += size-1;

  for (i=0; i<size; i++) {
    y = *spec;
    temp_accu = 0;
    for (j=0; j<order; j++) {
      temp_accu += ((INT64)lpc[j+1]) * state[j];
    }
    y -= (int)( ( ( temp_accu >> (SHIFT_INTTNS-1) ) + 1) >> 1 );
    for (j=order-1; j>0; j--)
	    state[j] = state[j-1];
    state[0] = y;
    *spec = y;
    spec += inc;
  }
}

/* TNS decoding for one channel and frame */
void int_tns_decode_subblock(int *spec, int nbands, short *sfb_top, int islong,
                             TNSinfo *tns_info)
{
  int f, m, start, stop, size, inc;
  int n_filt, coef_res, order, direction;
  short *coef;
  int lpc[TNS_MAX_ORDER+1];
  TNSfilt *filt;

  n_filt = tns_info->n_filt;
  for (f=0; f<n_filt; f++)  {
    coef_res = tns_info->coef_res;
    filt = &tns_info->filt[f];
    order = filt->order;
    direction = filt->direction;
    coef = filt->coef;
    start = filt->start_band;
    stop = filt->stop_band;
    
    m = tns_max_order(islong, TNS_LC, 0 /*not necessary*/);
    if (order > m) {
      PRINT(SE, "Error in tns max order: %d %d\n",
            order, tns_max_order(islong, TNS_LC, 0 /*not necessary*/));
      order = m;
    }
    if (!order)  continue;

    int_tns_decode_coef( order, coef_res, coef, lpc );

    start = min(start, tns_max_bands(islong, TNS_LC, 0 /*not necessary*/));
    start = min(start, nbands);
    start = sfb_offset( start );
    
    stop = min(stop, tns_max_bands(islong, TNS_LC, 0 /*not necessary*/));
    stop = min(stop, nbands);
    stop = sfb_offset( stop );
    if ((size = stop - start) <= 0)  continue;
    
    if (direction)  {
	    inc = -1;
    }  else {
	    inc =  1;
    }
    
    int_tns_ar_filter( &spec[start], size, inc, lpc, order );
  }
}


/********************************************************/
/* TnsInvFilter:                                        */
/*   Inverse filter the given spec with specified       */
/*   length using the coefficients specified in filter. */
/*   Not that the order and direction are specified     */
/*   withing the TNS_FILTER_DATA structure.             */
/********************************************************/
static
void int_TnsInvFilter(int length, int *spec, TNSfilt *filter, int *coef) 
{
  int i,j,k = 0;
  int order = filter->order;
  int *a = coef;
  int *temp;
  INT64 temp_accu;

  temp = (int *) malloc (length * sizeof (int));
  if (!temp)
    /* myexit("TnsInvFilter: Could not allocate memory for TNS array\nExiting program..."); */
    exit(-1);
  
  /* Determine loop parameters for given direction */
  if (filter->direction) 
  {
    /* Startup, initial state is zero */
    temp[length-1]=spec[length-1];
    for (i=length-2;i>(length-1-order);i--) {
      temp[i]=spec[i];
      temp_accu = 0;
      k++;
      for (j=1;j<=k;j++) {
        temp_accu += ((INT64)temp[i+j]) * a[j];
      }
      spec[i] += (int)( ( ( temp_accu >> (SHIFT_INTTNS-1) ) + 1) >> 1 );
    }
    
    /* Now filter the rest */
    for	(i=length-1-order;i>=0;i--) {
      temp[i]=spec[i];
      temp_accu = 0;
      for (j=1;j<=order;j++) {
        temp_accu += ((INT64)temp[i+j]) * a[j]; 
      }
      spec[i] += (int)( ( ( temp_accu >> (SHIFT_INTTNS-1) ) + 1) >> 1 );
    }
    
    
  } else {
    /* Startup, initial state is zero */
    temp[0]=spec[0];
    for (i=1;i<order;i++) {
      temp[i]=spec[i];
      temp_accu = 0;
      for (j=1;j<=i;j++) {
        temp_accu += ((INT64)temp[i-j]) * a[j];
      }
      spec[i] += (int)( ( ( temp_accu >> (SHIFT_INTTNS-1) ) + 1) >> 1 );
    }
    
    /* Now filter the rest */
    for	(i=order;i<length;i++) {
      temp[i]=spec[i];
      temp_accu = 0;
      for (j=1;j<=order;j++) {
        temp_accu += ((INT64)temp[i-j])*a[j];
      }
      spec[i] += (int)( ( ( temp_accu >> (SHIFT_INTTNS-1) ) + 1) >> 1 );
    }
  }
  
  free(temp);
}


