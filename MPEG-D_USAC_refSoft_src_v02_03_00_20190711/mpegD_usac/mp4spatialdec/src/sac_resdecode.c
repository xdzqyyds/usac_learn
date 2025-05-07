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

#include "sac_resdecode.h"
#include "sac_reshuffdec.h"

#include <assert.h>

static int sampling_boundarys[16] = {
  92017, 75132, 55426, 46009, 37566, 27713, 23004, 18783,
  13856, 11502, 9391, 0, 0, 0, 0, 0
};


static void applyTools(Float* coef,
                       TNS_frame_info* tns,
                       byte max_sfb,
                       Info* info){

  int i, j, ch=0;

  
  for (i=j=0; i<tns->n_subblocks; i++) {
    if (debug['T']) {
      s_print_tns( &(tns->info[i]));
    }

    s_tns_decode_subblock(&coef[j],
                        max_sfb,
                        info->sbk_sfb_top[i],
                        info->islong,
                        &(tns->info[i]) );

    j += info->bins_per_sbk[i];
  }

}

BLOCKTYPE decodeIcs(spatialDec* self,
                    int channel,
                    int channelFrame,
                    int extra) {

  int i;

  RESIDUAL_DATA_INTERN* rdi =
    (RESIDUAL_DATA_INTERN*) self->bsFrame[0].resData.residualDataIntern;

  RDI_CHANNEL* rdic = &(rdi->rdiChannel[channel]);

  s_huffdecode(ID_SCE,
               &(rdic->win),
               &(rdic->wndShape),
               rdic->cb_map,
               rdic->factors,
               rdic->group,
               &(rdic->hasmask),
               rdic->mask,
               &(rdic->max_sfb),
               &(rdic->tns),
               rdic->coef,
               &(rdic->info),
               &(rdi->winmap[0])
               );

  applyTools(rdic->coef,
             &rdic->tns,
             rdic->max_sfb,
             &rdic->info);

  
  self->resMdctPresent[channel][channelFrame] = 1;

  if (extra==1) 
	  assert(self->resBlocktype[channel][channelFrame] == rdic->win);
  else
	  self->resBlocktype[channel][channelFrame] = rdic->win;

  for (i=0; i< BLOCK_LEN_LONG; i++) {
    self->resMDCT[channel][channelFrame][extra*MAX_MDCT_COEFFS + i]=
      (float)rdic->coef[i]; 
  }

  return rdic->win;
}

BLOCKTYPE decodeCpe(spatialDec* self,
                    int channel,
                    int channelFrame,
                    int extra) {

  int temp;
  BLOCKTYPE win1, win2;

  temp = s_GetBits(self->bitstream, 4); 
  temp = s_GetBits(self->bitstream, 1); 

  assert(temp == 0);

  win1 = decodeIcs(self, channel, channelFrame, extra);
  win2 = decodeIcs(self, channel + 1, channelFrame, extra);

  assert(win1 == win2);

  return win1;
}

void initResidualCore(spatialDec* self) {

  RESIDUAL_DATA_INTERN** rdi =
    (RESIDUAL_DATA_INTERN**) &(self->bsFrame[0].resData.residualDataIntern);

  *rdi = (RESIDUAL_DATA_INTERN*) calloc(1, sizeof(RESIDUAL_DATA_INTERN));


  if (self->residualCoding != 0) {
    s_huffbookinit(self->bsConfig.bsResidualSamplingFreqIndex);
  } else {
    if (self->arbitraryDownmix == 2) {
      s_huffbookinit(self->bsConfig.bsArbitraryDownmixResidualSamplingFreqIndex);
    }
  }

  (*rdi)->winmap[0] = s_win_seq_info[ONLYLONG_WINDOW];
  (*rdi)->winmap[1] = s_win_seq_info[ONLYLONG_WINDOW];
  (*rdi)->winmap[2] = s_win_seq_info[EIGHTSHORT_WINDOW];
  (*rdi)->winmap[3] = s_win_seq_info[ONLYLONG_WINDOW];


  return;
}
