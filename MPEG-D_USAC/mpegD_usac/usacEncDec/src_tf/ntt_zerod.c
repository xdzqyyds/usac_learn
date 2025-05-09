/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Shigeki Sagayama (NTT)                                                  */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/* in the course of development of the                                       */
/* MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.        */
/* This software module is an implementation of a part of one or more        */
/* MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio */
/* standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards   */
/* free license to this software module or modifications thereof for use in  */
/* hardware or software products claiming conformance to the MPEG-2 AAC/     */
/* MPEG-4 Audio  standards. Those intending to use this software module in   */
/* hardware or software products are advised that this use may infringe      */
/* existing patents. The original developer of this software module and      */
/* his/her company, the subsequent editors and their companies, and ISO/IEC  */
/* have no liability for use of this software module or modifications        */
/* thereof in an implementation. Copyright is not released for non           */
/* MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer       */
/* retains full right to use the code for his/her  own purpose, assign or    */
/* donate the code to a third party and to inhibit third party from using    */
/* the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.             */
/* This copyright notice must be included in all copies or derivative works. */
/* Copyright (c)1996.                                                        */
/*****************************************************************************/



/* --- ntt_zerod ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   divroutine  library     # nn    *
*               coded by S.Sagayama,                3/10/1987    *
******************************************************************
 ( C version coded by S.Sagayama, 3/10/1987)

   description:
     * array arithmetic :  xx = 0
       i.e. xx[i]=0 for i=0,n-1

   synopsis:
          -----------
          ntt_zerod(n,xx)
          -----------

    n      : dimension of data
    xx[.]  : output data array (double)
*/

#include "ntt_conf.h"

#define FAST 1

/*** MODULE FUNCTION PROTOTYPE ***/

#if FAST
void ntt_zerod(int n, double xx[])
{ while(n-- != 0) *(xx++) = 0.0; }
#else
void ntt_zerod(int n,double xx[])
{ int i; for(i=0;i<n;i++) xx[i]=0.0; }
#endif
