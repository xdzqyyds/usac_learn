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
$Id: usac_mdct.c,v 1.7 2011-03-20 14:06:07 mul Exp $
*******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vector_ops.h"
#include "tf_main.h"

#ifdef AAC_ELD
#include "win512LD.h"
#include "win480LD.h"
#endif

#include "common_m4a.h"
#include "usac_tw_enc.h"
#include "usac_tw_tools.h"
#include "usac_mdct.h"
#include "usac_tw_defines.h"
#include "cnst.h"

const double dolby_win_768[768] =
{
  0.000338, 0.000503, 0.000647, 0.000784, 0.000918, 0.001052, 0.001187, 0.001325,
  0.001465, 0.001608, 0.001755, 0.001905, 0.002060, 0.002218, 0.002382, 0.002549,
  0.002721, 0.002899, 0.003081, 0.003268, 0.003460, 0.003657, 0.003860, 0.004068,
  0.004282, 0.004501, 0.004726, 0.004957, 0.005194, 0.005436, 0.005685, 0.005940,
  0.006201, 0.006469, 0.006743, 0.007023, 0.007310, 0.007604, 0.007904, 0.008211,
  0.008526, 0.008847, 0.009175, 0.009511, 0.009853, 0.010204, 0.010561, 0.010926,
  0.011299, 0.011679, 0.012067, 0.012463, 0.012867, 0.013279, 0.013699, 0.014127,
  0.014564, 0.015008, 0.015461, 0.015923, 0.016394, 0.016873, 0.017360, 0.017857,
  0.018362, 0.018877, 0.019400, 0.019933, 0.020475, 0.021026, 0.021587, 0.022157,
  0.022737, 0.023326, 0.023925, 0.024534, 0.025153, 0.025781, 0.026420, 0.027069,
  0.027727, 0.028397, 0.029076, 0.029766, 0.030466, 0.031177, 0.031899, 0.032631,
  0.033374, 0.034127, 0.034892, 0.035668, 0.036454, 0.037252, 0.038061, 0.038881,
  0.039712, 0.040555, 0.041409, 0.042275, 0.043153, 0.044042, 0.044942, 0.045855,
  0.046779, 0.047715, 0.048663, 0.049623, 0.050595, 0.051579, 0.052576, 0.053584,
  0.054605, 0.055638, 0.056684, 0.057742, 0.058813, 0.059896, 0.060991, 0.062100,
  0.063221, 0.064354, 0.065501, 0.066660, 0.067833, 0.069018, 0.070216, 0.071427,
  0.072651, 0.073888, 0.075139, 0.076402, 0.077679, 0.078969, 0.080272, 0.081588,
  0.082918, 0.084261, 0.085618, 0.086988, 0.088371, 0.089768, 0.091178, 0.092602,
  0.094039, 0.095490, 0.096955, 0.098433, 0.099924, 0.101430, 0.102949, 0.104481,
  0.106028, 0.107588, 0.109161, 0.110749, 0.112350, 0.113965, 0.115593, 0.117236,
  0.118892, 0.120561, 0.122245, 0.123942, 0.125653, 0.127378, 0.129116, 0.130868,
  0.132634, 0.134414, 0.136207, 0.138014, 0.139834, 0.141669, 0.143517, 0.145378,
  0.147253, 0.149142, 0.151044, 0.152960, 0.154889, 0.156832, 0.158788, 0.160758,
  0.162741, 0.164737, 0.166747, 0.168770, 0.170806, 0.172856, 0.174919, 0.176995,
  0.179084, 0.181186, 0.183301, 0.185429, 0.187570, 0.189724, 0.191891, 0.194071,
  0.196263, 0.198468, 0.200686, 0.202916, 0.205159, 0.207414, 0.209682, 0.211962,
  0.214254, 0.216559, 0.218875, 0.221204, 0.223545, 0.225898, 0.228263, 0.230639,
  0.233027, 0.235427, 0.237838, 0.240261, 0.242696, 0.245142, 0.247599, 0.250067,
  0.252546, 0.255036, 0.257538, 0.260050, 0.262572, 0.265106, 0.267650, 0.270205,
  0.272770, 0.275345, 0.277930, 0.280526, 0.283131, 0.285747, 0.288372, 0.291007,
  0.293652, 0.296306, 0.298969, 0.301642, 0.304324, 0.307015, 0.309715, 0.312423,
  0.315141, 0.317867, 0.320602, 0.323345, 0.326096, 0.328856, 0.331623, 0.334398,
  0.337182, 0.339973, 0.342771, 0.345577, 0.348390, 0.351211, 0.354038, 0.356873,
  0.359714, 0.362562, 0.365416, 0.368277, 0.371144, 0.374018, 0.376897, 0.379782,
  0.382673, 0.385570, 0.388472, 0.391380, 0.394292, 0.397210, 0.400133, 0.403060,
  0.405993, 0.408930, 0.411871, 0.414816, 0.417766, 0.420719, 0.423677, 0.426638,
  0.429602, 0.432570, 0.435541, 0.438516, 0.441493, 0.444473, 0.447456, 0.450441,
  0.453429, 0.456419, 0.459411, 0.462405, 0.465400, 0.468398, 0.471396, 0.474397,
  0.477398, 0.480400, 0.483404, 0.486408, 0.489413, 0.492418, 0.495423, 0.498429,
  0.501434, 0.504440, 0.507445, 0.510450, 0.513454, 0.516457, 0.519460, 0.522461,
  0.525461, 0.528460, 0.531458, 0.534453, 0.537447, 0.540439, 0.543429, 0.546417,
  0.549402, 0.552385, 0.555365, 0.558343, 0.561317, 0.564288, 0.567256, 0.570221,
  0.573182, 0.576139, 0.579092, 0.582042, 0.584987, 0.587928, 0.590865, 0.593797,
  0.596724, 0.599647, 0.602564, 0.605477, 0.608384, 0.611286, 0.614182, 0.617072,
  0.619957, 0.622836, 0.625708, 0.628575, 0.631435, 0.634289, 0.637136, 0.639976,
  0.642809, 0.645636, 0.648455, 0.651267, 0.654071, 0.656868, 0.659658, 0.662439,
  0.665213, 0.667979, 0.670736, 0.673486, 0.676226, 0.678959, 0.681683, 0.684398,
  0.687104, 0.689801, 0.692489, 0.695168, 0.697838, 0.700498, 0.703149, 0.705790,
  0.708421, 0.711043, 0.713654, 0.716256, 0.718847, 0.721428, 0.723999, 0.726559,
  0.729109, 0.731648, 0.734176, 0.736694, 0.739200, 0.741696, 0.744180, 0.746654,
  0.749116, 0.751566, 0.754005, 0.756433, 0.758849, 0.761253, 0.763646, 0.766026,
  0.768395, 0.770752, 0.773096, 0.775429, 0.777749, 0.780057, 0.782353, 0.784636,
  0.786906, 0.789165, 0.791410, 0.793643, 0.795863, 0.798070, 0.800265, 0.802446,
  0.804615, 0.806771, 0.808913, 0.811043, 0.813159, 0.815262, 0.817352, 0.819428,
  0.821492, 0.823541, 0.825578, 0.827601, 0.829610, 0.831606, 0.833589, 0.835558,
  0.837513, 0.839455, 0.841383, 0.843297, 0.845198, 0.847085, 0.848958, 0.850817,
  0.852663, 0.854495, 0.856313, 0.858117, 0.859908, 0.861684, 0.863447, 0.865196,
  0.866931, 0.868652, 0.870359, 0.872052, 0.873732, 0.875398, 0.877049, 0.878687,
  0.880311, 0.881921, 0.883518, 0.885100, 0.886669, 0.888224, 0.889765, 0.891293,
  0.892806, 0.894306, 0.895792, 0.897265, 0.898724, 0.900169, 0.901600, 0.903018,
  0.904423, 0.905814, 0.907191, 0.908555, 0.909905, 0.911242, 0.912566, 0.913876,
  0.915173, 0.916457, 0.917728, 0.918985, 0.920229, 0.921461, 0.922679, 0.923884,
  0.925076, 0.926255, 0.927422, 0.928575, 0.929716, 0.930844, 0.931960, 0.933063,
  0.934153, 0.935231, 0.936296, 0.937350, 0.938390, 0.939419, 0.940435, 0.941440,
  0.942432, 0.943412, 0.944380, 0.945337, 0.946281, 0.947214, 0.948135, 0.949045,
  0.949943, 0.950830, 0.951705, 0.952569, 0.953421, 0.954263, 0.955093, 0.955912,
  0.956721, 0.957518, 0.958305, 0.959081, 0.959846, 0.960601, 0.961345, 0.962079,
  0.962803, 0.963516, 0.964219, 0.964912, 0.965595, 0.966268, 0.966931, 0.967585,
  0.968229, 0.968863, 0.969487, 0.970102, 0.970708, 0.971305, 0.971892, 0.972470,
  0.973039, 0.973600, 0.974151, 0.974694, 0.975227, 0.975753, 0.976270, 0.976778,
  0.977278, 0.977770, 0.978253, 0.978729, 0.979196, 0.979656, 0.980107, 0.980551,
  0.980988, 0.981416, 0.981837, 0.982251, 0.982658, 0.983057, 0.983449, 0.983834,
  0.984212, 0.984583, 0.984947, 0.985305, 0.985655, 0.986000, 0.986338, 0.986669,
  0.986994, 0.987313, 0.987625, 0.987932, 0.988232, 0.988527, 0.988816, 0.989099,
  0.989376, 0.989648, 0.989914, 0.990175, 0.990430, 0.990680, 0.990925, 0.991165,
  0.991400, 0.991629, 0.991854, 0.992074, 0.992289, 0.992500, 0.992706, 0.992907,
  0.993104, 0.993297, 0.993485, 0.993669, 0.993848, 0.994024, 0.994196, 0.994363,
  0.994527, 0.994687, 0.994843, 0.994995, 0.995144, 0.995289, 0.995430, 0.995569,
  0.995703, 0.995835, 0.995963, 0.996088, 0.996209, 0.996328, 0.996444, 0.996556,
  0.996666, 0.996773, 0.996877, 0.996978, 0.997077, 0.997173, 0.997267, 0.997357,
  0.997446, 0.997532, 0.997615, 0.997697, 0.997776, 0.997853, 0.997927, 0.998000,
  0.998070, 0.998138, 0.998205, 0.998269, 0.998332, 0.998392, 0.998451, 0.998508,
  0.998563, 0.998617, 0.998669, 0.998719, 0.998768, 0.998815, 0.998861, 0.998905,
  0.998948, 0.998990, 0.999030, 0.999068, 0.999106, 0.999142, 0.999177, 0.999211,
  0.999244, 0.999275, 0.999306, 0.999335, 0.999364, 0.999391, 0.999417, 0.999443,
  0.999467, 0.999491, 0.999514, 0.999536, 0.999557, 0.999577, 0.999597, 0.999615,
  0.999634, 0.999651, 0.999668, 0.999684, 0.999699, 0.999714, 0.999728, 0.999741,
  0.999754, 0.999767, 0.999779, 0.999790, 0.999801, 0.999812, 0.999822, 0.999831,
  0.999841, 0.999849, 0.999858, 0.999866, 0.999873, 0.999880, 0.999887, 0.999894,
  0.999900, 0.999906, 0.999912, 0.999917, 0.999922, 0.999927, 0.999932, 0.999936,
  0.999940, 0.999944, 0.999948, 0.999951, 0.999955, 0.999958, 0.999961, 0.999964,
  0.999966, 0.999969, 0.999971, 0.999973, 0.999975, 0.999977, 0.999979, 0.999981,
  0.999982, 0.999984, 0.999985, 0.999987, 0.999988, 0.999989, 0.999990, 0.999991,
  0.999992, 0.999993, 0.999993, 0.999994, 0.999995, 0.999995, 0.999996, 0.999996,
  0.999997, 0.999997, 0.999998, 0.999998, 0.999998, 0.999998, 0.999999, 0.999999,
  0.999999, 0.999999, 0.999999, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000
};

