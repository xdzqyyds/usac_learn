/*******************************************************************************
This software module was originally developed by

AT&T, Dolby Laboratories, Fraunhofer IIS, and VoiceAge Corp.

Initial author:

and edited by
Markus Werner       (SEED / Software Development Karlsruhe)

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: usac_tns.c,v 1.2 2009-11-13 08:45:53 mul Exp $
*******************************************************************************/

#include <math.h>
#include <stdio.h>

#include "allHandles.h"

#include "all.h"                 /* structs */
#include "tns.h"                 /* structs */
#include "allVariables.h"

#include "port.h"
#include "usac_main.h"

#define	sfb_offset(x) ( ((x) > 0) ? sfb_top[(x)-1] : 0 )

/* Decoder transmitted coefficients for one TNS filter */
static void
usac_tns_decode_coef( int order, int coef_res, short *coef, float *a )
{
    int i, m;
    float iqfac,  iqfac_m;
    float tmp[TNS_MAX_ORDER+1], b[TNS_MAX_ORDER+1];
    const float pi2 = 3.14159265358979323846F / 2.0F;
    /* Inverse quantization */
    iqfac   = ((1 << (coef_res-1)) - 0.5) / (pi2);
    iqfac_m = ((1 << (coef_res-1)) + 0.5) / (pi2);
    for (i=0; i<order; i++)  {
	tmp[i+1] = (float) sin( coef[i] / ((coef[i] >= 0) ? iqfac : iqfac_m) );
    }
    /* Conversion to LPC coefficients
     * Markel and Gray,  pg. 95
     */
    a[0] = 1;
    for (m=1; m<=order; m++)  {
	b[0] = a[0];
	for (i=1; i<m; i++)  {
	    b[i] = a[i] + tmp[m] * a[m-i];
	}
	b[m] = tmp[m];
	for (i=0; i<=m; i++)  {
	    a[i] = b[i];
	}
    }

}

/* apply the TNS filter */
static void
tns_ar_filter( float *spec, int size, int inc, float *lpc, int order )
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
    float y, state[TNS_MAX_ORDER];

    for (i=0; i<order; i++)
	state[i] = 0;

    if (inc == -1)
      spec += size-1;

    for (i=0; i<size; i++) {
      y = *spec;
      for (j=0; j<order; j++)
        y -= lpc[j+1] * state[j];
      for (j=order-1; j>0; j--)
        state[j] = state[j-1];
      state[0] = y;
      *spec = y;
      spec += inc;
	}
}




/* TNS decoding for one channel and frame */
void
usac_tns_decode_subblock(float *spec, int nbands, const short *sfb_top, int islong,
                         TNSinfo *tns_info, QC_MOD_SELECT qc_select,int samplFreqIdx)
{
  int f, m, start, stop, size, inc;
  int n_filt, coef_res, order, direction;
  short *coef;
  float tmpBuf[1024];
  float lpc[TNS_MAX_ORDER+1];
  TNSfilt *filt;
  TNS_COMPLEXITY profile = get_tns_complexity(qc_select);
  int nbins = islong ? 1024 : 128;
  int i;

  for ( i = 0 ; i < nbins ; i++) {
    tmpBuf[i] =  spec[i];
  }

  n_filt = tns_info->n_filt;
  for (f=0; f<n_filt; f++)  {
    int tmp;
    coef_res = tns_info->coef_res;
    filt = &tns_info->filt[f];
    order = filt->order;
    direction = filt->direction;
    coef = filt->coef;
    start = filt->start_band;
    stop = filt->stop_band;

    m = tns_max_order(islong,profile,samplFreqIdx);
    if (order > m) {
      fprintf(stderr, "Error in tns max order: %d %d\n", order, m);
    }
    if (!order)  continue;

    usac_tns_decode_coef( order, coef_res, coef, lpc );
    tmp = tns_max_bands(islong,profile,samplFreqIdx);

    start = MIN ( start, tmp );

    start = MIN(start, nbands);
    start = sfb_offset( start );

    stop  = MIN ( stop, tmp );
    stop = MIN(stop, nbands);
    stop = sfb_offset( stop );
    if ((size = stop - start) <= 0)  continue;

    if (direction)  {
      inc = -1;
    }  else {
      inc =  1;
    }

    tns_ar_filter( &tmpBuf[start], size, inc, lpc, order );
  }

  for ( i = 0 ; i < nbins ; i++) {
    spec[i] =  tmpBuf[i];
  }

}


