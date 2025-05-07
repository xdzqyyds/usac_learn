/**********************************************************************
  This software module was originally developed by
  Bernhard Grill (University of Erlangen)
  and edited by
  Huseyin Kemal Cakmak (Fraunhofer Gesellschaft IIS)
  and edited by
  Bernhard Grill (University of Erlangen),
  Takashi Koike (Sony Corporation)
  Naoki Iwakami (Nippon Telegraph and Telephone)
  Olaf Kaehler (Fraunhofer IIS-A)
  Jeremie Lecomte (Fraunhofer IIS-A)

  FN3 LN3 (CN3),
  in the course of development of the
  MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
  This software module is an implementation of a part of one or more
  MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio
  standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards
  free license to this software module or modifications thereof for use in
  hardware or software products claiming conformance to the MPEG-2 AAC/
  MPEG-4 Audio  standards. Those intending to use this software module in
  hardware or software products are advised that this use may infringe
  existing patents. The original developer of this software module and
  his/her company, the subsequent editors and their companies, and ISO/IEC
  have no liability for use of this software module or modifications
  thereof in an implementation. Copyright is not released for non
  MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer
  retains full right to use the code for his/her  own purpose, assign or
  donate the code to a third party and to inhibit third party from using
  the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.
  This copyright notice must be included in all copies or derivative works.
  Copyright (c)1996. 
  **********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allHandles.h"
#include "block.h"

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */

#include "allVariables.h"        /* variables */

#ifdef AAC_ELD
#include "win512LD.h"
#include "win480LD.h"
#endif

#include "common_m4a.h"
#include "tf_main.h"
#include "transfo.h"
#include "dolby_win.h"
#include "dolby_win_ssr.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif


static const double zero = 0;

static void vcopy( const double src[], double dest[], int inc_src, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen-1; i++ ) {
    *dest = *src;
    dest += inc_dest;
    src  += inc_src;
  }
  if (vlen) /* just for bounds-checkers sake */
    *dest = *src;

}

static void vmult( const double src1[], const double src2[], double dest[], 
            int inc_src1, int inc_src2, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen-1; i++ ) {
    *dest = *src1 * *src2;
    dest += inc_dest;
    src1 += inc_src1;
    src2 += inc_src2;
  }
  if (i<vlen)
    *dest = *src1 * *src2;

}

static void vadd( const double src1[], const double src2[], double dest[], 
            int inc_src1, int inc_src2, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen; i++ ) {
    *dest = *src1 + *src2;
    dest += inc_dest;
    src1 += inc_src1;
    src2 += inc_src2;
  }
}
 
/*****************************************************************************

    functionname: Izero
    description:  calculates the modified Bessel function of the first kind
    returns:      value of Bessel function
    input:        argument
    output:       

*****************************************************************************/
static double Izero(double x)
{
  const double IzeroEPSILON = 1E-41;  /* Max error acceptable in Izero */
  double sum, u, halfx, temp;
  int n;
  
  sum = u = n = 1;
  halfx = x/2.0;
  do {
    temp = halfx/(double)n;
    n += 1;
    temp *= temp;
    u *= temp;
    sum += u;
  } while (u >= IzeroEPSILON*sum);

  return(sum);
}



/*****************************************************************************

    functionname: CalculateKBDWindow
    description:  calculates the window coefficients for the Kaiser-Bessel
                  derived window
    returns:        
    input:        window length, alpha
    output:       window coefficients

*****************************************************************************/
static void CalculateKBDWindow(double* win, double alpha, int length)
{
  int i;
  double IBeta;
  double tmp;
  double sum = 0.0;

  alpha *= M_PI;
  IBeta = 1.0/Izero(alpha);
 
  /* calculate lower half of Kaiser Bessel window */
  for(i=0; i<(length>>1); i++) {
    tmp = 4.0*(double)i/(double)length - 1.0;
    win[i] = Izero(alpha*sqrt(1.0-tmp*tmp))*IBeta;
    sum += win[i];
  }

  sum = 1.0/sum;
  tmp = 0.0;

  /* calculate lower half of window */
  for(i=0; i<(length>>1); i++) {
    tmp += win[i];
    win[i] = sqrt(tmp*sum);
  }
}

/* Calculate window */
static void calc_window( double window[], int len, WINDOW_SHAPE wfun_select)

