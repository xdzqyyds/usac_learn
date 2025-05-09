/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
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
/* Copyright (c)1997.                                                        */
/*****************************************************************************/

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "ntt_conf.h"
#include "ntt_scale_conf.h"

void ntt_scale_message(int iscl, int ntt_IBPS, int ntt_IBPS_SCL[])
{
  double total;
  int    ii;

  total = (double)ntt_IBPS;
  for (ii=0; ii<=iscl; ii++) total += ntt_IBPS_SCL[ii];

  printf("<<>><<>> SCALABLE LAYER %d:\n", iscl+1);
  printf("### Bitrate:               %8.1f kbit/s\n",
	  (ntt_IBPS_SCL[iscl]  /**ntt_N_SUP*/  )/1000. );
  printf("### Total bitrate:         %8.1f kbit/s\n",
	  (total  /**ntt_N_SUP */)/1000. );
/*
printf("### Cutoff:                %5.4f\n",
	  ntt_BAND_UPPER_SCL[iscl]);
*/
}
