/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Bernhard Grill       (Fraunhofer IIS)

and edited by
Huseyin Kemal Cakmak (Fraunhofer IIS)
Takashi Koike        (Sony Corporation)
Naoki Iwakami        (Nippon Telegraph and Telephone)
Olaf Kaehler         (Fraunhofer IIS)
Jeremie Lecomte      (Fraunhofer IIS)

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
$Id: usac_imdct.c,v 1.10 2012-03-13 11:26:09 frd Exp $
*******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allHandles.h"
#include "block.h"
#include "usac_main.h"
#include "tf_main.h"

#include "all.h"                 /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "usac_mainStruct.h"       /* structs */
#include "usac_tw_tools.h"

#include "allVariables.h"        /* variables */

#ifdef AAC_ELD
#include "win512LD.h"
#include "win480LD.h"
#endif

#include "common_m4a.h"
#include "vector_ops.h"
#include "tf_main.h"
#include "transfo.h"
#include "usac_transfo.h"
#include "usac_calcWindow.h"
#include "usac_tw_defines.h"


extern const double dolby_win_1024[1024]; /* symbol already in imdct.o */
extern const double dolby_win_960[960]; /* symbol already in imdct.o */
extern const double dolby_win_256[256]; /* symbol already in imdct.o */
extern const double dolby_win_128[128]; /* symbol already in imdct.o */
extern const double dolby_win_120[120]; /* symbol already in imdct.o */
extern const double dolby_win_32[32]; /* symbol already in imdct.o */
extern const float ShortWindowSine64[64];
extern const double dolby_win_16[16]; /* symbol already in imdct.o */
extern const double dolby_win_4[4]; /* symbol already in imdct.o */
extern const float usac_sine_win_64[64];
extern const float usac_kbd_win_64[64];
extern const float usac_sine_win_1024[1024];
extern const float usac_sine_win_128[128];
extern const float usac_dolby_win_1024[1024];
extern const float usac_dolby_win_128[128];
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif


#ifndef TW_M_PI
#define TW_M_PI 3.14159265358979323846
#endif

/* begin IMDCT */
#include "proto_func.h"

typedef struct T_IIS_FFT {

  int iSign;  /* Fwd or Bwd FFT */
  int len;    /* length */
  int shift;


  float* trigPtr;

} IIS_FFT, *HANDLE_IIS_FFT;

typedef enum {
  IIS_FFT_NO_ERROR = 0,
  IIS_FFT_INTERNAL_ERROR
} IIS_FFT_ERROR;

typedef enum {
  IIS_FFT_FWD = -1,
  IIS_FFT_BWD =  1
} IIS_FFT_DIR;


static int nmax=0;
static float *cosTab=NULL;

static
IIS_FFT_ERROR IIS_CFFT_Create(HANDLE_IIS_FFT* phIisFft, int len, int iSign){

  HANDLE_IIS_FFT hIisFft;

  /* first handle is not valid */
  *phIisFft = NULL;

  if( iSign != IIS_FFT_FWD && iSign != IIS_FFT_BWD ){
    return IIS_FFT_INTERNAL_ERROR;   /*neither forward nor backward FFT*/
  }

  if( NULL == (hIisFft = (HANDLE_IIS_FFT)calloc(1, sizeof(IIS_FFT)))){
    return IIS_FFT_INTERNAL_ERROR;
  }

  for( hIisFft->shift = 1; len>>hIisFft->shift != 2 && len>>hIisFft->shift != 3 && len>>hIisFft->shift != 5 && len>>hIisFft->shift != 0;  hIisFft->shift++ );
  hIisFft->len   = len;
  hIisFft->iSign = iSign;


  /* now handle is valid */
  *phIisFft = hIisFft;
  return IIS_FFT_NO_ERROR;

}

static
IIS_FFT_ERROR IIS_FFT_Apply_CFFT(HANDLE_IIS_FFT hIisFft, float* pInReBuffer, float* pInImBuffer, float* pOutReBuffer, float* pOutImBuffer ){


  if( hIisFft != NULL){
	  							       /*CFFTN*/
    if (IIS_FFT_NO_ERROR != !(CFFTN_NI(pInReBuffer, pInImBuffer, pOutReBuffer, pOutImBuffer, hIisFft->len, hIisFft->iSign))){
      return IIS_FFT_INTERNAL_ERROR;
    }

    return IIS_FFT_NO_ERROR;
  }

  else{
    return IIS_FFT_INTERNAL_ERROR;
  }

}


