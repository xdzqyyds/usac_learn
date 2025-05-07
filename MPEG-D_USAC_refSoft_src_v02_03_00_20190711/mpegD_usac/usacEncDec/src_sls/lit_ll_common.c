/***********

This software module was originally developed by

Rongshan Yu (Institute for Infocomm Research)

and edited by

Michael Haertl (Fraunhofer IIS)

in the course of development of the MPEG-4 extension 3 
audio scalable lossless (SLS) coding . This software module is an 
implementation of a part of one or more MPEG-4 Audio tools as 
specified by the MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works. 

Copyright 2003.  

***********/
/******************************************************************** 
/
/ filename : lit_ll_common.c
/ project : MPEG-4 audio scalable lossless coding
/    

*/

#include "all.h"
#include "lit_ll_common.h"


short scale_pcm[4] = {-8, 0, 4, 8}; /* 8,16,20,24  bit PCM input */   /* bug */


void calc_sfb_offset_table(LIT_LL_quantInfo *quantInfo,
                           /* Info * info, */
                           int osf)
{
  int grp, sfb;

  quantInfo->num_window_groups = quantInfo->info->num_groups;
  quantInfo->num_sfb = quantInfo->info->sfb_per_sbk[0];
  if (quantInfo->info->islong) {
    quantInfo->num_osf_sfb = (osf-1)*16;
    quantInfo->osf_sfb_width = 64;
  } else {
    quantInfo->num_osf_sfb = (osf-1)*4;
    quantInfo->osf_sfb_width = 32;
  }

  quantInfo->aac_sfb_offset[0] = 0;
  quantInfo->sls_sfb_offset[0] = 0;


  for (grp=0; grp<quantInfo->num_window_groups; grp++) {
    for (sfb=0; sfb<quantInfo->num_sfb; sfb++) {
      int sfb_aac = grp * quantInfo->num_sfb + sfb;
      quantInfo->aac_sfb_offset[sfb_aac+1] = quantInfo->info->bk_sfb_top[sfb_aac];
    }
  }

  for (grp=0; grp<quantInfo->num_window_groups; grp++) {
    for (sfb=0; sfb<quantInfo->num_sfb; sfb++) {
      int sfb_aac = grp * quantInfo->num_sfb + sfb;
      int sfb_sls = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;

      quantInfo->sls_sfb_offset[sfb_sls+1] = 
        quantInfo->sls_sfb_offset[sfb_sls]
        + quantInfo->aac_sfb_offset[sfb_aac+1] 
        - quantInfo->aac_sfb_offset[sfb_aac];     
    }
    for (sfb=0; sfb<quantInfo->num_osf_sfb; sfb++) {
      int sfb_sls = (grp+1) * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
      quantInfo->sls_sfb_offset[sfb_sls+1] =
        quantInfo->sls_sfb_offset[sfb_sls]
        + quantInfo->info->group_len[grp] * quantInfo->osf_sfb_width;
    }
  }

  for (grp=0; grp<quantInfo->num_window_groups; grp++) {
    for (sfb=0; sfb<quantInfo->num_sfb; sfb++) {
      int sfb_aac = grp * quantInfo->num_sfb + sfb;
      quantInfo->aac_sfb_width[sfb_aac] = 
        quantInfo->aac_sfb_offset[sfb_aac+1] 
        - quantInfo->aac_sfb_offset[sfb_aac];
    }
  }

  for (grp=0; grp<quantInfo->num_window_groups; grp++) {
    for (sfb=0; sfb<quantInfo->num_sfb+quantInfo->num_osf_sfb; sfb++) {
      int sfb_sls = grp * quantInfo->num_sfb + grp * quantInfo->num_osf_sfb + sfb;
      quantInfo->sls_sfb_width[sfb_sls] = 
        quantInfo->sls_sfb_offset[sfb_sls+1] 
        - quantInfo->sls_sfb_offset[sfb_sls];
    }
  }

}