{
  int i;
  
  switch(wfun_select) {
  case WS_FHG: 
    for( i=0; i<len; i++ ) 
      window[i] = sin( ((i+1)-0.5) * M_PI / (2*len) );
    break;
#ifdef AAC_ELD
  case WS_FHG_LDFB:
      /* no nothing use coeffitients directly */
    break;
#endif
  case WS_DOLBY: 
    switch(len)
      {
      case BLOCK_LEN_SHORT_S:
        for( i=0; i<len; i++ ) 
          window[i] = dolby_win_120[i]; 
        break;
      case BLOCK_LEN_SHORT:
        memcpy(window, dolby_win_128, len*sizeof(double));
        break;
      case BLOCK_LEN_SHORT_SSR:
        for( i=0; i<len; i++ ) 
          window[i] = dolby_win_32[i]; 
        break;
      case BLOCK_LEN_LONG_S:
        for( i=0; i<len; i++ ) 
          window[i] = dolby_win_960[i]; 
        break;
      case BLOCK_LEN_LONG:
        memcpy(window, dolby_win_1024, len*sizeof(double));
        break;
      case 64:
          for( i=0; i<len; i++ )
             window[i] = ShortWindowSine64[i];
          break;
      case BLOCK_LEN_LONG_SSR:
        for( i=0; i<len; i++ ) 
          window[i] = dolby_win_256[i]; 
        break;
      case 4:
        memcpy(window, dolby_win_4, len*sizeof(double));
        break;
      case 16:
        memcpy(window, dolby_win_16, len*sizeof(double));
        break;
      default:
        CalculateKBDWindow(window, 6.0, 2*len);
        CommonWarning("strange window size %d",len);
        break;
      }
    break;

    /* Zero padded window for low delay mode */
  case WS_ZPW:
    for( i=0; i<3*(len>>3); i++ )
      window[i] = 0.0;
    for(; i<5*(len>>3); i++)
      window[i] = sin((i-3*(len>>3)+0.5) * M_PI / (len>>1));
    for(; i<len; i++)
      window[i] = 1.0;
    break; 

  default:
    CommonExit(1,"Unsupported window shape: %d", wfun_select);
    break;
  }
}


/* %%%%%%%%%%%%%%%%%% IMDCT - STUFF %%%%%%%%%%%%%%%%*/

#define MAX_SHIFT_LEN_LONG 4096

#ifdef AAC_ELD
static void ildfb(double in_data[], double out_data[], int len)
{
    vcopy(in_data, out_data, 1, 1, len/2);
    LDFB(out_data, len, len/2, -1, -1);
}
#endif

void imdct(double in_data[], double out_data[], int len)
{
  vcopy(in_data, out_data, 1, 1, len/2);
  MDCT(out_data, len, len/2, -1);
}