static
IIS_FFT_ERROR IIS_CFFT_Destroy(HANDLE_IIS_FFT* phIisFft){

  HANDLE_IIS_FFT hIisFft;
  if(phIisFft != NULL){
    if(*phIisFft != NULL){

      hIisFft = *phIisFft;
      if( hIisFft != NULL ){
        free(hIisFft);
      }
      *phIisFft = NULL;
    }
  }
  return IIS_FFT_NO_ERROR;
}




static void Dct_vi(float vec[], HANDLE_IIS_FFT hIisFft, int size, float* cosPhi, float* sinPhi) /*DOUBLE per float*/
{
  int  i;
  float vecRe[576];
  float vecIm[576];

  /* reorder input */

  if(vec[0] != 0)
   i=0;

  for (i = 0; i < size/4; i++) {
    float  tmp;

    vec[2*i]         = -vec[2*i];
    vec[2*i+size/2]  = -vec[2*i+size/2];

    tmp              = vec[2*i+1];
    vec[2*i+1]       = vec[size-1-2*i];
    vec[size-1-2*i]  = tmp;
  }

  /* pre modulation */
  for (i = 0; i < size/2; i++) {
    float  t;  /*DOUBLE per float*/

    t = vec[i*2] * cosPhi[i] - vec[i*2+1] * sinPhi[i];
    vecIm[i] = vec[i*2+1] * cosPhi[i] + vec[i*2] * sinPhi[i];
    vecRe[i] = t;
  }

  IIS_FFT_Apply_CFFT(hIisFft, vecRe, vecIm, vecRe, vecIm );

  for(i = 0; i < size/2; i++) {
    float t;   /*DOUBLE per float*/

    t = vecRe[i] * cosPhi[i] - vecIm[i] * sinPhi[i];
    vec[i*2+1] = vecIm[i] * cosPhi[i] + vecRe[i] * sinPhi[i];
    vec[i*2]   = t;
  }

  /* post modulation */
 /* reorder output */
  for (i = 0; i < size/4; i++) {
    float  tmp;

    vec[2*i]        = -vec[2*i];
    vec[2*i+size/2] = -vec[2*i+size/2];

    tmp             =  vec[2*i+1];
    vec[2*i+1]      = -vec[size-1-2*i];
    vec[size-1-2*i] = -tmp;
 }
}


static void *Erealloc (
          void*  ptr, /* in: old memory pointer */
          size_t size)  /* in: size in bytes */
          /* returns: new memory pointer */
{
  void *ptr2;

  if (!(ptr2=realloc(ptr,size))) {
    fprintf(stderr,
      "re-allocation of %d bytes failed in Erealloc",(int)size);
    exit(1);
  }
  return ptr2;
}



static void Bitrev(float* x,int n)
{
  int i,j,k;
  float v;

  j = 0;
  for(i=1;i<n-1;i++) {
    k = n/2;
    while(k<=j) {
      j -= k;
      k /= 2;
    }
    j += k;
    if(i<j) {
      v = x[i];
      x[i] = x[j];
      x[j] = v;
    }
  }
}




static void DctInit(int n)
{
  int i;

  for(i=2;i<n;i*=2);
  if(i!=n) {
    fprintf(stderr,"Block length is not a power of 2: %d",n);
    exit(1);
  }
  if(n>nmax) {
    cosTab = (float *)Erealloc(cosTab,2*n*sizeof(float));
    nmax = n;
    for(i=0;i<2*nmax;i++)
      cosTab[i] = .5/cos(TW_M_PI*(float)i/nmax/4.);
  }
}


static void DctIV(float* x,int n)
{
  int i,j,k,len,ba,of;
  float f,v;

  DctInit(n);

  k = nmax/n;       /* input mult. for type IV */
  for(i=0;i<n;i++){
    x[i] = x[i]*cosTab[k*(2*i+1)];
  }

  len = n/2;        /* butterfly width */
  while(len>=1) {
    k = nmax/len;
    for(of=0;of<len;of++) {   /* offset within group */
      f = cosTab[k*(2*of+1)];
      for(ba=0;ba<n;ba+=2*len) {  /* offset of group */
        if(of<((len+1)/2)) {    /* swap second half of group */
          i = ba+len+of;
          j = ba+2*len-1-of;
          v = x[i];
          x[i] = x[j];
          x[j] = v;
        }
        i = ba+of;      /* calculate butterfly */
        j = i+len;
        v = (x[i]-x[j])*f;
        x[i] += x[j];
        x[j] = v;
      }
    }
    len /= 2;
  }
  Bitrev(x,n);        /* bit reversal */

  len = n/2;        /* output additions */
  while(len>1) {
    for(i=0;i<n-len;i++){
      if((2*i/len)%2){
        x[i] += x[i+len];
      }
    }
    len /= 2;
  }

  for(i=0;i<n-1;i++) x[i] += x[i+1];  /* output additions for type IV */
}

