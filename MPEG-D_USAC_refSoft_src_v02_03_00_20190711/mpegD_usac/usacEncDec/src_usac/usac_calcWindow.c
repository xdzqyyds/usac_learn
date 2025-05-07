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
$Id: usac_calcWindow.c,v 1.4 2011-03-01 10:08:36 frd Exp $
*******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usac_calcWindow.h"
#include "usac_tw_defines.h"
#include "common_m4a.h"

#ifdef AAC_ELD
#include "win512LD.h"
#include "win480LD.h"
#endif



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

/* 768 window length */
extern const float usac_sine_win_768[768];
extern const float usac_dolby_win_768[768];
extern const float usac_sine_win_192[192];
extern const float usac_dolby_win_192[192];
extern const float usac_sine_win_96[96];
extern const float usac_dolby_win_96[96];


#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#ifndef TW_M_PI
#define TW_M_PI 3.14159265358979323846
#endif


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
static void CalculateKBDWindow(float* win, double alpha, int length)
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
    win[i] = (float) sqrt(tmp*sum);
  }
}

#define BESSEL_EPSILON 1.0e-17

static double Bessel(double x)
{
  register double p,s,ds,k;

  x *= 0.5;
  k = 0.0;
  p = s = 1.0;
  do {
    k += 1.0;
    p *= x/k;
    ds = p*p;
    s += ds;
  } while (ds>BESSEL_EPSILON*s);

  return s;
}


static void CalculateTwKBDTransition(float* transWin,float  alpha, int transLen)
{
  int i;
  double IBeta;
  double inc;
  double sum;
  double *wtmp;
  double w,lw;

  wtmp = (double *) calloc(transLen,sizeof(double));

  alpha *= M_PI;
  IBeta = 1.0/Bessel(alpha);

  inc = 2.0/(double)transLen;
  sum = IBeta;
  /* calculate lower half of Kaiser Bessel window */
  for(i=0; i<transLen; i++) {
    wtmp[i] = Bessel(alpha*sqrt(((double)i*inc)*(2. - (double)i*inc)))*IBeta;
    sum += wtmp[i];
  }

  sum = 1.0/sum;
  inc = 0.0;

  lw = 0.;
  /* calculate lower half of kbd window */
  for(i=0; i<transLen; i++) {
    inc += wtmp[i];
    w = sqrt(inc*sum);
    transWin[transLen-i] = 0.5*(w+lw);
    lw = w;
  }
  transWin[0] = 0.5*(1.0+lw);
  free(wtmp);

}


void calc_window_ratio( float window[], int len, int prev_len, WINDOW_SHAPE wfun_select, WINDOW_SHAPE prev_wfun_select) {

  int i,j;
  float prev_window[1024];

  calc_window(prev_window,prev_len,prev_wfun_select);

  for ( i = len - 1, j = prev_len -1 ; i >= 0 ; i--, j-- ) {
    window[i] = window[i] / prev_window[j];
  }

}


/* Calculate window */
void calc_window( float window[], int len, WINDOW_SHAPE wfun_select)
{
  int i;

  switch(wfun_select) {
  case WS_FHG:
    switch(len) {
    case BLOCK_LEN_SHORT:
      for ( i = 0 ; i < len ; i++ ) {
        window[i] = (float) usac_sine_win_128[i];
      }
      break;
    case BLOCK_LEN_LONG:
      for( i=0; i<len; i++ )
        window[i] = (float) usac_sine_win_1024[i];
      break;
    case 64:
      for( i=0; i<len; i++ )
        window[i] = (float) usac_sine_win_64[i];
      break;

    case 768:
      memcpy(window, usac_sine_win_768, len * sizeof(float));
      break;
    case 192:
      memcpy(window, usac_sine_win_192, len * sizeof(float));
      break;
    case 96:
      memcpy(window, usac_sine_win_96, len * sizeof(float));
      break;

    default:
      for( i=0; i<len; i++ )
        window[i] = sin( ((i+1)-0.5) * M_PI / (2*len) );
    }
    break;
  case WS_DOLBY:
    switch(len)
      {
      case BLOCK_LEN_SHORT_S:
        for( i=0; i<len; i++ )
          window[i] = (float) dolby_win_120[i];
        break;
      case BLOCK_LEN_SHORT:
        for ( i = 0 ; i < len ; i++ ) {
            window[i] = (float) usac_dolby_win_128[i];
        }
        break;
      case BLOCK_LEN_SHORT_SSR:
        for( i=0; i<len; i++ )
          window[i] = (float) dolby_win_32[i];
        break;
      case BLOCK_LEN_LONG_S:
        for( i=0; i<len; i++ )
          window[i] = (float) dolby_win_960[i];
        break;
      case BLOCK_LEN_LONG:
        for( i=0; i<len; i++ )
          window[i] = (float) usac_dolby_win_1024[i];
        break;
      case BLOCK_LEN_LONG_SSR:
        for( i=0; i<len; i++ )
          window[i] = (float) dolby_win_256[i];
        break;
      case 4:
        for( i=0; i<len; i++ )
          window[i] = (float) dolby_win_4[i];
        break;
      case 16:
        for( i=0; i<len; i++ )
          window[i] = (float) dolby_win_16[i];
        break;
      case 64:
        for( i=0; i<len; i++ )
          window[i] = (float) usac_kbd_win_64[i];
        break;
      case (BLOCK_LEN_LONG_S*TW_OS_FACTOR_WIN):
      case (BLOCK_LEN_LONG*TW_OS_FACTOR_WIN):
          CalculateKBDWindow(window,4.0,2*len);
        break;

      case 768:
        memcpy(window, usac_dolby_win_768, len * sizeof(float));
        break;
      case 192:
        memcpy(window, usac_dolby_win_192, len * sizeof(float));
        break;
      case 96:
        memcpy(window, usac_dolby_win_96, len * sizeof(float));
        break;

      case 48:
        CalculateKBDWindow(window, 6.0, 2 * len);
        break;

      default:
        CalculateKBDWindow(window, 6.0, 2*len);
        CommonWarning("strange window size %d",len);
        /* 	  CommonExit(1,"Unsupported window size: %d", len); */
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

#ifndef TW_M_PI
#define TW_M_PI 3.14159265358979323846
#endif

void calc_tw_window( float window[], int len, WINDOW_SHAPE wfun_select)

{
  int i;
  double inc;

  switch(wfun_select) {
    case WS_FHG:
      inc = 0.5/(double)len;
      for( i=0; i<len; i++ )
        window[i] = (float) cos(TW_M_PI*inc*(double) i);
      break;
#ifdef AAC_ELD
    case WS_FHG_LDFB:
      /* no nothing use coeffitients directly */
      break;
#endif
    case WS_DOLBY:

      CalculateTwKBDTransition(window, 4.0, len);


      break;
    default:
      CommonExit(1,"Unsupported window shape: %d", wfun_select);
      break;
  }
}