const double dolby_win_96[96] =
{
  0.000051, 0.000166, 0.000355, 0.000643, 0.001058, 0.001636, 0.002413, 0.003434,
  0.004745, 0.006400, 0.008454, 0.010968, 0.014006, 0.017634, 0.021922, 0.026939,
  0.032755, 0.039440, 0.047061, 0.055684, 0.065368, 0.076169, 0.088136, 0.101309,
  0.115721, 0.131394, 0.148339, 0.166557, 0.186035, 0.206749, 0.228661, 0.251721,
  0.275863, 0.301013, 0.327081, 0.353968, 0.381563, 0.409748, 0.438395, 0.467369,
  0.496534, 0.525746, 0.554862, 0.583741, 0.612241, 0.640227, 0.667568, 0.694143,
  0.719837, 0.744549, 0.768186, 0.790671, 0.811940, 0.831942, 0.850642, 0.868017,
  0.884062, 0.898783, 0.912199, 0.924343, 0.935258, 0.944996, 0.953620, 0.961197,
  0.967800, 0.973506, 0.978394, 0.982543, 0.986032, 0.988937, 0.991330, 0.993282,
  0.994855, 0.996108, 0.997095, 0.997861, 0.998448, 0.998892, 0.999222, 0.999463,
  0.999637, 0.999760, 0.999844, 0.999902, 0.999940, 0.999964, 0.999979, 0.999989,
  0.999994, 0.999997, 0.999999, 0.999999, 1.000000, 1.000000, 1.000000, 1.000000
};


