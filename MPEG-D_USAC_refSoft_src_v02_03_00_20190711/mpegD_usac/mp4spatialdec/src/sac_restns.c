/*******************************************************************************
This software module was originally developed by

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips

and edited by

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

Agere Systems, Coding Technologies, Fraunhofer IIS, Philips retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/



#include "sac_resintrins.h"
#include "sac_bitinput.h" 
#include "sac_restns.h"

#ifndef min
#define min(x, y)   ( ((x) < (y) ) ? (x) : (y) )
#endif

#define	sfb_offset(x) ( ((x) > 0) ? sfb_top[(x)-1] : 0 )


void
s_tns_decode_coef( int order, int coef_res, short *coef, Float *a )
{
    int i, m;
    Float iqfac,  iqfac_m;
    Float tmp[TNS_MAX_ORDER+1], b[TNS_MAX_ORDER+1];

    
    iqfac   = ((1 << (coef_res-1)) - 0.5) / (C_PI/2.0);
    iqfac_m = ((1 << (coef_res-1)) + 0.5) / (C_PI/2.0);
    for (i=0; i<order; i++)  {
	tmp[i+1] = sin( coef[i] / ((coef[i] >= 0) ? iqfac : iqfac_m) );
    }
    
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


void
s_tns_ar_filter( Float *spec, int size, int inc, Float *lpc, int order )
{
    
    int i, j;
    Float y, state[TNS_MAX_ORDER];

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


void
s_tns_decode_subblock(Float *spec, int nbands, short *sfb_top, int islong,
	TNSinfo *tns_info)
{
    int f, m, start, stop, size, inc;
    int n_filt, coef_res, order, direction;
    short *coef;
    Float lpc[TNS_MAX_ORDER+1];
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

	m = s_tns_max_order(islong);
	if (order > m) {
	    PRINT(SE, "Error in tns max order: %d %d\n",
		order, s_tns_max_order(islong));
	    order = m;
	}
	if (!order)  continue;

	s_tns_decode_coef( order, coef_res, coef, lpc );

	start = min(start, s_tns_max_bands(islong));
        start = min(start, nbands);
	start = sfb_offset( start );
	
	stop = min(stop, s_tns_max_bands(islong));
        stop = min(stop, nbands);
	stop = sfb_offset( stop );
	if ((size = stop - start) <= 0)  continue;

	if (direction)  {
	    inc = -1;
	}  else {
	    inc =  1;
	}

	s_tns_ar_filter( &spec[start], size, inc, lpc, order );
    }
}









static
void TnsInvFilter(int length, Float *spec, TNSfilt *filter, Float *coef) 
{
  int i,j,k = 0;
  int order = filter->order;
  Float *a = coef;
  Float *temp;
  
  temp = (Float *) s_mal1 (length * sizeof (Float));
  
  
  if (filter->direction) 
  {
    
    temp[length-1]=spec[length-1];
    for (i=length-2;i>(length-1-order);i--) {
      temp[i]=spec[i];
      k++;
      for (j=1;j<=k;j++) {
	spec[i]+=temp[i+j]*a[j];
      }
    }
    
    
    for	(i=length-1-order;i>=0;i--) {
      temp[i]=spec[i];
      for (j=1;j<=order;j++) {
	spec[i]+=temp[i+j]*a[j];
      }
    }
    
    
  } else {
    
    temp[0]=spec[0];
    for (i=1;i<order;i++) {
      temp[i]=spec[i];
      for (j=1;j<=i;j++) {
	spec[i]+=temp[i-j]*a[j];
      }
    }
    
    
    for	(i=order;i<length;i++) {
      temp[i]=spec[i];
      for (j=1;j<=order;j++) {
	spec[i]+=temp[i-j]*a[j];
      }
    }
  }
  
  s_free1(temp);
}


void
s_tns_filter_subblock(Float *spec, int nbands, short *sfb_top, int islong,
		    TNSinfo *tns_info)
{
  int f, m, start, stop, size;
  int n_filt, coef_res, order;
  short *coef;
  Float lpc[TNS_MAX_ORDER+1];
  TNSfilt *filt;
  
  n_filt = tns_info->n_filt;
  for (f=0; f<n_filt; f++)  
  {
    coef_res = tns_info->coef_res;
    filt = &tns_info->filt[f];
    order = filt->order;
    coef = filt->coef;
    start = filt->start_band;
    stop = filt->stop_band;
    
    m = s_tns_max_order(islong);
    if (order > m)
      order = m;
    
    if (!order)  continue;
    
    s_tns_decode_coef( order, coef_res, coef, lpc );
    
    start = min(start, s_tns_max_bands(islong));
    start = min(start, nbands);
    start = sfb_offset( start );
    
    stop = min(stop, s_tns_max_bands(islong));
    stop = min(stop, nbands);
    stop = sfb_offset( stop );
    if ((size = stop - start) <= 0)  
      continue;
 
    TnsInvFilter(size, spec + start, filt, lpc);  
  }
}


void
s_clr_tns( Info *info, TNS_frame_info *tns_frame_info )
{
  int s;

  tns_frame_info->n_subblocks = info->nsbk;
  for (s=0; s<tns_frame_info->n_subblocks; s++)
    tns_frame_info->info[s].n_filt = 0;
}

int
s_get_tns( Info *info, TNS_frame_info *tns_frame_info )
{
  int                       f, t, top, res, res2, compress;
  int                       short_flag, s;
  short                     *sp, tmp, s_mask, n_mask;
  TNSfilt                   *tns_filt;
  TNSinfo                   *tns_info;
  static short              sgn_mask[] = { 
    0x2, 0x4, 0x8     };
  static unsigned short              neg_mask[] = { 
    0xfffc, 0xfff8, 0xfff0     };


  short_flag = (!info->islong);
  tns_frame_info->n_subblocks = info->nsbk;

  for (s=0; s<tns_frame_info->n_subblocks; s++) {
    tns_info = &tns_frame_info->info[s];

    if (!(tns_info->n_filt = s_getbits( short_flag ? 1 : 2 )))
      continue;
	    
    tns_info -> coef_res = res = s_getbits( 1 ) + 3;
    top = info->sfb_per_sbk[s];
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  {
      tns_filt->stop_band = top;
      top = tns_filt->start_band = top - s_getbits( short_flag ? 4 : 6 );
      tns_filt->order = s_getbits( short_flag ? 3 : 5 );

      if (tns_filt->order)  {
        tns_filt->direction = s_getbits( 1 );
        compress = s_getbits( 1 );

        res2 = res - compress;
        s_mask = sgn_mask[ res2 - 2 ];
        n_mask = neg_mask[ res2 - 2 ];

        sp = tns_filt -> coef;
        for (t=tns_filt->order; t>0; t--)  {
          tmp = (short) s_getbits( res2 );
          *sp++ = (tmp & s_mask) ? (tmp | n_mask) : tmp;
        }
      }
      tns_filt++;
    }
  }   
  return 1;
}


void
s_print_tns( TNSinfo *tns_info)
{
    int           f, t;
    static char   *s = "TNS>> ";

    PRINT(SE, "%s n_filt: %d\n", s, tns_info -> n_filt );
    if (tns_info->n_filt)
	PRINT(SE, "%s res   : %d\n", s, tns_info -> coef_res );
    for (f=0; f<tns_info->n_filt; f++)  {

	PRINT(SE, "%s filt %d: [%d %d] o=%d", s, f, 
	    tns_info->filt[f].start_band,
	    tns_info->filt[f].stop_band,
	    tns_info->filt[f].order );

	if (tns_info->filt[f].order)  {
	    PRINT(SE, " d=%d | " ,tns_info->filt[f].direction  );

	    for (t=0; t<tns_info->filt[f].order; t++)
		PRINT(SE, "%d ", tns_info->filt[f].coef[t] );

	}
	PRINT(SE, "\n" );
    }
    PRINT(SE, "%s ------------\n", s );
}