void freq2buffer(
  double           p_in_data[], 
  double           p_out_data[],
  double           p_overlap[],
  WINDOW_SEQUENCE  windowSequence,
  int              nlong,            /* shift length for long windows   */
  int              nshort,           /* shift length for short windows  */
  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
  WINDOW_SHAPE     wfun_select_prev, /* YB : 971113 */
  Imdct_out	   overlap_select,   /* select imdct output *TK*	*/
				     /* switch (overlap_select) {	*/
				     /* case OVERLAPPED_MODE:		*/
				     /*   p_out_data[]			*/
				     /*   = overlapped and added signal */
				     /*		(bufferlength: nlong)	*/
				     /* case NON_OVERLAPPED_MODE:		*/
				     /*   p_out_data[]			*/
				     /*   = non overlapped signal	*/
				     /*		(bufferlength: 2*nlong)	*/
  int              num_short_win     /* number of short windows to      */
                                     /* transform                       */
  )
{
  double           *o_buf, transf_buf[ 2*MAX_SHIFT_LEN_LONG ]={0.0};
  double           overlap_buf[ 2*MAX_SHIFT_LEN_LONG ] = {0.0}; 
 
  double           window_long[MAX_SHIFT_LEN_LONG]; 
  double           window_long_prev[MAX_SHIFT_LEN_LONG]; 
  double           window_short[MAX_SHIFT_LEN_LONG]; 
  double           window_short_prev[MAX_SHIFT_LEN_LONG]; 
  double           *window_short_prev_ptr;
 
  double  *fp; 
  int      k; 
  int      overlap_fac = 1;
  int nflat_ls    = (nlong-nshort)/ 2; 
  int transfak_ls =  nlong/nshort; 

  window_short_prev_ptr=window_short_prev ; 

#ifdef AAC_ELD
  if (wfun_select == WS_FHG_LDFB) overlap_fac = 3;
#endif

  if( (nlong%nshort) || (nlong > MAX_SHIFT_LEN_LONG) || (nshort > MAX_SHIFT_LEN_LONG/2) ) { 
    CommonExit( 1, "mdct_synthesis: Problem with window length" ); 
  } 
  if( windowSequence==EIGHT_SHORT_SEQUENCE
      && ( (num_short_win <= 0) || (num_short_win > transfak_ls) ) ) {
    CommonExit( 1, "mdct_synthesis: Problem with number of short windows" ); 
  } 

  calc_window( window_long,      nlong, wfun_select ); 
  calc_window( window_long_prev, nlong, wfun_select_prev ); 
  calc_window( window_short,      nshort, wfun_select ); 
  calc_window( window_short_prev_ptr, nshort, wfun_select_prev ); 

  /* Assemble overlap buffer */ 
  vcopy( p_overlap, overlap_buf, 1, 1, nlong*overlap_fac ); 
  o_buf = overlap_buf; 
 
  /* Separate action for each Block Type */ 
   switch( windowSequence ) { 
   case ONLY_LONG_SEQUENCE :
#ifdef AAC_ELD
    if (wfun_select == WS_FHG_LDFB) {
      double *p_ldwin = (nlong==512) ? WIN512LD : WIN480LD;
      ildfb( p_in_data, transf_buf, 2*nlong );
      /* do windowing */
      vmult( transf_buf, p_ldwin, transf_buf, 1, 1, 1, nlong*4 );
      /* do overlap add */
      vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong*4 );
    } else {
#endif
      imdct( p_in_data, transf_buf, 2*nlong ); 
      vmult( transf_buf, window_long_prev, transf_buf, 1, 1, 1, nlong ); 
      if (overlap_select != NON_OVERLAPPED_MODE) {
        vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong ); 
        vmult( transf_buf+nlong, window_long+nlong-1, o_buf+nlong, 1, -1, 1, nlong );
      }
      else { /* overlap_select == NON_OVERLAPPED_MODE */
      vmult( transf_buf+nlong, window_long+nlong-1, transf_buf+nlong, 1, -1, 1, nlong );
      }    
#ifdef AAC_ELD
    }