static
void InverseMDCTransform(float *pInData, float *pOutData, int N, HANDLE_IIS_FFT hIisFft, float* cosPhi, float* sinPhi)
{
  float tmpBuffer[8192];

  int       i;
  float     norm = 2.0f / (float)N;

  for (i = 0 ; i < N / 2 ; i++) {
    tmpBuffer[i] = pInData[i] * norm;
  }

  Dct_vi(tmpBuffer, hIisFft, N/2, cosPhi, sinPhi);

  for (i = 0 ; i < N / 4 ; i++) {
    pOutData [i]                     =  tmpBuffer [N / 4 + i];
    pOutData [N / 2 - i - 1]         = -tmpBuffer [N / 4 + i];
    pOutData [N / 2 + N / 4 + i]     = -tmpBuffer [i];
    pOutData [N / 2 + N / 4 - i - 1] = -tmpBuffer [i];
  }
}
/* end IMDCT */



static const float zero = 0;




/* %%%%%%%%%%%%%%%%%% IMDCT - STUFF %%%%%%%%%%%%%%%%*/

#define MAX_SHIFT_LEN_LONG 4096


void usac_imdct_float(float in_data[], float out_data[], int len)
{
  HANDLE_IIS_FFT hIisFft;
  int i;
  float *cosPhi = (float*) calloc(len/4, sizeof(float));
  float *sinPhi = (float*) calloc(len/4, sizeof(float));

  for (i = 0; i < len/4; i++) {
    sinPhi[i] = (float)sin(PI*(i+0.125)/(len/2));
    cosPhi[i] = (float)cos(PI*(i+0.125)/(len/2));
  }

  IIS_CFFT_Create(&hIisFft, len/4, IIS_FFT_BWD);

  InverseMDCTransform(in_data, out_data, len, hIisFft, cosPhi, sinPhi);

  IIS_CFFT_Destroy(&hIisFft);

  free(sinPhi);
  free(cosPhi);
}

static
void usac_imdct(float in_data[], float out_data[], int len)
{
  int i;
  float *inDataTmp = (float*) calloc(len/2, sizeof(float));
  float *outDataTmp = (float*) calloc(len, sizeof(float));

  for(i=0; i<len/2; i++){
    inDataTmp[i] = (float)(in_data[i]);
  }

  usac_imdct_float(inDataTmp, outDataTmp, len);

  for(i=0; i<len; i++){
    out_data[i] = outDataTmp[i];
  }

  free(inDataTmp);
  free(outDataTmp);
}

void usac_tw_imdct(float in_data[], float out_data[], int len)
{

  int i;
  int   N=len;
  float *inDataTmp = (float*) calloc(len/2, sizeof(float));
  float *outDataTmp = (float*) calloc(len, sizeof(float));
  float tmpBuffer[8192];
  float     norm = 2.0f / (float)N;


  for(i=0; i<len/2; i++){
    inDataTmp[i] = in_data[i];
  }

   /* const int N = windowLen; */


    for (i = 0 ; i < N /2 ; i++) {
  /*  tmpBuffer[i] = (pInData[i] * (2.0F / N)); */
      tmpBuffer[i] = inDataTmp[i]; /*norm*/;
    }

    DctIV(tmpBuffer, N/2);

    for (i = 0 ; i < N / 4 ; i++) {
      outDataTmp [i]                     =  tmpBuffer [N / 4 + i];
      outDataTmp [N / 2 - i - 1]         = -tmpBuffer [N / 4 + i];
      outDataTmp [N / 2 + N / 4 + i]     = -tmpBuffer [i];
      outDataTmp [N / 2 + N / 4 - i - 1] = -tmpBuffer [i];
    }

  for(i=0; i<len; i++){
    out_data[i] = outDataTmp[i]*norm;
  }

  free(inDataTmp);
  free(outDataTmp);

}


