/*
This software module was originally developed by
Toshiyuki Nomura (NEC Corporation)
and edited by

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
*/
/*
 *	MPEG-4 Audio Verification Model (LPC-ABS Core)
 *	
 *	LPF Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>

#include "nec_abs_proto.h"
#include "nec_abs_const.h"

static float nec_lpf_coef[81] = {
 (float)  4.8749828e-01,  (float)  3.1800737e-01,  (float)  1.2479170e-02,  (float) -1.0519724e-01,
 (float) -1.2413947e-02,  (float)  6.2158761e-02,  (float)  1.2305790e-02,  (float) -4.3381345e-02,
 (float) -1.2155302e-02,  (float)  3.2701355e-02,  (float)  1.1964675e-02,  (float) -2.5712499e-02,
 (float) -1.1735021e-02,  (float)  2.0722269e-02,  (float)  1.1468040e-02,  (float) -1.6942294e-02,
 (float) -1.1166773e-02,  (float)  1.3953708e-02,  (float)  1.0830312e-02,  (float) -1.1527189e-02,
 (float) -1.0467123e-02,  (float)  9.4936193e-03,  (float)  1.0078949e-02,  (float) -7.7819596e-03,
 (float) -9.6619784e-03,  (float)  6.3144599e-03,  (float)  9.2270602e-03,  (float) -5.0434647e-03,
 (float) -8.7764313e-03,  (float)  3.9383742e-03,  (float)  8.3117364e-03,  (float) -2.9767646e-03,
 (float) -7.8363655e-03,  (float)  2.1406228e-03,  (float)  7.3546181e-03,  (float) -1.4144288e-03,
 (float) -6.8693320e-03,  (float)  7.8488294e-04,  (float)  6.3827917e-03,  (float) -2.4118577e-04,
 (float) -5.9022546e-03,  (float) -2.2182016e-04,  (float)  5.4275040e-03,  (float)  6.1605167e-04,
 (float) -4.9611729e-03,  (float) -9.4238127e-04,  (float)  4.5077288e-03,  (float)  1.2098297e-03,
 (float) -4.0681230e-03,  (float) -1.4226218e-03,  (float)  3.6454255e-03,  (float)  1.5861505e-03,
 (float) -3.2416364e-03,  (float) -1.7055635e-03,  (float)  2.8579075e-03,  (float)  1.7850303e-03,
 (float) -2.4965477e-03,  (float) -1.8304865e-03,  (float)  2.1577244e-03,  (float)  1.8438554e-03,
 (float) -1.8417709e-03,  (float) -1.8308485e-03,  (float)  1.5513254e-03,  (float)  1.7938703e-03,
 (float) -1.2841625e-03,  (float) -1.7371747e-03,  (float)  1.0418853e-03,  (float)  1.6634709e-03,
 (float) -8.2487722e-04,  (float) -1.5762212e-03,  (float)  6.3187871e-04,  (float)  1.4784085e-03,
 (float) -4.6220103e-04,  (float) -1.3731783e-03,  (float)  3.1496653e-04,  (float)  1.2627709e-03,
 (float) -1.8964791e-04,  (float) -1.1482980e-03,  (float)  1.5850931e-04,  (float)  5.1561088e-03,
 (float) -9.2470890e-05
  };

#define NEC_LPF_BUFLEN	(2*(NEC_LPF_DELAY))

void nec_lpf_down( float xin[], float xout[], int len )
{
   int		i, j, k;
   float	*x;
   static float	buf[NEC_LPF_BUFLEN];
   static int	flag = 0;


   if ( flag == 0 ) {
      for ( i = 0; i < NEC_LPF_BUFLEN; i++ ) buf[i] = 0.0;
      flag = 1;
   }

   if ((x = (float *)calloc(NEC_LPF_BUFLEN+len,sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lpf \n");
      exit(1);
   }
   
   for ( i = 0; i < NEC_LPF_BUFLEN; i++ ) x[i] = buf[i];
   for ( i = 0; i < len; i++ ) x[NEC_LPF_BUFLEN+i] = xin[i];

   for ( i = 0, k = 0; i < len; i+=2, k++ ) {
      xout[k] = nec_lpf_coef[0] * x[NEC_LPF_DELAY+i];
      for ( j = 1; j <= NEC_LPF_DELAY; j++ ) {
	 xout[k] += ( nec_lpf_coef[j] *
		     (x[NEC_LPF_DELAY+i-j]+x[NEC_LPF_DELAY+i+j]));
      }
   }

   for ( i = 0; i < NEC_LPF_BUFLEN; i++ ) buf[i] = x[len+i];

   free( x );

}