#endif   
    break; 
 
  case LONG_START_SEQUENCE : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf, window_long_prev, transf_buf, 1, 1, 1, nlong ); 
    if (overlap_select != NON_OVERLAPPED_MODE) {
      vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong ); 
      vcopy( transf_buf+nlong, o_buf+nlong, 1, 1, nflat_ls ); 
      vmult( transf_buf+nlong+nflat_ls, window_short+nshort-1, o_buf+nlong+nflat_ls, 1, -1, 1, nshort ); 
      vcopy( &zero, o_buf+2*nlong-1, 0, -1, nflat_ls ); 
    }
    else { /* overlap_select == NON_OVERLAPPED_MODE */
      vmult( transf_buf+nlong+nflat_ls, window_short+nshort-1, transf_buf+nlong+nflat_ls, 1, -1, 1, nshort ); 
      vcopy( &zero, transf_buf+2*nlong-1, 0, -1, nflat_ls ); 
    }
    break; 
    
  case LONG_STOP_SEQUENCE : 
    imdct( p_in_data, transf_buf, 2*nlong ); 
    vmult( transf_buf+nflat_ls, window_short_prev_ptr, transf_buf+nflat_ls, 1, 1, 1, nshort ); 
    if (overlap_select != NON_OVERLAPPED_MODE) {
      vadd( transf_buf+nflat_ls, o_buf+nflat_ls, o_buf+nflat_ls, 1, 1, 1, nshort+nflat_ls ); 
      vmult( transf_buf+nlong, window_long+nlong-1, o_buf+nlong, 1, -1, 1, nlong ); 
    }
    else { /* overlap_select == NON_OVERLAPPED_MODE */
      vcopy( &zero, transf_buf, 0, 1, nflat_ls);
      vmult( transf_buf+nlong, window_long+nlong-1, transf_buf+nlong, 1, -1, 1, nlong);
    }
    break; 
 
  case EIGHT_SHORT_SEQUENCE : 
    {int ii; for(ii=0; ii<2*nlong; ii++) transf_buf[ii]=0;}
    if (overlap_select != NON_OVERLAPPED_MODE) {
      fp = o_buf + nflat_ls; 
      vcopy(&zero,o_buf+nlong,0,1,nlong);
    }
    else { /* overlap_select == NON_OVERLAPPED_MODE */
      fp = transf_buf;
    }
    for( k = num_short_win-1; k-->= 0; ) { 
      if (overlap_select != NON_OVERLAPPED_MODE) {
        imdct( p_in_data, transf_buf, 2*nshort ); 
        vmult( transf_buf, window_short_prev_ptr, transf_buf, 1, 1, 1, nshort ); 
        vadd( transf_buf, fp, fp, 1, 1, 1, nshort ); 
        vmult( transf_buf+nshort, window_short+nshort-1, transf_buf+nshort, 1, -1, 1, nshort ); 
        vadd( transf_buf+nshort, fp+nshort, fp+nshort, 1, 1, 1, nshort );
        p_in_data += nshort; 
        fp        += nshort; 
        window_short_prev_ptr = window_short; 
      }
      else { /* overlap_select == NON_OVERLAPPED_MODE */
        imdct( p_in_data, fp, 2*nshort );
        vmult( fp, window_short_prev_ptr, fp, 1, 1, 1, nshort ); 
        vmult( fp+nshort, window_short+nshort-1, fp+nshort, 1, -1, 1, nshort ); 
        p_in_data += nshort; 
        fp        += 2*nshort;
        window_short_prev_ptr = window_short; 
      }
    } 
    vcopy( &zero, o_buf+2*nlong-1, 0, -1, nflat_ls ); 
    break;     
    
   default : 
    CommonExit( 1, "mdct_synthesis: Unknown window type(1)" ); 
  } 

  /* copy output to pointer */
  if (overlap_select != NON_OVERLAPPED_MODE) {
    vcopy( o_buf, p_out_data, 1, 1, nlong ); 
  }
  else { /* overlap_select == NON_OVERLAPPED_MODE */
    vcopy( transf_buf, p_out_data, 1, 1, 2*nlong ); 
  }

  /* save unused output data */ 
  if (overlap_select != NON_OVERLAPPED_MODE)
  vcopy( o_buf+nlong, p_overlap, 1, 1, nlong*overlap_fac ); 
} 



/* %%%%%%%%%%%%%%%%% MDCT - STUFF %%%%%%%%%%%%%%%% */
#ifdef AAC_ELD
static void ldfb( double in_data[], double out_data[], int len ) {

    double *tmp_data;

    tmp_data = (double*)malloc(len*2*sizeof(double));
    vcopy(in_data, tmp_data, 1, 1, 2*len);
    LDFB(tmp_data, len, len/2, 1, -1);
    vcopy(tmp_data, out_data, 1, 1, len/2);
    free(tmp_data);
}
#endif

void mdct( double in_data[], double out_data[], int len )
{
  double *tmp_data;

  tmp_data = (double*)malloc(len*sizeof(double));
  vcopy(in_data, tmp_data, 1, 1, len); 
  MDCT(tmp_data, len, len/2, 1);
  vcopy(tmp_data, out_data, 1, 1, len/2); 
  free(tmp_data);
}