extern const double dolby_win_1024[1024]; /* symbol already in imdct.o */
extern const double dolby_win_960[960]; /* symbol already in imdct.o */
extern const double dolby_win_256[256]; /* symbol already in imdct.o */
extern const double dolby_win_128[128]; /* symbol already in imdct.o */
extern const double dolby_win_120[120]; /* symbol already in imdct.o */
extern const double dolby_win_32[32]; /* symbol already in imdct.o */
extern const float ShortWindowSine64[64];
extern const double dolby_win_16[16]; /* symbol already in imdct.o */
extern const double dolby_win_4[4]; /* symbol already in imdct.o */
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#ifndef TW_M_PI
#define TW_M_PI 3.14159265358979323846
#endif


static const double zero = 0;


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



static void CalculateTwKBDTransition(double* transWin,double  alpha, int transLen)
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




#ifndef TW_M_PI
#define TW_M_PI 3.14159265358979323846
#endif



static void calc_tw_window( double window[], int len, WINDOW_SHAPE wfun_select)

{
  int i;
  double inc;

  switch(wfun_select) {
    case WS_FHG:
      inc = 0.5/(double)len;
      for( i=0; i<len; i++ )
        window[i] = cos(TW_M_PI*inc*(double) i);
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
void calc_window_db( double window[], int len, WINDOW_SHAPE wfun_select)

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
      case 768:
        for( i=0; i<len; i++ )
          window[i] = dolby_win_768[i];
        break;        
      case 96:
        for( i=0; i<len; i++ )
          window[i] = dolby_win_96[i];
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


static void calc_window_ratio_db( double window[], int len, int prev_len, WINDOW_SHAPE wfun_select, WINDOW_SHAPE prev_wfun_select) {

  int i,j;
  double prev_window[1024];

  calc_window_db(prev_window,prev_len,prev_wfun_select);

  for ( i = len - 1, j = prev_len -1 ; i >= 0 ; i--, j-- ) {
    window[i] = window[i] / prev_window[j];
  }

}
/* %%%%%%%%%%%%%%%%%% IMDCT - STUFF %%%%%%%%%%%%%%%%*/

#define MAX_SHIFT_LEN_LONG 4096





void buffer2fd(
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
               int              num_short_win,     /* number of short windows per frame */
               int              twMdct,            /* TW-MDCT flag */
               float            sample_pos[],      /* TW sample positions */
               float            tw_trans_len[],    /* TW transition lengths */
               int              tw_start_stop[]   /* TW window borders */
               )
{
  double          transf_buf[ 3*MAX_SHIFT_LEN_LONG ] = {0};
  double          windowed_buf[ 2*MAX_SHIFT_LEN_LONG ] = {0};
  double          *p_o_buf;
  int             k;

  double           window_long[MAX_SHIFT_LEN_LONG];
  double           window_long_prev[MAX_SHIFT_LEN_LONG];
  double           window_short[MAX_SHIFT_LEN_LONG];
  double           window_short_prev[MAX_SHIFT_LEN_LONG];
  double           window_medium_prev[MAX_SHIFT_LEN_LONG];
  double           window_medium[MAX_SHIFT_LEN_LONG];
  double           window_extra_short[MAX_SHIFT_LEN_LONG];
  double           os_window[MAX_SHIFT_LEN_LONG*TW_OS_FACTOR_WIN];
  double           os_window_prev[MAX_SHIFT_LEN_LONG*TW_OS_FACTOR_WIN];
  double           *window_short_prev_ptr;
  double           *window_medium_prev_ptr;

  int nflat_ls    = (nlong-nshort)/ 2;
  int transfak_ls =  nlong/nshort;
  static int firstTime=1;
  int lfac;
  int overlap_fac = 1;
  int ntrans_ls = nshort;

  if ( twMdct ) {
    overlap_fac = 2;
  }

  lfac = nlong/8;
  if ( previousMode==1 ||nextMode==1 ) {
    if ( windowSequence  == EIGHT_SHORT_SEQUENCE) {
      lfac = nlong/16;
    }
    else {
      lfac = nlong/8;
    }
    nflat_ls    = (nlong-lfac*2)/ 2;
    ntrans_ls = lfac*2;
  }
#ifdef AAC_ELD
  if (wfun_select == WS_FHG_LDFB) overlap_fac = 3;
#endif

  window_short_prev_ptr = window_short_prev ;
  window_medium_prev_ptr = window_medium_prev ;

  if( (nlong%nshort) || (nlong > MAX_SHIFT_LEN_LONG) || (nshort > MAX_SHIFT_LEN_LONG/2) ) {
    CommonExit( 1, "mdct_analysis: Problem with window length" ); }
  if( windowSequence==EIGHT_SHORT_SEQUENCE
      && ( (num_short_win <= 0) || (num_short_win > transfak_ls) ) ) {
    CommonExit( 1, "mdct_analysis: Problem with number of short windows" );  }

  if ( twMdct == 1 ) {
    /* oversampled windows for TW */
    calc_tw_window (os_window,      nlong*TW_OS_FACTOR_WIN, wfun_select);
    calc_tw_window (os_window_prev, nlong*TW_OS_FACTOR_WIN, wfun_select_prev);
    calc_window_db( window_extra_short,      nshort/2, wfun_select );
  }
  else {
    calc_window_db( window_long,      nlong, wfun_select );
    calc_window_db( window_long_prev, nlong, wfun_select_prev );
    calc_window_db( window_short,      nshort, wfun_select );
    calc_window_db( window_short_prev, nshort, wfun_select_prev );
    /* lcm */
    calc_window_db( window_extra_short, nshort/2, wfun_select );
    calc_window_db( window_medium_prev, 2*lfac, wfun_select_prev );
    calc_window_db( window_medium,      2*lfac, wfun_select );
  }
  if (overlap_select != NON_OVERLAPPED_MODE) {
    /* create / shift old values */
    /* We use p_overlap here as buffer holding the last frame time signal*/
    if (firstTime){
      firstTime=0;
      vcopy_db( &zero, transf_buf, 0, 1, nlong*overlap_fac );
    }
    else

#ifdef AAC_ELD
      if (wfun_select != WS_FHG_LDFB)
#endif
        {
          vcopy_db( p_overlap, transf_buf, 1, 1, nlong*overlap_fac );
          /* Append new data */
          vcopy_db( p_in_data, transf_buf+nlong*overlap_fac, 1, 1, nlong );
          vcopy_db( transf_buf + nlong, p_overlap,        1, 1, nlong*overlap_fac );
        }
  }
  else { /* overlap_select == NON_OVERLAPPED_MODE */
    vcopy_db( p_in_data, transf_buf, 1, 1, (overlap_fac+1)*nlong);
  }

  /* Set ptr to transf-Buffer */
  p_o_buf = transf_buf;

  if ( twMdct == 1 ) {
    double resamp_buf[MAX_SHIFT_LEN_LONG];

    switch (windowSequence) {
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
    case STOP_START_SEQUENCE:
    case EIGHT_SHORT_SEQUENCE:
      break;
    default:
      CommonExit( 1, "mdct_synthesis: Unknown window type (2)" );
    }

    tw_resamp_enc(p_o_buf,
                  0.5f,
                  3*nlong,
                  tw_start_stop[1]-tw_start_stop[0]+1,
                  sample_pos+TW_IPLEN2S+tw_start_stop[0],
                  resamp_buf+tw_start_stop[0]);

    if ( windowSequence != EIGHT_SHORT_SEQUENCE ) {
      tw_windowing_long_enc(resamp_buf,
                            windowed_buf,
                            tw_start_stop[0],
                            tw_start_stop[1],
                            nlong,
                            tw_trans_len[0],
                            tw_trans_len[1],
                            os_window_prev,
                            os_window);
      mdct(windowed_buf,p_out_mdct,2*nlong);
    }
    else {
      tw_windowing_short_enc(resamp_buf,
                             windowed_buf,
                             tw_start_stop[0],
                             tw_start_stop[1],
                             tw_trans_len[0],
                             tw_trans_len[1],
                             os_window_prev,
                             os_window);
      for (k=num_short_win-1; k-->=0; ) {
        mdct( windowed_buf , p_out_mdct, 2*nshort );
        p_out_mdct += nshort;
        p_o_buf    += 2*nshort;
        window_short_prev_ptr = window_short;
      }

    }


  }
  else {
    /* Separate action for each Block Type */
    switch( windowSequence ) {
    case ONLY_LONG_SEQUENCE :
#ifdef AAC_ELD
      if (wfun_select == WS_FHG_LDFB) {
        double *p_ldwin = (nlong==512) ? WIN512LD : WIN480LD;
        int winoff = (nlong==512) ? NUM_LD_COEF_512 : NUM_LD_COEF_480;
        /*vcopy_db ( windowed_buf, w_buf, 1,1, nlong*3);*/
        vcopy_db ( p_in_data, &p_o_buf[nlong*3], 1,1,nlong);
        vcopy_db ( p_o_buf+nlong, p_overlap, 1,1, nlong*3);

        vmult_db( p_o_buf, p_ldwin+winoff-1,p_o_buf, 1, -1,  1, nlong*4 );
        ldfb(p_o_buf, p_out_mdct, 2*nlong);
      }
      else
        {
#endif
          vmult_db( p_o_buf, window_long_prev, windowed_buf,       1, 1,  1, nlong );
          vmult_db( p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, 1, -1, 1, nlong );
          mdct( windowed_buf, p_out_mdct, 2*nlong );
#ifdef AAC_ELD
        }
#endif
      break;

    case LONG_START_SEQUENCE:
      vmult_db( p_o_buf, window_long_prev, windowed_buf, 1, 1, 1, nlong );
      vcopy_db( p_o_buf+nlong, windowed_buf+nlong, 1, 1, nflat_ls );
      if (nextMode==1){
          vmult_db( p_o_buf+nlong+nflat_ls, window_medium+2*lfac-1, windowed_buf+nlong+nflat_ls, 1, -1, 1, 2*lfac );
          vcopy_db( &zero, windowed_buf+2*nlong-1, 0, -1, nflat_ls );
      }else{
    	  vmult_db( p_o_buf+nlong+nflat_ls, window_short+nshort-1, windowed_buf+nlong+nflat_ls, 1, -1, 1, nshort );
    	  vcopy_db( &zero, windowed_buf+2*nlong-1, 0, -1, nflat_ls );
      }
      mdct( windowed_buf, p_out_mdct, 2*nlong );
      break;

    case LONG_STOP_SEQUENCE :
      if (previousMode==1) {
        vcopy_db( &zero, windowed_buf, 0, 1, (nlong-2*lfac)/ 2 );
        vmult_db( p_o_buf+nflat_ls, window_medium_prev_ptr, windowed_buf+nflat_ls, 1, 1, 1, 2*lfac );
        vcopy_db( p_o_buf+nflat_ls+2*lfac, windowed_buf+nflat_ls+2*lfac, 1, 1, nflat_ls);
      }else{
        vcopy_db( &zero, windowed_buf, 0, 1, nflat_ls );
        vmult_db( p_o_buf+nflat_ls, window_short_prev_ptr, windowed_buf+nflat_ls, 1, 1, 1, nshort );
        vcopy_db( p_o_buf+nflat_ls+nshort, windowed_buf+nflat_ls+nshort, 1, 1, nflat_ls );
      }
      vmult_db( p_o_buf+nlong, window_long+nlong-1, windowed_buf+nlong, 1, -1, 1, nlong );
      mdct( windowed_buf, p_out_mdct, 2*nlong );
      break;
     
    case STOP_START_SEQUENCE :
      if (previousMode==1) {
        vcopy_db( &zero, windowed_buf, 0, 1, (nlong-2*lfac)/ 2 );
        vmult_db( p_o_buf+(nlong-2*lfac)/ 2 , window_medium_prev_ptr, windowed_buf+(nlong-2*lfac)/ 2 , 1, 1, 1, 2*lfac );
        vcopy_db( p_o_buf+(nlong-2*lfac)/ 2 +2*lfac, windowed_buf+(nlong-2*lfac)/ 2 +2*lfac, 1, 1, (nlong-2*lfac)/ 2 );
      }else{
        vcopy_db( &zero, windowed_buf, 0, 1, (nlong-nshort)/ 2 );
        vmult_db( p_o_buf+(nlong-nshort)/ 2 , window_short_prev_ptr, windowed_buf+(nlong-nshort)/ 2 , 1, 1, 1, nshort );
        vcopy_db( p_o_buf+(nlong-nshort)/ 2 +nshort, windowed_buf+(nlong-nshort)/ 2 +nshort, 1, 1, (nlong-nshort)/ 2  );
      }
      
      if (nextMode==1){
        vcopy_db( p_o_buf+nlong, windowed_buf+nlong, 1, 1, (nlong-2*lfac)/ 2 );
        vmult_db( p_o_buf+nlong+(nlong-2*lfac)/ 2 , window_medium+2*lfac-1, windowed_buf+nlong+(nlong-2*lfac)/ 2 , 1, -1, 1, 2*lfac );
        vcopy_db( &zero, windowed_buf+2*nlong-1, 0, -1, (nlong-2*lfac)/ 2 );
      }else{
        vcopy_db( p_o_buf+nlong, windowed_buf+nlong, 1, 1, (nlong-nshort)/ 2 );
        vmult_db( p_o_buf+nlong+(nlong-nshort)/ 2, window_short+nshort-1, windowed_buf+nlong+(nlong-nshort)/ 2, 1, -1, 1, nshort );
        vcopy_db( &zero, windowed_buf+2*nlong-1, 0, -1, (nlong-nshort)/ 2 );
      }
      mdct( windowed_buf, p_out_mdct, 2*nlong );
      break;
        
    case EIGHT_SHORT_SEQUENCE :
      if (previousMode==1) {
        if (overlap_select != NON_OVERLAPPED_MODE) {
          p_o_buf += nflat_ls;
        }
        vmult_db( p_o_buf,  window_medium_prev_ptr, windowed_buf,1, 1,  1, 2*lfac );
        vmult_db( p_o_buf+2*lfac, window_short+nshort-1, windowed_buf+2*lfac, 1, -1, 1, nshort );
        mdct( windowed_buf, p_out_mdct, 2*nshort );
      
        p_out_mdct += nshort;
        p_o_buf    += (overlap_select != NON_OVERLAPPED_MODE) ? nshort : 2*nshort;
        window_short_prev_ptr = window_short;
      
        for (k=num_short_win-1; k-->=2; ) {
          vmult_db( p_o_buf,  window_short_prev_ptr, windowed_buf,1, 1,  1, nshort );
          vmult_db( p_o_buf+nshort, window_short+nshort-1, windowed_buf+nshort, 1, -1, 1, nshort );
          mdct( windowed_buf, p_out_mdct, 2*nshort );
        
          p_out_mdct += nshort;
          p_o_buf    += (overlap_select != NON_OVERLAPPED_MODE) ? nshort : 2*nshort;
          window_short_prev_ptr = window_short;
        }
      } else {
        if (overlap_select != NON_OVERLAPPED_MODE) {
          p_o_buf += (nlong-nshort)/ 2;
        }
      
        for (k=num_short_win-1; k-->=1; ) {
          vmult_db( p_o_buf,  window_short_prev_ptr, windowed_buf,1, 1,  1, nshort );
          vmult_db( p_o_buf+nshort, window_short+nshort-1, windowed_buf+nshort, 1, -1, 1, nshort );
          mdct( windowed_buf, p_out_mdct, 2*nshort );
        
          p_out_mdct += nshort;
          p_o_buf    += (overlap_select != NON_OVERLAPPED_MODE) ? nshort : 2*nshort;
          window_short_prev_ptr = window_short;
        }
      }
      if (nextMode==1){
        vmult_db( p_o_buf,  window_short_prev_ptr, windowed_buf,1, 1,  1, nshort );
        vmult_db( p_o_buf+nshort, window_medium+2*lfac-1, windowed_buf+nshort, 1, -1, 1, 2*lfac );
        mdct( windowed_buf, p_out_mdct, 2*nshort );
        
        p_out_mdct += 2*lfac;
        p_o_buf    += (overlap_select != NON_OVERLAPPED_MODE) ? 2*lfac : 2*(2*lfac);
        window_short_prev_ptr = window_short;
      }else{
        vmult_db( p_o_buf,  window_short_prev_ptr, windowed_buf,1, 1,  1, nshort );
        vmult_db( p_o_buf+nshort, window_short+nshort-1, windowed_buf+nshort, 1, -1, 1, nshort );
        mdct( windowed_buf, p_out_mdct, 2*nshort );
        
        p_out_mdct += nshort;
        p_o_buf    += (overlap_select != NON_OVERLAPPED_MODE) ? nshort : 2*nshort;
        window_short_prev_ptr = window_short;
      }
      break;

    default :
      CommonExit( 1, "mdct_synthesis: Unknown window type (2)" );
    }
  }
 
}

/***********************************************************************************************/