void fd2buffer(
               float           p_in_data[],
               float           p_out_data[],
               float           p_overlap[],
               WINDOW_SEQUENCE  windowSequence,
               WINDOW_SEQUENCE  windowSequenceLast,
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
               int              num_short_win,     /* number of short windows to      */
               /* transform                       */
               int             FrameWasTd,
               int             twMdct,
               float           sample_pos[],
               float           tw_trans_len[],
               int              tw_start_stop[],
               float           past_sample_pos[],
               float           past_tw_trans_len[],
               int             past_tw_start_stop[],
               float           past_warped_time_sample_vector[],
               int ApplyFac,
               int facPrm[],
               float lastLPC[],
               float acelpZIR[],
               HANDLE_USAC_TD_DECODER    st,
               HANDLE_RESILIENCE hResilience,
               HANDLE_ESC_INSTANCE_DATA hEscInstanceData
               )
{
  float           *o_buf, transf_buf[ 2*MAX_SHIFT_LEN_LONG ]={0.0};
  float           transf_buf_past[ 2*MAX_SHIFT_LEN_LONG ]={0.0};
  float           overlap_buf[ 2*MAX_SHIFT_LEN_LONG ] = {0.0};

  float           window_long[MAX_SHIFT_LEN_LONG];
  float           window_long_prev[MAX_SHIFT_LEN_LONG];
  float           window_short[MAX_SHIFT_LEN_LONG];
  float           window_short_prev[MAX_SHIFT_LEN_LONG];
  float           window_extra_short[MAX_SHIFT_LEN_LONG];
  float           os_window[MAX_SHIFT_LEN_LONG*TW_OS_FACTOR_WIN];
  float           os_window_prev[MAX_SHIFT_LEN_LONG*TW_OS_FACTOR_WIN];
  float           *window_short_prev_ptr;

  float  *fp;
  int      k;
  int      overlap_fac = 1;
  int nflat_ls    = (nlong-nshort)/ 2;
  int transfak_ls =  nlong/nshort;
  int lfac = LFAC_1024;
  int ntrans_ls = nshort;
  float facTimeSig[2*LFAC_1024];
  int sbrSyncDelay = 0;

  memset(facTimeSig, 0, sizeof(facTimeSig));
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
  if ( FrameWasTd ) {
    if ( windowSequence  == EIGHT_SHORT_SEQUENCE) {
      lfac = nlong/16;
    }
    else {
      lfac = nlong/8;
    }
    nflat_ls    = (nlong-lfac*2)/ 2;
    ntrans_ls = lfac*2;
  }

  if ( ApplyFac) {
    decode_fdfac(facPrm, nlong/4, lfac, lastLPC, acelpZIR, facTimeSig);
  }

  if ( twMdct == 1 ) {
    /* oversampled windows for TW */
     calc_tw_window (os_window,          nlong*TW_OS_FACTOR_WIN, wfun_select);
     calc_tw_window (os_window_prev,     nlong*TW_OS_FACTOR_WIN, wfun_select_prev);
     calc_window( window_extra_short,    nshort/2,               wfun_select );
     calc_window( window_short_prev_ptr, ntrans_ls,              wfun_select_prev );
  }
  else {
    calc_window( window_long,           nlong,     wfun_select );
    calc_window( window_long_prev,      nlong,     wfun_select_prev );
    calc_window( window_short,          nshort,    wfun_select );
    calc_window( window_short_prev_ptr, ntrans_ls, wfun_select_prev );
    calc_window( window_extra_short,    nshort/2,  wfun_select );
  }


  /* Assemble overlap buffer */
  if ( twMdct == 1 ) {
    vcopy (p_overlap, overlap_buf, 1, 1, 2*nlong*overlap_fac + sbrSyncDelay);
  }
  else {
    vcopy( p_overlap, overlap_buf, 1, 1, nlong*overlap_fac );
  }
  o_buf = overlap_buf;


  /* Separate action for each Block Type */
  if ( twMdct ){ /* tw */
    float tw_transf_buf[ 2*MAX_SHIFT_LEN_LONG ];
    float tw_transf_buf_past[ 2*MAX_SHIFT_LEN_LONG ];
    int nmdct=nlong;
    int nmdct_past = nlong;
    vcopy(&zero,tw_transf_buf,0,1,2*MAX_SHIFT_LEN_LONG);
    if ( overlap_select == NON_OVERLAPPED_MODE ) {
      /* exit with error, we don't do that in tw-mode */
      exit(-1);
    }


     /* map to correct window sequence */
     switch ( windowSequence ) {
     case ONLY_LONG_SEQUENCE:
     case LONG_START_SEQUENCE:
     case LONG_STOP_SEQUENCE:
     case STOP_START_SEQUENCE:
     case EIGHT_SHORT_SEQUENCE:
       nmdct = nlong;
       break;
     default:
       /* do nothing */
       break;
    }
     /* map to correct window sequence */
     switch ( windowSequenceLast ) {
     case ONLY_LONG_SEQUENCE:
     case LONG_START_SEQUENCE:
     case LONG_STOP_SEQUENCE:
     case STOP_START_SEQUENCE:
     case EIGHT_SHORT_SEQUENCE:
       nmdct_past = nlong;
       break;
     default:
       /* do nothing */
       break;
     }

    vcopy( &zero, tw_transf_buf, 0, 1, 3*nlong);
    vcopy( past_warped_time_sample_vector, tw_transf_buf_past, 1 ,1, 3*nlong);
    if ( windowSequence != EIGHT_SHORT_SEQUENCE ) {

      usac_imdct(p_in_data, transf_buf, 2*nmdct);

      tw_windowing_long(transf_buf,
                        tw_transf_buf+TW_IPLEN2S,
                        tw_start_stop[0],
                        tw_start_stop[1],
                        nmdct,
                        tw_trans_len[0],
                        tw_trans_len[1],
                        os_window_prev,
                        os_window);
    }
    else {
      for ( k = 0 ; k < transfak_ls ; k++ ) {
        usac_imdct(p_in_data+k*nshort, transf_buf+2*k*nshort, 2*nshort);
      }
      tw_windowing_short(transf_buf,
                         tw_transf_buf+TW_IPLEN2S,
                         tw_start_stop[0],
                         tw_start_stop[1],
                         tw_trans_len[0],
                         tw_trans_len[1],
                         os_window_prev,
                         os_window,
                         (nmdct / 8));
    }
    tw_resamp(tw_transf_buf,
              0,
              2*nmdct+2*TW_IPLEN2S,
              nlong+(LFAC_1024*nlong)/L_FRAME_1024,
              sample_pos,
              0.5f,
              transf_buf);

    if ( windowSequenceLast != EIGHT_SHORT_SEQUENCE && !FrameWasTd ) {

      tw_windowing_past(tw_transf_buf_past+TW_IPLEN2S,
                        tw_transf_buf_past+TW_IPLEN2S,
                        past_tw_start_stop[1],
                        nmdct_past,
                        past_tw_trans_len[1],
                        os_window_prev);
    }

    if ( !FrameWasTd ) {
      tw_resamp(tw_transf_buf_past,
                nlong+(LFAC_1024*nlong)/L_FRAME_1024,
                2*nmdct_past+2*TW_IPLEN2S,
                3*nlong,
                past_sample_pos,
                0.5f,
                transf_buf_past);

      vadd(transf_buf_past + nlong + (LFAC_1024*nlong)/L_FRAME_1024, o_buf + sbrSyncDelay+(LFAC_1024*nlong)/L_FRAME_1024,o_buf+sbrSyncDelay+(LFAC_1024*nlong)/L_FRAME_1024,1,1,1,2*nlong-(LFAC_1024*nlong)/L_FRAME_1024);
    }

    if(ApplyFac){
      vadd(facTimeSig, transf_buf+nlong,transf_buf+nlong,1,1,1,2*lfac);
      vcopy(&zero, transf_buf,  0, 1, nlong);
      vcopy(&zero, o_buf+nlong, 0, 1, (LFAC_1024*nlong)/L_FRAME_1024);
    }

    if ( FrameWasTd && !ApplyFac ) {
      vmult( o_buf+(nlong/2)+nflat_ls, window_short_prev_ptr+ntrans_ls-1, o_buf+(nlong/2)+nflat_ls, 1, -1, 1, ntrans_ls );
      vcopy(&zero, o_buf+(nlong/2)+nflat_ls+ntrans_ls,0,1,nflat_ls);
    }

    /* OLA */
    vadd(transf_buf, o_buf + sbrSyncDelay, o_buf + sbrSyncDelay, 1 , 1, 1, nlong+(LFAC_1024*nlong)/L_FRAME_1024);

    /* copy to respective output buffers */
    vcopy (o_buf, p_out_data, 1, 1, nlong);
    vcopy (o_buf + nlong, p_overlap, 1, 1, 2*nlong + sbrSyncDelay);
    vcopy (tw_transf_buf,past_warped_time_sample_vector,1,1,2*nlong+2*TW_IPLEN2S);
    vcopy (sample_pos,past_sample_pos,1,1,3*nlong);
    past_tw_trans_len[0] = tw_trans_len[0];
    past_tw_trans_len[1] = tw_trans_len[1];
    past_tw_start_stop[0] = tw_start_stop[0];
    past_tw_start_stop[1] = tw_start_stop[1];

  }
  else {

    switch( windowSequence ) {
    case ONLY_LONG_SEQUENCE :
      usac_imdct( p_in_data, transf_buf, 2*nlong );
      vmult( transf_buf, window_long_prev, transf_buf, 1, 1, 1, nlong );
      if (overlap_select != NON_OVERLAPPED_MODE) {
        vmult( o_buf, window_long_prev+nlong-1, o_buf, 1, -1, 1, nlong );
        vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong );
        vcopy( transf_buf+nlong, o_buf+nlong, 1, 1, nlong );
      }
      else { /* overlap_select == NON_OVERLAPPED_MODE */
      }
      break;

    case STOP_START_SEQUENCE :
      usac_imdct( p_in_data, transf_buf, 2*nlong );
      vmult( transf_buf+nflat_ls, window_short_prev_ptr, transf_buf+nflat_ls, 1, 1, 1, ntrans_ls );
      if (overlap_select != NON_OVERLAPPED_MODE) {

        if (ApplyFac) {
          vcopy(&zero,transf_buf,0,1,nflat_ls+lfac);
          vadd(transf_buf+nflat_ls+lfac, facTimeSig, transf_buf+nflat_ls+lfac,1,1,1,2*lfac);
          vcopy(&zero, o_buf+nflat_ls+lfac,0,1,nflat_ls+lfac);
        }
        else {
          vcopy(&zero,transf_buf,0,1,nflat_ls);
          vmult( o_buf+nflat_ls, window_short_prev_ptr+ntrans_ls-1, o_buf+nflat_ls, 1, -1, 1, ntrans_ls );
          vcopy(&zero, o_buf+nflat_ls+ntrans_ls,0,1,nflat_ls);
        }
        vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong );
        vcopy( transf_buf+nlong, o_buf+nlong, 1, 1, nlong );
        /*vcopy( &zero, o_buf+2*nlong-1, 0, -1, nflat_ls );*/
      }
      else { /* overlap_select == NON_OVERLAPPED_MODE */
        vcopy( &zero, transf_buf+2*nlong-1, 0, -1, nflat_ls );
      }
      break;

    case LONG_START_SEQUENCE:
      usac_imdct( p_in_data, transf_buf, 2*nlong );
      vmult( transf_buf, window_long_prev, transf_buf, 1, 1, 1, nlong );
      if (overlap_select != NON_OVERLAPPED_MODE) {
        vmult( o_buf, window_long_prev+nlong-1, o_buf, 1, -1, 1, nlong );
        vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong );
        vcopy( transf_buf+nlong, o_buf+nlong, 1, 1, nlong );
      }
      else { /* overlap_select == NON_OVERLAPPED_MODE */
        vmult( transf_buf+nlong+nflat_ls, window_short+nshort-1, transf_buf+nlong+nflat_ls, 1, -1, 1, nshort );
        vcopy( &zero, transf_buf+2*nlong-1, 0, -1, nflat_ls );
      }
      break;

    case LONG_STOP_SEQUENCE :
      usac_imdct( p_in_data, transf_buf, 2*nlong );
      vmult( transf_buf+nflat_ls, window_short_prev_ptr, transf_buf+nflat_ls, 1, 1, 1, ntrans_ls );
      if (overlap_select != NON_OVERLAPPED_MODE) {

        if (ApplyFac) {
          vcopy(&zero,transf_buf,0,1,nflat_ls+lfac);
          vadd(transf_buf+nflat_ls+lfac, facTimeSig, transf_buf+nflat_ls+lfac,1,1,1,2*lfac);
          vcopy(&zero, o_buf+nflat_ls+lfac,0,1,nflat_ls+lfac);
        }
        else {
          vcopy(&zero,transf_buf,0,1,nflat_ls);
          vmult( o_buf+nflat_ls, window_short_prev_ptr+ntrans_ls-1, o_buf+nflat_ls, 1, -1, 1, ntrans_ls );
          vcopy(&zero, o_buf+nflat_ls+ntrans_ls,0,1,nflat_ls);
        }
        vadd( transf_buf, o_buf, o_buf, 1, 1, 1, nlong );
        vcopy( transf_buf+nlong, o_buf+nlong, 1, 1, nlong );
      }
      else { /* overlap_select == NON_OVERLAPPED_MODE */
        vcopy( &zero, transf_buf, 0, 1, nflat_ls);
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
      for( k = 0; k<num_short_win;k++ ) {
        if (k==0){
          if (overlap_select != NON_OVERLAPPED_MODE) {
            usac_imdct( p_in_data, transf_buf, 2*nshort );
            vmult( transf_buf, window_short_prev_ptr, transf_buf, 1, 1, 1, nshort );
#ifdef APPLY_FAC_NO_BUGFIX
            if (ApplyFac) {
              vcopy(&zero,transf_buf,0,1,lfac);
              vadd(transf_buf+lfac, facTimeSig, transf_buf+lfac,1,1,1,2*lfac);
              vcopy(&zero, fp+lfac,0,1,nflat_ls+lfac);
            }
            else {
              vcopy(&zero, fp+ntrans_ls,0,1,nflat_ls);
              vmult( fp, window_short_prev_ptr+ntrans_ls-1, fp, 1, -1, 1, ntrans_ls );
            }
#else
            if (ApplyFac) {
              vcopy(&zero,transf_buf,0,1,lfac);
              vcopy(&zero, fp+lfac,0,1,nflat_ls+lfac);
            } else {
              vcopy(&zero, fp+ntrans_ls,0,1,nflat_ls);
              vmult( fp, window_short_prev_ptr+ntrans_ls-1, fp, 1, -1, 1, ntrans_ls );
            }
#endif 
            vadd( transf_buf, fp, fp, 1, 1, 1, nshort );
            vmult( transf_buf+nshort, window_short+nshort-1, transf_buf+nshort, 1, -1, 1, nshort );
            vadd( transf_buf+nshort, fp+nshort, fp+nshort, 1, 1, 1, nshort );
            p_in_data += nshort;
            fp        += nshort;
            window_short_prev_ptr = window_short;
          }
          else { /* overlap_select == NON_OVERLAPPED_MODE */
            vmult( fp-nshort, window_short+nshort-1, fp-nshort, 1, -1, 1, nshort );
            usac_imdct( p_in_data, fp, 2*nshort );
            vmult( fp, window_short_prev_ptr, fp, 1, 1, 1, nshort );
            vmult( fp+nshort, window_short+nshort-1, fp+nshort, 1, -1, 1, nshort );
            p_in_data += nshort;
            fp        += 2*nshort;
            window_short_prev_ptr = window_short;
          }
        }else if(k==num_short_win-1){
          if (overlap_select != NON_OVERLAPPED_MODE) {
            usac_imdct( p_in_data, transf_buf, 2*nshort );
            vmult( transf_buf, window_short_prev_ptr, transf_buf, 1, 1, 1, nshort );
            vadd( transf_buf, fp, fp, 1, 1, 1, nshort );
            vadd( transf_buf+nshort, fp+nshort, fp+nshort, 1, 1, 1, nshort );
            p_in_data += nshort;
            fp        += nshort;
            window_short_prev_ptr = window_short;
          }
          else { /* overlap_select == NON_OVERLAPPED_MODE */
            usac_imdct( p_in_data, fp, 2*nshort );
            vmult( fp, window_short_prev_ptr, fp, 1, 1, 1, nshort );
            p_in_data += nshort;
            fp        += 2*nshort;
            window_short_prev_ptr = window_short;
          }
        }else{
          if (overlap_select != NON_OVERLAPPED_MODE) {
            usac_imdct( p_in_data, transf_buf, 2*nshort );
            vmult( transf_buf, window_short_prev_ptr, transf_buf, 1, 1, 1, nshort );
            vadd( transf_buf, fp, fp, 1, 1, 1, nshort );
            vmult( transf_buf+nshort, window_short+nshort-1, transf_buf+nshort, 1, -1, 1, nshort );
            vadd( transf_buf+nshort, fp+nshort, fp+nshort, 1, 1, 1, nshort );
            p_in_data += nshort;
            fp        += nshort;
            window_short_prev_ptr = window_short;
          }
          else { /* overlap_select == NON_OVERLAPPED_MODE */
            usac_imdct( p_in_data, fp, 2*nshort );
            vmult( fp, window_short_prev_ptr, fp, 1, 1, 1, nshort );
            vmult( fp+nshort, window_short+nshort-1, fp+nshort, 1, -1, 1, nshort );
            p_in_data += nshort;
            fp        += 2*nshort;
            window_short_prev_ptr = window_short;
          }
        }
      }
#ifndef APPLY_FAC_NO_BUGFIX
      if (ApplyFac) {
        vadd(o_buf + nflat_ls + lfac, facTimeSig,o_buf + nflat_ls + lfac, 1, 1, 1, 2*lfac);
      }
#endif
      vcopy( &zero, o_buf+2*nlong-1, 0, -1, nflat_ls );
      break; 

    default :
      CommonExit( 1, "mdct_synthesis: Unknown window type(1)" );
    }
    /* copy output to pointer */
    if (overlap_select != NON_OVERLAPPED_MODE) {
      if ( FrameWasTd ) {
        /* base postfilter FAC area */
        decoder_LPD_BPF_end(windowSequence == EIGHT_SHORT_SEQUENCE,
                            o_buf,
                            st);
      }
      vcopy( o_buf, p_out_data, 1, 1, nlong );
    }
    else { /* overlap_select == NON_OVERLAPPED_MODE */
      if(!FrameWasTd){
        vcopy( transf_buf, p_out_data, 1, 1, 2*nlong );
      }else{
        vcopy( transf_buf+192, p_out_data, 1, 1, 2*nlong );
      }
    }

    /* save unused output data*/
    if (overlap_select != NON_OVERLAPPED_MODE){

#ifndef USAC_REFSOFT_COR1_NOFIX_06
      vcopy( o_buf+nlong-L_DIV_1024, p_overlap-L_DIV_1024, 1, 1, nlong*overlap_fac+L_DIV_1024 );
#else
      vcopy( o_buf+nlong, p_overlap, 1, 1, nlong*overlap_fac );
#endif
    }
  }
}