void buffer2freq(
  double           p_in_data[], 
  double           p_out_mdct[],
  double           p_overlap[],
  WINDOW_SEQUENCE  windowSequence,
  WINDOW_SHAPE     wfun_select,      /* offers the possibility to select different window functions */
  WINDOW_SHAPE     wfun_select_prev,
  int              nlong,            /* shift length for long windows   */
  int              nshort,           /* shift length for short windows  */
  Mdct_in          overlap_select,   /* select mdct input *TK*          */
                                     /* switch (overlap_select) {       */
                                     /* case OVERLAPPED_MODE:                */
                                     /*   p_in_data[]                   */
                                     /*   = overlapped signal           */
                                     /*         (bufferlength: nlong)   */
                                     /* case NON_OVERLAPPED_MODE:            */
                                     /*   p_in_data[]                   */
                                     /*   = non overlapped signal       */
                                     /*         (bufferlength: 2*nlong) */
  int              previousMode,
  int              nextMode,
  int              num_short_win     /* number of short windows to      */
                                     /* transform                       */
  /*int              save_window*/       /* save window information         */
)
{
	double         transf_buf[ 2*MAX_SHIFT_LEN_LONG ] = {0};
  double         windowed_buf[ 2*MAX_SHIFT_LEN_LONG ] = {0};
  double           *p_o_buf;
  int             k;

  double           window_long[MAX_SHIFT_LEN_LONG]; 
  double           window_long_prev[MAX_SHIFT_LEN_LONG]; 
  double           window_short[MAX_SHIFT_LEN_LONG]; 
  double           window_short_prev[MAX_SHIFT_LEN_LONG];
  double           window_extra_short[MAX_SHIFT_LEN_LONG];
  double           *window_short_prev_ptr;

  int nflat_ls    = (nlong-nshort)/ 2; 
  int transfak_ls =  nlong/nshort; 
  static int firstTime=1;
  int overlap_fac = 1;

#ifdef AAC_ELD  
  if (wfun_select == WS_FHG_LDFB) overlap_fac = 3;
#endif
  
  window_short_prev_ptr = window_short_prev ; 


  if( (nlong%nshort) || (nlong > MAX_SHIFT_LEN_LONG) || (nshort > MAX_SHIFT_LEN_LONG/2) ) { 
    CommonExit( 1, "mdct_analysis: Problem with window length" ); } 
  if( windowSequence==EIGHT_SHORT_SEQUENCE
      && ( (num_short_win <= 0) || (num_short_win > transfak_ls) ) ) {
    CommonExit( 1, "mdct_analysis: Problem with number of short windows" );  } 
  
  calc_window( window_long,      nlong, wfun_select ); 
  calc_window( window_long_prev, nlong, wfun_select_prev ); 
  calc_window( window_short,      nshort, wfun_select ); 
  calc_window( window_short_prev, nshort, wfun_select_prev );
  /* lcm */
  calc_window( window_extra_short, nshort/2, wfun_select );
  
  if (overlap_select != NON_OVERLAPPED_MODE) {
    /* create / shift old values */
    /* We use p_overlap here as buffer holding the last frame time signal*/
    if (firstTime){
      firstTime=0;
      vcopy( &zero, transf_buf, 0, 1, nlong*overlap_fac );
    }
    else

#ifdef AAC_ELD
    if (wfun_select != WS_FHG_LDFB)
#endif
    	/*      vcopy( p_overlap, transf_buf, 1, 1, nlong*overlap_fac );*/
		/*{   
    	if(previousMode==1){
            vcopy( p_in_data, transf_buf+nlong+128, 1, 1, nlong+128);
            vcopy( p_in_data, p_overlap,        1, 1, nlong+128 );
    	}else{
    	
        vcopy( p_in_data, transf_buf+nlong, 1, 1, nlong );
        vcopy( p_in_data, p_overlap,        1, 1, nlong );
       }
      }*/
      
      {   
    	if(previousMode==1){
	  vcopy( &zero, transf_buf, 0, 1, (64+128) );
	  vcopy( p_overlap, transf_buf+64+128, 1, 1, nlong);
	  vcopy( p_in_data, transf_buf+64+128+nlong, 1, 1, nlong);
	  vcopy( &zero, transf_buf+64+128+2*nlong, 0, 1, 64 );
	  vcopy( p_in_data, p_overlap,        1, 1, nlong);
    	}else{
	  vcopy( p_overlap, transf_buf, 1, 1, nlong*overlap_fac );
	  /* Append new data */
	  vcopy( p_in_data, transf_buf+nlong, 1, 1, nlong );
	  vcopy( p_in_data, p_overlap,        1, 1, nlong );
       }
      }
  }
  else { /* overlap_select == NON_OVERLAPPED_MODE */
    vcopy( p_in_data, transf_buf, 1, 1, 2*nlong);
  }

  /* Set ptr to transf-Buffer */
  p_o_buf = transf_buf;
  
  
  /* Separate action for each Block Type */
  switch( windowSequence ) {
   case ONLY_LONG_SEQUENCE :
#ifdef AAC_ELD
   if (wfun_select == WS_FHG_LDFB) {
    double *p_ldwin = (nlong==512) ? WIN512LD : WIN480LD;
    int winoff = (nlong==512) ? NUM_LD_COEF_512 : NUM_LD_COEF_480;
    /*vcopy ( windowed_buf, w_buf, 1,1, nlong*3);*/
    vcopy ( p_in_data, &p_o_buf[nlong*3], 1,1,nlong);
    vcopy ( p_o_buf+nlong, p_overlap, 1,1, nlong*3);
    
    vmult( p_o_buf, p_ldwin+winoff-1,p_o_buf, 1, -1,  1, nlong*4 );
    ldfb(p_o_buf, p_out_mdct, 2*nlong);
   }
   else
   {
#endif     
     vmult( p_o_buf, window_long_prev, windowed_buf,       1, 1,  1, nlong );
     vmult( p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, 1, -1, 1, nlong );
     mdct( windowed_buf, p_out_mdct, 2*nlong );
#ifdef AAC_ELD   
   }
#endif
    break;
   
   case LONG_START_SEQUENCE:
    vmult( p_o_buf, window_long_prev, windowed_buf, 1, 1, 1, nlong );
    vcopy( p_o_buf+nlong, windowed_buf+nlong, 1, 1, nflat_ls );
    vmult( p_o_buf+nlong+nflat_ls, window_short+nshort-1, windowed_buf+nlong+nflat_ls, 1, -1, 1, nshort );
    vcopy( &zero, windowed_buf+2*nlong-1, 0, -1, nflat_ls );
    mdct( windowed_buf, p_out_mdct, 2*nlong );
    break;
    
   /*case AMR_START_SEQUENCE:
    vmult( p_o_buf, window_long_prev, windowed_buf, 1, 1, 1, nlong );
    vcopy( p_o_buf+nlong, windowed_buf+nlong, 1, 1, nflat_ls );
    vmult( p_o_buf+nlong+nflat_ls, window_extra_short+nshort/2-1, windowed_buf+nlong+nflat_ls, 1, -1, 1, nshort/2 );
    vcopy( &zero, windowed_buf+2*nlong-1, 0, -1, nflat_ls+nshort/2 );
    mdct( windowed_buf, p_out_mdct, 2*nlong );
    break;*/
    
   case LONG_STOP_SEQUENCE :
    vcopy( &zero, windowed_buf, 0, 1, nflat_ls );
    vmult( p_o_buf+nflat_ls, window_short_prev_ptr, windowed_buf+nflat_ls, 1, 1, 1, nshort );
    vcopy( p_o_buf+nflat_ls+nshort, windowed_buf+nflat_ls+nshort, 1, 1, nflat_ls );
    vmult( p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, 1, -1, 1, nlong );
    mdct( windowed_buf, p_out_mdct, 2*nlong );
    break;
    
   /*case STOP_1152_SEQUENCE :
       vcopy( &zero, windowed_buf, 0, 1, (nlong/2) );
       vmult( p_o_buf+(nlong/2), window_short_prev_ptr, windowed_buf+(nlong/2), 1, 1, 1, nshort );
       vcopy( p_o_buf+(nlong/2)+nshort, windowed_buf+(nlong/2)+nshort, 1, 1, (nlong/2)+(nshort/2) );
       vmult( p_o_buf+nlong+nshort+(nshort/2), window_long+nlong-1, windowed_buf+nlong+nshort+(nshort/2), 1, -1, 1, nlong );
       vcopy( &zero, windowed_buf+2*(nlong+128)-1, 0, -1, nshort/2 );
       mdct( windowed_buf, p_out_mdct, 2*(nlong+128) );
   break;*/
       
   case STOP_START_SEQUENCE : 
   	/*if(nextMode==0){*/
   		 vcopy( &zero, windowed_buf, 0, 1, nflat_ls );
   		 vmult( p_o_buf+nflat_ls, window_short_prev_ptr, windowed_buf+nflat_ls, 1, 1, 1, nshort );
   		 vcopy( p_o_buf+nflat_ls+nshort, windowed_buf+nflat_ls+nshort, 1, 1, nflat_ls );   
   		 vmult( p_o_buf+nlong+nflat_ls, window_short+nshort-1, windowed_buf+nlong+nflat_ls, 1, -1, 1, nshort );
   		 vcopy( &zero, windowed_buf+2*nlong-1, 0, -1, nflat_ls );
   		 mdct( windowed_buf, p_out_mdct, 2*nlong );
   		 
   /*	}else{
   		 vcopy( &zero, windowed_buf, 0, 1, nflat_ls );
   		 vmult( p_o_buf+nflat_ls, window_short_prev_ptr, windowed_buf+nflat_ls, 1, 1, 1, nshort );
   		 vcopy( p_o_buf+nflat_ls+nshort, windowed_buf+nflat_ls+nshort, 1, 1, nflat_ls );   
   		 vmult( p_o_buf+nlong+nflat_ls, window_extra_short+nshort/2-1, windowed_buf+nlong+nflat_ls, 1, -1, 1, nshort/2 );
   		 vcopy( &zero, windowed_buf+2*nlong-1, 0, -1, nflat_ls+nshort/2 );
   		 mdct( windowed_buf, p_out_mdct, 2*nlong );
   	}*/
   break;   
   
   /*case STOP_START_1152_SEQUENCE :
   	if(nextMode==0){
       vcopy( &zero, windowed_buf, 0, 1, (nlong/2) );
       vmult( p_o_buf+(nlong/2), window_short_prev_ptr, windowed_buf+(nlong/2), 1, 1, 1, nshort );
       vcopy( p_o_buf+(nlong/2)+nshort, windowed_buf+(nlong/2)+nshort, 1, 1, nlong );  
	   vmult( p_o_buf+nlong+(nlong/2)+nshort, window_short+nshort-1, windowed_buf+nlong+(nlong/2)+nshort, 1, -1, 1, nshort );
	   vcopy( &zero, windowed_buf+2*(nlong+128)-1, 0, -1, (nlong/2));
	   mdct( windowed_buf, p_out_mdct, 2*(nlong+128) );
   	}else{
        vcopy( &zero, windowed_buf, 0, 1, (nlong/2) );
        vmult( p_o_buf+(nlong/2), window_short_prev_ptr, windowed_buf+(nlong/2), 1, 1, 1, nshort );
        vcopy( p_o_buf+(nlong/2)+nshort, windowed_buf+(nlong/2)+nshort, 1, 1, nlong );  
 	    vmult( p_o_buf+nlong+(nlong/2)+nshort, window_extra_short+nshort/2-1, windowed_buf+nlong+(nlong/2)+nshort, 1, -1, 1, nshort/2 );
 	    vcopy( &zero, windowed_buf+2*(nlong+128)-1, 0, -1, (nlong/2)+(nshort/2));
 	    mdct( windowed_buf, p_out_mdct, 2*(nlong+128) );	
   	}
	break; */
	   
   case EIGHT_SHORT_SEQUENCE :
    if (overlap_select != NON_OVERLAPPED_MODE) {
      p_o_buf += nflat_ls;
    }
   /*	if(nextMode!=0){
   		for (k=num_short_win-1; k-->=0; ) {
          vmult( p_o_buf, window_short_prev_ptr,   windowed_buf, 1, 1,  1, nshort );
          if(k==1){
        	  vmult( p_o_buf+nshort, window_extra_short+nshort/2-1, windowed_buf+nshort, 1, -1, 1, nshort/2 );
       	      vcopy( &zero, windowed_buf+2*(nshort)-1, 0, -1, (nshort/2));
          }else{
        	  vmult( p_o_buf+nshort, window_short+nshort-1, windowed_buf+nshort, 1, -1, 1, nshort );
          }
          mdct( windowed_buf, p_out_mdct, 2*nshort );

          p_out_mdct += nshort;
          p_o_buf    += (overlap_select != NON_OVERLAPPED_MODE) ? nshort : 2*nshort;
          window_short_prev_ptr = window_short; 
        }
   	}else{*/
   		for (k=num_short_win-1; k-->=0; ) {
   		          vmult( p_o_buf,  window_short_prev_ptr, windowed_buf,1, 1,  1, nshort );
   		          vmult( p_o_buf+nshort, window_short+nshort-1, windowed_buf+nshort, 1, -1, 1, nshort );
   		          mdct( windowed_buf, p_out_mdct, 2*nshort );

   		          p_out_mdct += nshort;
   		          p_o_buf    += (overlap_select != NON_OVERLAPPED_MODE) ? nshort : 2*nshort;
   		          window_short_prev_ptr = window_short; 
   		        }
   	/*}*/
    break;
   default :
      CommonExit( 1, "mdct_synthesis: Unknown window type (2)" ); 
  }

  /* Set output data 
  vcopy(transf_buf, p_out_mdct,1, 1, nlong); */
 
}

/***********************************************************************************************/ 
 
