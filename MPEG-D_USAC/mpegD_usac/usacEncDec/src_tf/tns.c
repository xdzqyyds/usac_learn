/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of
development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7,
14496-1,2 and 3. This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio
standards free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing patents.
The original developer of this software module and his/her company, the subsequent
editors and their companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not released for
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the
code to a third party and to inhibit third party from using the code for non
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works."
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#include <math.h>
#include <stdio.h>

#include "allHandles.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */
#include "allVariables.h"

#include "port.h"

#define	sfb_offset(x) ( ((x) > 0) ? sfb_top[(x)-1] : 0 )

/* Decoder transmitted coefficients for one TNS filter */
void
tns_decode_coef( int order, int coef_res, short *coef, double *a )
{
    int i, m;
    double iqfac,  iqfac_m;
    double tmp[TNS_MAX_ORDER+1], b[TNS_MAX_ORDER+1];

    /* Inverse quantization */
    iqfac   = ((1 << (coef_res-1)) - 0.5) / (C_PI/2.0);
    iqfac_m = ((1 << (coef_res-1)) + 0.5) / (C_PI/2.0);
    for (i=0; i<order; i++)  {
	tmp[i+1] = sin( coef[i] / ((coef[i] >= 0) ? iqfac : iqfac_m) );
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
tns_ar_filter( double *spec, int size, int inc, double *lpc, int order )
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
    double y, state[TNS_MAX_ORDER];

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



TNS_COMPLEXITY get_tns_complexity(QC_MOD_SELECT qc_select)
{
  switch (qc_select) {
  case AAC_LD:
    if (only_long_info.bins_per_bk == 480) return TNS_LD_480;
    else if (only_long_info.bins_per_bk == 512) return TNS_LD_512;
  case AAC_SCALABLE:
  case NTT_VQ:
    return TNS_SCAL;
  case AAC_SCA_BSAC:
  case AAC_BSAC:
    return TNS_BSAC;
  case AAC_QC:
    switch (mc_info.profile) {
    case Main_Profile:
      return TNS_MAIN;
    case LC_Profile:
      return TNS_LC;
    case SSR_Profile:
      return TNS_SSR;
    case LTP_Profile:
      return TNS_SCAL;
    }
    case USAC_AC:
      return TNS_USAC;
  default:
    fprintf(stderr, "get_tns_complexity: case not handled in switch (%i)\n",qc_select);
  }
  return NO_TNS;
}

/* TNS decoding for one channel and frame */
void
tns_decode_subblock(double *spec, int nbands, const short *sfb_top, int islong,
	TNSinfo *tns_info, QC_MOD_SELECT qc_select,int samplFreqIdx)
{
    int f, m, start, stop, size, inc;
    int n_filt, coef_res, order, direction;
    short *coef;
    double lpc[TNS_MAX_ORDER+1];
    TNSfilt *filt;
    TNS_COMPLEXITY profile = get_tns_complexity(qc_select);

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

      tns_decode_coef( order, coef_res, coef, lpc );
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

	tns_ar_filter( &spec[start], size, inc, lpc, order );
  }
}

void
print_tns( TNSinfo *tns_info)
{
    int           f, t;
    static char   *s = "TNS>> ";

    fprintf(stderr, "%s n_filt: %d\n", s, tns_info -> n_filt );
    if (tns_info->n_filt)
	fprintf(stderr, "%s res   : %d\n", s, tns_info -> coef_res );
    for (f=0; f<tns_info->n_filt; f++)  {

	fprintf(stderr, "%s filt %d: [%d %d] o=%d", s, f,
	    tns_info->filt[f].start_band,
	    tns_info->filt[f].stop_band,
	    tns_info->filt[f].order );

	if (tns_info->filt[f].order)  {
	    fprintf(stderr, " d=%d | " ,tns_info->filt[f].direction  );

	    for (t=0; t<tns_info->filt[f].order; t++)
		fprintf(stderr, "%d ", tns_info->filt[f].coef[t] );

	}
	fprintf(stderr, "\n" );
    }
    fprintf(stderr, "%s ------------\n", s );
}
