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
#include <stdio.h>

#include "allHandles.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */
#include "port.h"


#define	sfb_offset(x) ( ((x) > 0) ? sfb_top[(x)-1] : 0 )


/* apply the TNS filter */
static void
tns_ma_filter( Float *spec, int size, int inc, Float *lpc, int order )
{
    /*
     *	- Simple all-zero filter of order "order" defined by
     *	  y(n) =  x(n) + a(2)*x(n-1) + ... + a(order+1)*x(n-order)
     *
     *	- The state variables of the filter are initialized to zero every time
     *
     *	- The output data is written over the input data ("in-place operation")
     *
     *	- An input vector of "size" samples is processed and the index increment
     *	  to the next data sample is given by "inc"
     */
    int i, j;
    Float y, state[TNS_MAX_ORDER];

    for (i=0; i<order; i++)
	state[i] = 0;

    if (inc == -1)
	spec += size-1;

    for (i=0; i<size; i++) {
	y = *spec;
	for (j=0; j<order; j++)
	    y += lpc[j+1] * state[j];
	for (j=order-1; j>0; j--)
	    state[j] = state[j-1];
	state[0] = *spec;
	*spec = y;
	spec += inc;
    }
}


 
/* TNS decoding for one channel and frame */
void
tns_encode_subblock(Float *spec, int nbands, const short *sfb_top, int islong,
	TNSinfo *tns_info, QC_MOD_SELECT qc_select,int samplRateIdx)
{
  int f, m, start, stop, size, inc;
  int n_filt, coef_res, order, direction;
  short *coef;
  int tmp;
  Float lpc[TNS_MAX_ORDER+1];
  TNSfilt *filt;
  TNS_COMPLEXITY profile = get_tns_complexity(qc_select);

  n_filt = tns_info->n_filt;
  for (f=0; f<n_filt; f++)  {
    coef_res = tns_info->coef_res;
    filt = &tns_info->filt[f];
    order = filt->order;
    direction = filt->direction;
    coef = filt->coef;
    start = filt->start_band;
    stop = filt->stop_band;

    m = tns_max_order(islong,profile,samplRateIdx);
    if (order > m) {
      fprintf(stderr, "Error in tns max order: %d %d\n", order, m);
      order = m;
    }
    if (!order)  continue;

    tns_decode_coef( order, coef_res, coef, lpc );
        
    tmp   = tns_max_bands ( islong,profile,samplRateIdx);
    start = MIN(start, tmp );
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

    tns_ma_filter( &spec[start], size, inc, lpc, order );
  }
}

