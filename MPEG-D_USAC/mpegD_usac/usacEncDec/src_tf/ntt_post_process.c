/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-08-25,                                      */
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

/* 18-apr-97   NI   generalized the module */
/* 25-aug-97   NI   bugfixes */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"

void ntt_post_process(/* Parameters */
		      int    nfr,
		      int    nsf,
		      double band_lower,
		      double band_upper,
		      /* Input */
		      double spectrum[],
		      /* Output */
		      double out_spectrum[])
{
  /*--- Variabels ---*/
  int isf, subtop, ismp, start, stop, width;
  float ftmp;

  /*--- Band limit ---*/
  for (isf=0; isf<nsf; isf++){
    subtop = isf * nfr;
    ftmp   =  (float)band_lower;
    ftmp   *= (float)nfr;
    start = (int)ftmp;
    ftmp   =  (float)band_upper-(float)band_lower;
    ftmp   *= (float)nfr;
    width  = (int)ftmp;
    stop  = start+width;
    for (ismp=0; ismp<start; ismp++)
      out_spectrum[ismp+subtop] = 0.;
    for (ismp=start; ismp<stop; ismp++)
      out_spectrum[ismp+subtop] = spectrum[ismp+subtop];
    for (ismp=stop; ismp<nfr; ismp++)
      out_spectrum[ismp+subtop] = 0.;
  }

}