/* %%%%%%%%%%%%%%%%% MDCT - STUFF %%%%%%%%%%%%%%%% */
void  TDAliasing( float                 inputData[],
                  float                 Output[],
                  int              nlong,
                  int              nshort,
                  int                   pos)
{
	  /* lcm */
  WINDOW_SHAPE pWs = WS_FHG;
  int i;
  int len = 2*nshort;

  float windowBuffer[256];
  float tdaBuffer[128];
  float window_short_prev[MAX_SHIFT_LEN_LONG];


  calc_window( window_short_prev, nshort,  pWs );
  /* data copying and windowing */
  if(pos==2){
    for(i=0; i<len/2; i++){
      windowBuffer[i]           = 0;
      windowBuffer[len - 1 - i] = window_short_prev[i] * inputData[len/2 - 1 - i];
    }
  }
 else if (pos == 1) {
    for(i=0; i<len/2; i++){
      windowBuffer[i]           = inputData[i];
      windowBuffer[len - 1 - i] = 0;
     }
  }

  /* fold in */
  for(i=0; i<len/4; i++){
    tdaBuffer[i]             = -1.0 * windowBuffer[len/4 - 1 - i]   + windowBuffer[len/4 + i];
    tdaBuffer[len/2 - 1 - i] =        windowBuffer[3*len/4 - 1 - i] + windowBuffer[3*len/4 + i];
    }

  /* fold out */
  for(i=0; i<len/4; i++){
    windowBuffer[i]             = -1.0 * tdaBuffer[len/4 - 1 - i];
    windowBuffer[len/2 - 1 - i] =  1.0 * tdaBuffer[len/4 - 1 - i];
    windowBuffer[len/2 + i]     =  1.0 * tdaBuffer[len/4 + i];
    windowBuffer[len - 1 - i]   =  1.0 * tdaBuffer[len/4 + i];
  }

  /* synthesis windowing */
  for(i=0; i<len/2; i++){
    windowBuffer[i]           = window_short_prev[i] * windowBuffer[i];
    windowBuffer[len - 1 - i] = window_short_prev[i] * windowBuffer[len - 1 - i];
  }

  /* copy correct part of output data into buffer */
  if (pos == 2) {
    for(i=0; i<len/2; i++){
      Output[i]         = windowBuffer[len/2 + i];
      Output[len/2 + i] = 0;
    }
  } else if (pos == 1){
    for(i=0; i<len/2; i++){
      Output[i]         = windowBuffer[i];
      Output[len/2 + i] = 0;
    }
  }

}

