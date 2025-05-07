/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Bodo Teichmann      (Fraunhofer IIS)

and edited by
Bernhard Grill      (Fraunhofer IIS)
Takashi koike       (Sony Corporation)
Markus Werner       (SEED / Software Development Karlsruhe) 
Olaf Kaehler        (Fraunhofer IIS)

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
$Id: scal_dec_frame.c,v 1.1.1.1 2009-05-29 14:10:17 mul Exp $
*******************************************************************************/

#include <memory.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "allHandles.h"
#include "block.h"

#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */

#include "obj_descr.h"           /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "dec_lpc.h"

#include "sam_tns.h"             /* structs */
#include "mod_buf.h"
#include "port.h"
#include "flex_mux.h"
#include "lpc_common.h"
#include "dec_tf.h"
#include "bitstream.h"
#include "common_m4a.h"
#include "mp4ifc.h"
/* ---  AAC --- */
#include "aac.h"
#include "huffdec2.h"
#include "nok_lt_prediction.h"
#include "scal_dec.h"
#include "plotmtv.h"
#include "sam_dec.h"    /* HP 980211 */
#include "flex_mux.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "plotmtv.h"
#include "interface.h"
#include "buffers.h"
#include "mp4_tf.h"
#include "tf_main.h"

#ifdef I2R_LOSSLESS
#include "int_mdct_defs.h"
#include "inv_int_mdct.h"
#include "lit_ll_common.h"
#include "int_tns.h"
#include "sls.h"
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b)) 
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* ***************************************************************************/
/* scalable decoding : bitstream has 3 subframes (called granules) per frame */


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

#ifdef PNS
/* compute scalar product of two vectors */
static double scalprod( const double src1[], const double src2[], 
                        int inc_src1, int inc_src2, int vlen)
{
  int i;
  double sum=0.0;

  for (i=0;i<vlen; i++) {
    sum += (*src1)*(*src2);
    src1 += inc_src1;
    src2 += inc_src2;
  }
  return (sum);
}
#endif /*PNS*/

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

static void vsub( const double src1[], const double src2[], double dest[], 
                  int inc_src1, int inc_src2, int inc_dest, int vlen )
{
  int i;

  for( i=0; i<vlen; i++ ) {
    *dest = *src1 - *src2;
    dest += inc_dest;
    src1 += inc_src1;
    src2 += inc_src2;
  }
}

/****************************************************************************************************************/

#define TEST_NEW_FUNCTIONS


static void DecodePns(int nFirstAacLay,
                      int nLastMonoLay,
                      int nFirstStLay, 
                      int nNrLay, 
                      double* pSpectrum[MAX_TF_LAYER][MAX_TIME_CHANNELS],
                      byte pCodeBooks[MAX_TF_LAYER][MAX_TIME_CHANNELS][MAXBANDS],
                      short* pFactors[MAX_TF_LAYER][MAX_TIME_CHANNELS],
                      Info* pInfo) {
  
  int   nSb, nWin;
  int   nSfbw; 
  int   nGroupCnt;
  int   nWndCnt;
  int   nCbOffset;
  int   nCb, nCbLeft, nCbRight;
  int   nLayer;
  int   i;

  static long nNoiseState;

  Float fScale;


  /* mono channels */
  for (nLayer=nFirstAacLay; nLayer <= nLastMonoLay; nLayer++) {
    int nSfOffset=0;
     
    nWin=0;
    nCbOffset=0;
  
    for(nGroupCnt=0;nGroupCnt<pInfo->num_groups;nGroupCnt++) {
      for(nWndCnt=0;nWndCnt<pInfo->group_len[nGroupCnt];nWndCnt++) {

        int nSboffs = nWin*pInfo->bins_per_sbk[nWin];
    
        for( nSb=0; nSb<pInfo->sfb_per_sbk[nWin]; nSb++ ) {
          if (pInfo->islong==0) {
            nSfbw = pInfo->sfb_width_short[nSb];
          }else {
            if (nSb==0)
              nSfbw = pInfo->bk_sfb_top[nSb];
            else 
              nSfbw = pInfo->bk_sfb_top[nSb]-pInfo->bk_sfb_top[nSb-1];        
          }
 
          nCb = (int) pCodeBooks[nLayer][MONO_CHAN][nSb+nCbOffset];
          if (nCb == NOISE_HCB) {
            fScale = pow( 2.0, 0.25*(pFactors[nLayer][MONO_CHAN][nSfOffset/*nSb*/]));
            gen_rand_vector(&pSpectrum[nLayer][MONO_CHAN][nSboffs],nSfbw, &nNoiseState);
            for (i=0; i<nSfbw; i++) {
              pSpectrum[nLayer][MONO_CHAN][nSboffs+i] *= fScale;
            }
          }
          nSfOffset++;
          nSboffs += nSfbw;
        }
        nWin++;
      }
      /* get new codebooks for next group*/
      nCbOffset+=pInfo->sfb_per_sbk[nGroupCnt];
    }
  }


  /* stereo channels */
  if (nFirstStLay!=-1 ) {
    int nSfOffset=0;

    for (nLayer=nFirstStLay; nLayer <=nNrLay; nLayer++) {
     
      nWin=0;
      nCbOffset=0;
  
      for(nGroupCnt=0;nGroupCnt<pInfo->num_groups;nGroupCnt++) {
        for(nWndCnt=0;nWndCnt<pInfo->group_len[nGroupCnt];nWndCnt++) {

          int nSboffs = nWin*pInfo->bins_per_sbk[nWin];
    
          for( nSb=0; nSb<pInfo->sfb_per_sbk[nWin]; nSb++ ) {
            if (pInfo->islong==0) {
              nSfbw = pInfo->sfb_width_short[nSb];
            }else {
              if (nSb==0)
                nSfbw = pInfo->bk_sfb_top[nSb];
              else 
                nSfbw = pInfo->bk_sfb_top[nSb]-pInfo->bk_sfb_top[nSb-1];        
            }
 
            nCbLeft = (int) pCodeBooks[nLayer][LEFT_CHAN][nSb+nCbOffset];
            nCbRight = (int) pCodeBooks[nLayer][RIGHT_CHAN][nSb+nCbOffset];

            /* correlated channels */
            if (nCbRight == NOISE_HCB + 100) {
              /*GenRandVectorDouble(&pSpectrum[nLayer][LEFT_CHAN][nSboffs],nSfbw, &nNoiseState);*/
              gen_rand_vector(&pSpectrum[nLayer][LEFT_CHAN][nSboffs],nSfbw, &nNoiseState);
              vcopy(&pSpectrum[nLayer][LEFT_CHAN][nSboffs], &pSpectrum[nLayer][RIGHT_CHAN][nSboffs], 1, 1, nSfbw);

              fScale = pow( 2.0, 0.25*(pFactors[nLayer][LEFT_CHAN][/*nSb*/nSfOffset]));
              for (i=0; i<nSfbw; i++) {
                pSpectrum[nLayer][LEFT_CHAN][nSboffs+i] *= fScale;
              }
              fScale = pow( 2.0, 0.25*(pFactors[nLayer][RIGHT_CHAN][/*nSb*/nSfOffset]));
              for (i=0; i<nSfbw; i++) {
                pSpectrum[nLayer][RIGHT_CHAN][nSboffs+i] *= fScale;
              }              
            }
            else {
              if (nCbLeft == NOISE_HCB) {
                fScale = pow( 2.0, 0.25*(pFactors[nLayer][LEFT_CHAN][/*nSb*/nSfOffset]));
                /*GenRandVectorDouble(&pSpectrum[nLayer][LEFT_CHAN][nSboffs],nSfbw, &nNoiseState);*/
                gen_rand_vector(&pSpectrum[nLayer][LEFT_CHAN][nSboffs],nSfbw, &nNoiseState);
                for (i=0; i<nSfbw; i++) {
                  pSpectrum[nLayer][LEFT_CHAN][nSboffs+i] *= fScale;
                }
              }
              if (nCbRight == NOISE_HCB) {
                fScale = pow( 2.0, 0.25*(pFactors[nLayer][RIGHT_CHAN][/*nSb*/nSfOffset]));
                /*GenRandVectorDouble(&pSpectrum[nLayer][RIGHT_CHAN][nSboffs],nSfbw, &nNoiseState);*/
                gen_rand_vector(&pSpectrum[nLayer][RIGHT_CHAN][nSboffs],nSfbw, &nNoiseState);
                for (i=0; i<nSfbw; i++) {
                  pSpectrum[nLayer][RIGHT_CHAN][nSboffs+i] *= fScale;
                }
              } 
            }

            nSfOffset++;
            nSboffs += nSfbw;
          }
          nWin++;
        }
        /* get new codebooks for next group*/
        nCbOffset+=pInfo->sfb_per_sbk[nGroupCnt];       
      }
    }
  }
}

static void DecodeIntensity(int nFirstStLay, 
                            int nNrLay, 
                            double* pSpectrum[MAX_TF_LAYER][MAX_TIME_CHANNELS],
                            byte pCodeBooks[MAX_TF_LAYER][MAX_TIME_CHANNELS][MAXBANDS],
                            short* pFactors[MAX_TF_LAYER][MAX_TIME_CHANNELS],
                            Info* pInfo,  
                            byte pMaxSfb[10],
                            NOK_LT_PRED_STATUS **nok_lt_status) {
  
  int nSb, nWin;
  int nSfbw; 
  int nGroupCnt;
  int nWndCnt;
  int nCbOffset;
  int nCb, nCbNextLay;
  int nSign;
  int nLayer;
  int i;

  Float fScale;

  for (nLayer=nFirstStLay; nLayer <=nNrLay; nLayer++) {
     
    nWin=0;
    nCbOffset=0;
  
    for(nGroupCnt=0;nGroupCnt<pInfo->num_groups;nGroupCnt++) {
      for(nWndCnt=0;nWndCnt<pInfo->group_len[nGroupCnt];nWndCnt++) {

        int nSboffs = nWin*pInfo->bins_per_sbk[nWin];
    
        /* for( nSb=0; nSb<pInfo->sfb_per_sbk[nWin]; nSb++ ) { */

        for( nSb=0; nSb<pMaxSfb[nLayer]; nSb++ ) {
          if (pInfo->islong==0) {
            nSfbw = pInfo->sfb_width_short[nSb];
          }else {
            if (nSb==0)
              nSfbw = pInfo->bk_sfb_top[nSb];
            else 
              nSfbw = pInfo->bk_sfb_top[nSb]-pInfo->bk_sfb_top[nSb-1];        
          }
 
          nCb = (int) pCodeBooks[nLayer][RIGHT_CHAN][nSb+nCbOffset];
          if (nCb == INTENSITY_HCB || nCb == INTENSITY_HCB2) {

            /* decode IS for this layer */
            
            nSign = (nCb == INTENSITY_HCB) ? 1 : -1;
            /* Bug or feature? pFactors are de-grouped, this seems not to be... */
            fScale = nSign * pow( 0.5, 0.25*(pFactors[nLayer][RIGHT_CHAN][nSb]));
            
            for (i=0; i<nSfbw; i++) {
              pSpectrum[nLayer][RIGHT_CHAN][nSboffs+i] = fScale * pSpectrum[nLayer][LEFT_CHAN][nSboffs+i];
            }

            /* switch off LTP */
            if (nLayer == nFirstStLay) {

              /* long windows */
              nok_lt_status[LEFT_CHAN] ->sfb_prediction_used[nSb+1] = 0;
              nok_lt_status[RIGHT_CHAN] ->sfb_prediction_used[nSb+1] = 0;
              /* short windows */
              /* switches off all 8 sfbs !!!! this is not yet implemented right !!!! */
              nok_lt_status[LEFT_CHAN]->sbk_prediction_used[nWin+1] = 0;          
              nok_lt_status[RIGHT_CHAN]->sbk_prediction_used[nWin+1] = 0;          
            }
        
            if (nLayer < nNrLay) {
              nCbNextLay = (int) pCodeBooks[nLayer+1][RIGHT_CHAN][nSb+nCbOffset];

              /* if there's IS in the next layer add left channels */
              if(nCbNextLay == INTENSITY_HCB || nCbNextLay == INTENSITY_HCB2) {
                for (i=0; i<nSfbw; i++) {
                  pSpectrum[nLayer+1][LEFT_CHAN][nSboffs+i]+=pSpectrum[nLayer][LEFT_CHAN][nSboffs+i];
                }
              }
            }                        
          }
          nSboffs += nSfbw;
        }
        nWin++;
      }
      /* get new codebooks for next group */
      nCbOffset+=pInfo->sfb_per_sbk[nGroupCnt];       
    }
  }
}

static void MapMask(int nFirstStLay, 
                    int nNrLay, 
                    byte pCodeBooks[MAX_TF_LAYER][MAX_TIME_CHANNELS][MAXBANDS],
                    int pMsMask[8][60],
                    int pMsMaskIs[8][60],
                    Info* pInfo) {


  int nSb, nWin;
  int nSfbw; 
  int nGroupCnt;
  int nWndCnt;
  int nCbOffset;
  int nLayer;

  byte* pCb;

  for (nLayer=nFirstStLay; nLayer <=nNrLay; nLayer++) {
     
    nWin=0;
    nCbOffset=0;
  
    for(nGroupCnt=0;nGroupCnt<pInfo->num_groups;nGroupCnt++) {
      for(nWndCnt=0;nWndCnt<pInfo->group_len[nGroupCnt];nWndCnt++) {

        int nSboffs = nWin*pInfo->bins_per_sbk[nWin];
    
        for( nSb=0; nSb<pInfo->sfb_per_sbk[nWin]; nSb++ ) {
          if (pInfo->islong==0) {
            nSfbw = pInfo->sfb_width_short[nSb];
          }else {
            if (nSb==0)
              nSfbw = pInfo->bk_sfb_top[nSb];
            else 
              nSfbw = pInfo->bk_sfb_top[nSb]-pInfo->bk_sfb_top[nSb-1];        
          }

          pCb =  &pCodeBooks[nLayer][RIGHT_CHAN][nSb+nCbOffset];

          if(pMsMask[nWin][nSb]) {
            if (*pCb == INTENSITY_HCB) {
              /**pCb = INTENSITY_HCB2;*/
              /* if this is the last layer switch off MS, otherwise this layer is not added anyway ! */
              if(nLayer==nNrLay) {
                pMsMask[nWin][nSb]=0;
              }
              pMsMaskIs[nWin][nSb]=0;
            }
            else if (*pCb == INTENSITY_HCB2) {
              /**pCb = INTENSITY_HCB;*/
              if(nLayer==nNrLay) {
                pMsMask[nWin][nSb]=0;
              }
              pMsMaskIs[nWin][nSb]=0;
            }
            if (*pCb == NOISE_HCB) {
              /* indicate correlated channels */
              *pCb = NOISE_HCB + 100;
              /* if this is the last layer switch off MS, otherwise this layer is not added anyway ! */
              if(nLayer==nNrLay) {
                pMsMask[nWin][nSb]=0;
              }
            }
          }
          else {
            if (*pCb == INTENSITY_HCB) {
              /*int dummy=0;*/
            }
            else if (*pCb == INTENSITY_HCB2) {
              /*int dummy=0;*/
            }
          }
          nSboffs += nSfbw;
        }
        nWin++;
      }
      /* get new codebooks for next group */
      nCbOffset+=pInfo->sfb_per_sbk[nGroupCnt];       
    }
  }
}

static void AddSpectralLines(int nFirstAacLay,
                             int nLastMonoLay,
                             int nFirstStLay, 
                             int nNrLay, 
                             double* pSpectrum[MAX_TF_LAYER][MAX_TIME_CHANNELS],
                             byte pCodeBooks[MAX_TF_LAYER][MAX_TIME_CHANNELS][MAXBANDS],
                             int pMsMask[8][60],
                             Info* pInfo,
                             byte pMaxSfb[10]) {
  
  int nSb, nWin;
  int nSfbw; 
  int nGroupCnt;
  int nWndCnt;
  int nCb, nPrevCb;  
  int nCbOffset;
  int nLayer;
  int nCh;  

  double fEnergy; 
  double fZero = 0.0;  

  int i,j,k;

  /* make life easier */
  for( k=0; k<MAX_TF_LAYER; k++ ) {
    for( i=0; i <MAX_TIME_CHANNELS; i++ ) {
      for( j=0; j<MAXBANDS; j++ ) {
        if(pCodeBooks[k][i][j] == NOISE_HCB + 100) pCodeBooks[k][i][j] = NOISE_HCB;
        if(pCodeBooks[k][i][j] == INTENSITY_HCB2) pCodeBooks[k][i][j] = INTENSITY_HCB;
      }
    }
  }

  /* mono channels */
  for (nLayer=nFirstAacLay+1; nLayer <= nLastMonoLay; nLayer++) {
     
    nWin=0;
    nCbOffset=0;
  
    for(nGroupCnt=0;nGroupCnt<pInfo->num_groups;nGroupCnt++) {
      for(nWndCnt=0;nWndCnt<pInfo->group_len[nGroupCnt];nWndCnt++) {

        int nSboffs = nWin*pInfo->bins_per_sbk[nWin];
    
        for( nSb=0; nSb<pMaxSfb[nLayer-1]; nSb++ ) {
          /*for( nSb=0; nSb<pInfo->sfb_per_sbk[nWin]; nSb++ ) {*/

          if (pInfo->islong==0) {
            nSfbw = pInfo->sfb_width_short[nSb];
          }else {
            if (nSb==0)
              nSfbw = pInfo->bk_sfb_top[nSb];
            else 
              nSfbw = pInfo->bk_sfb_top[nSb]-pInfo->bk_sfb_top[nSb-1];        
          }
 
          nPrevCb = (int) pCodeBooks[nLayer-1][MONO_CHAN][nSb+nCbOffset];
          nCb = (int) pCodeBooks[nLayer][MONO_CHAN][nSb+nCbOffset];

          if (nPrevCb == NOISE_HCB) {

            if (nCb == NOISE_HCB) {                
              vcopy(&fZero, &pSpectrum[nLayer-1][MONO_CHAN][nSboffs], 0 ,1 ,nSfbw);
            }
            else {
              fEnergy = scalprod(&pSpectrum[nLayer][MONO_CHAN][nSboffs], &pSpectrum[nLayer][MONO_CHAN][nSboffs], 1, 1, nSfbw);
              if (fEnergy > 0) {
                vcopy(&fZero, &pSpectrum[nLayer-1][MONO_CHAN][nSboffs], 0 ,1 ,nSfbw);         
              }
              else {
                pCodeBooks[nLayer][MONO_CHAN][nSb+nCbOffset] = pCodeBooks[nLayer-1][MONO_CHAN][nSb+nCbOffset];
                vcopy(&pSpectrum[nLayer-1][MONO_CHAN][nSboffs], &pSpectrum[nLayer][MONO_CHAN][nSboffs], 1, 1, nSfbw);
                vcopy(&fZero,&pSpectrum[nLayer-1][MONO_CHAN][nSboffs], 0, 1, nSfbw);  
              }
            }
          }
          else {

            if (nCb == NOISE_HCB) {
              pCodeBooks[nLayer][MONO_CHAN][nSb+nCbOffset] = pCodeBooks[nLayer-1][MONO_CHAN][nSb+nCbOffset];
              vcopy(&pSpectrum[nLayer-1][MONO_CHAN][nSboffs], &pSpectrum[nLayer][MONO_CHAN][nSboffs], 1, 1, nSfbw);
              vcopy(&fZero,&pSpectrum[nLayer-1][MONO_CHAN][nSboffs], 0, 1, nSfbw);                           
            }
          }
        
          nSboffs += nSfbw;
        }
        nWin++;
      }
      /* get new codebooks for next group*/
      nCbOffset+=pInfo->sfb_per_sbk[nGroupCnt];       
    }
  }

  /* combine all mono layers */
  for (nLayer=nFirstAacLay+1; nLayer <= nLastMonoLay; nLayer++) {

      vadd(pSpectrum[nFirstAacLay][MONO_CHAN],
           pSpectrum[nLayer][MONO_CHAN],pSpectrum[nFirstAacLay][MONO_CHAN],  
           1,1,1, pInfo->bins_per_bk);
  }  

  /* stereo channels */
  if (nFirstStLay!=-1 ) {

    for (nLayer=nFirstStLay+1; nLayer <=nNrLay; nLayer++) {
     
      nWin=0;
      nCbOffset=0;
  
      for(nGroupCnt=0;nGroupCnt<pInfo->num_groups;nGroupCnt++) {
        for(nWndCnt=0;nWndCnt<pInfo->group_len[nGroupCnt];nWndCnt++) {

          int nSboffs = nWin*pInfo->bins_per_sbk[nWin];
    
 
          for( nSb=0; nSb<pMaxSfb[nLayer-1]; nSb++ ) {
            /* for( nSb=0; nSb<pInfo->sfb_per_sbk[nWin]; nSb++ ) {*/
          
            if (pInfo->islong==0) {
              nSfbw = pInfo->sfb_width_short[nSb];
            }else {
              if (nSb==0)
                nSfbw = pInfo->bk_sfb_top[nSb];
              else 
                nSfbw = pInfo->bk_sfb_top[nSb]-pInfo->bk_sfb_top[nSb-1];        
            }
          
            for( nCh=0; nCh<2; nCh++) {
                        
              nPrevCb = (int) pCodeBooks[nLayer-1][nCh][nSb+nCbOffset];
              nCb = (int) pCodeBooks[nLayer][nCh][nSb+nCbOffset];

              if (pCodeBooks[nLayer][RIGHT_CHAN][nSb+nCbOffset] == INTENSITY_HCB) {
                if (pCodeBooks[nLayer][LEFT_CHAN][nSb+nCbOffset] == NOISE_HCB) {
                  nCb = NOISE_HCB;
                }
                else {
                  nCb = INTENSITY_HCB;
                }
              }
              if (pCodeBooks[nLayer-1][RIGHT_CHAN][nSb+nCbOffset] == INTENSITY_HCB) {
                if (pCodeBooks[nLayer-1][LEFT_CHAN][nSb+nCbOffset] == NOISE_HCB) {
                  nPrevCb = NOISE_HCB;
                }
                else {
                  nPrevCb = INTENSITY_HCB;
                }
              }

              if (nPrevCb == NOISE_HCB) {

                if (nCb == NOISE_HCB) {                
                  vcopy(&fZero, &pSpectrum[nLayer-1][nCh][nSboffs], 0 ,1 ,nSfbw);
                }
                else if (nCb == INTENSITY_HCB) {
                  vcopy(&fZero, &pSpectrum[nLayer-1][nCh][nSboffs], 0 ,1 ,nSfbw);         
                }
                else {
                  fEnergy = scalprod(&pSpectrum[nLayer][nCh][nSboffs], &pSpectrum[nLayer][nCh][nSboffs], 1, 1, nSfbw);
                  if (fEnergy > 0 || pMsMask[nWin][nSb] ) {
                    vcopy(&fZero, &pSpectrum[nLayer-1][nCh][nSboffs], 0 ,1 ,nSfbw);         
                  }
                  else {

                    pCodeBooks[nLayer][nCh][nSb+nCbOffset] = pCodeBooks[nLayer-1][nCh][nSb+nCbOffset];
                    vcopy(&pSpectrum[nLayer-1][nCh][nSboffs], &pSpectrum[nLayer][nCh][nSboffs], 1, 1, nSfbw);
                    vcopy(&fZero,&pSpectrum[nLayer-1][nCh][nSboffs], 0, 1, nSfbw);  
                  }

                }
              }
              else if (nPrevCb == INTENSITY_HCB) {

                if (nCb == NOISE_HCB) {
                  /*
                    pCodeBooks[nLayer][nCh][nSb+nCbOffset] = pCodeBooks[nLayer-1][nCh][nSb+nCbOffset];
                    vcopy(&pSpectrum[nLayer-1][nCh][nSboffs], &pSpectrum[nLayer][nCh][nSboffs], 1, 1, nSfbw);
                    vcopy(&fZero,&pSpectrum[nLayer-1][nCh][nSboffs], 0, 1, nSfbw);           
                  */
                  CommonWarning("invalid tools combination !");
                }
                else if (nCb == INTENSITY_HCB) {
                  vcopy(&fZero, &pSpectrum[nLayer-1][nCh][nSboffs], 0 ,1 ,nSfbw);
                }
                else {
                  vcopy(&fZero, &pSpectrum[nLayer-1][nCh][nSboffs], 0 ,1 ,nSfbw);
                }
              }
              else {

                if (nCb == NOISE_HCB) {
                  /*
                    pCodeBooks[nLayer][nCh][nSb+nCbOffset] = pCodeBooks[nLayer-1][nCh][nSb+nCbOffset];
                    vcopy(&pSpectrum[nLayer-1][nCh][nSboffs], &pSpectrum[nLayer][nCh][nSboffs], 1, 1, nSfbw);
                    vcopy(&fZero,&pSpectrum[nLayer-1][nCh][nSboffs], 0, 1, nSfbw);                           
                  */
                  CommonWarning("invalid tools combination !");
                }
                else if (nCb == INTENSITY_HCB) {
                  /*
                    pCodeBooks[nLayer][nCh][nSb+nCbOffset] = pCodeBooks[nLayer-1][nCh][nSb+nCbOffset];
                    vcopy(&pSpectrum[nLayer-1][nCh][nSboffs], &pSpectrum[nLayer][nCh][nSboffs], 1, 1, nSfbw);
                    vcopy(&fZero,&pSpectrum[nLayer-1][nCh][nSboffs], 0, 1, nSfbw);           
                  */
                  CommonWarning("invalid tools combination !");
                }
                else {

                }
              }
            }
            nSboffs += nSfbw;
          }
          nWin++;
        }
        /* get new codebooks for next group*/
        nCbOffset+=pInfo->sfb_per_sbk[nGroupCnt];       
      }
    }
  }

  /* combine all stereo layers */
  if (nFirstStLay!=-1 ) {
    for (nLayer=nFirstStLay+1; nLayer <= nNrLay; nLayer++) {
      for (nCh=0; nCh<2; nCh++){

        vadd(pSpectrum[nFirstStLay][nCh], pSpectrum[nLayer][nCh],
             pSpectrum[nFirstStLay][nCh],  
             1,1,1, pInfo->bins_per_bk );
      }
    }
  }

  /* Mono - Stereo Combination  */
  if (nFirstStLay!=-1 ) {

    if ( nFirstStLay != nFirstAacLay ) {

      /* layers have been added to the first one -> consider the correct codebooks */
      for( i =0; i <MAX_TIME_CHANNELS; i++ ) {
        for( j =0; j<MAXBANDS; j++ ) {
          pCodeBooks[nFirstAacLay][i][j] = pCodeBooks[nLastMonoLay][i][j];
          pCodeBooks[nFirstStLay][i][j] = pCodeBooks[nNrLay][i][j];
        }
      }

      nLayer = nFirstStLay;
     
      nWin=0;
      nCbOffset=0;
  
      for(nGroupCnt=0;nGroupCnt<pInfo->num_groups;nGroupCnt++) {
        for(nWndCnt=0;nWndCnt<pInfo->group_len[nGroupCnt];nWndCnt++) {

          int nSboffs = nWin*pInfo->bins_per_sbk[nWin];
    
          for( nSb=0; nSb<pMaxSfb[nLayer]; nSb++ ) {
            /* for( nSb=0; nSb<pInfo->sfb_per_sbk[nWin]; nSb++ ) {*/

            if (pInfo->islong==0) {
              nSfbw = pInfo->sfb_width_short[nSb];
            }else {
              if (nSb==0)
                nSfbw = pInfo->bk_sfb_top[nSb];
              else 
                nSfbw = pInfo->bk_sfb_top[nSb]-pInfo->bk_sfb_top[nSb-1];        
            }
            for( nCh=0; nCh<2; nCh++) {
 
              nPrevCb = (int) pCodeBooks[nFirstAacLay][MONO_CHAN][nSb+nCbOffset];
              nCb = (int) pCodeBooks[nFirstStLay][nCh][nSb+nCbOffset];

              if (nPrevCb == NOISE_HCB) {

                fEnergy = scalprod(&pSpectrum[nLayer][nCh][nSboffs], &pSpectrum[nLayer][nCh][nSboffs], 1, 1, nSfbw);
                if (fEnergy > 0 || pMsMask[nWin][nSb]) {
                  vcopy(&fZero, &pSpectrum[nFirstAacLay][MONO_CHAN][nSboffs], 0 ,1 ,nSfbw);         
                }
                else {
                  /*
                    pCodeBooks[nLayer][nCh][nSb+nCbOffset] = pCodeBooks[nFirstAacLay][MONO_CHAN][nSb+nCbOffset];
                    vcopy(&pSpectrum[nFirstAacLay][MONO_CHAN][nSboffs], &pSpectrum[nLayer][nCh][nSboffs], 1, 1, nSfbw);
                    if (nCh>0) {
                    vcopy(&fZero,&pSpectrum[nFirstAacLay][MONO_CHAN][nSboffs], 0, 1, nSfbw);                           
                    }
                  */
                  /* FSS-control ! */
                }
              }
              if (nCb == NOISE_HCB) {
                /*if (nCh>0) {*/
                vcopy(&fZero,&pSpectrum[nFirstAacLay][MONO_CHAN][nSboffs], 0, 1, nSfbw);                           
                /*}*/
              }

              /* don't add mono and intensity layers */
              if (nCh == RIGHT_CHAN && nCb == INTENSITY_HCB) {
                vcopy(&fZero,&pSpectrum[nFirstAacLay][MONO_CHAN][nSboffs], 0, 1, nSfbw);                           
              }
            }        
          
            nSboffs += nSfbw;
          }
          nWin++;
        }
        /* get new codebooks for next group*/
        nCbOffset+=pInfo->sfb_per_sbk[nGroupCnt];       
      }
    }
  }
}


/****************************************************************************************************************/

static void dec_lowpass(double spectrum[], int lopLong,
                        int lopShort, int maxLine, int iblen, 
                        int windowSequence, int nr_of_sbk)
{
  int bl, block_len, lop, i;
  float slope;
  double zero = 0.0;

  if (windowSequence != EIGHT_SHORT_SEQUENCE) {
    lop = lopLong;
  }
  else {
    lop = lopShort;
  }
    
  block_len = iblen /nr_of_sbk;
  if (lop > maxLine) {
    lop = maxLine;
  }
  slope = (1.0F /(float)lop);
  for (bl=0;bl<nr_of_sbk;bl++){
    vcopy( &zero, &spectrum[bl*block_len+maxLine], 0, 1, block_len - maxLine);
    for (i=1; i<lop; i++) {
      spectrum[bl*block_len+maxLine-i] *= (i*slope);
    }
  }
}

/* get_tns_vm does exactly the same as get_tns in huffdec2.c. Just the
   VM bitstream handling functions replace the AAC-type functions */

static void doInverseMsMatrix(
                              Info       *p_sb_info,
                              double *p_l_spec,
                              double *p_r_spec,
                              int msMask[8][60]
                              )
{
  int sb, win;
  int sfbw; 
  double  tmp_buffer[1024];
  for( win=0; win<p_sb_info->nsbk; win++ ) {
    int sboffs   = win*p_sb_info->bins_per_sbk[win];

    for( sb=0; sb<p_sb_info->sfb_per_sbk[win]; sb++ ) {
      if (p_sb_info->islong==0){
        sfbw = p_sb_info->sfb_width_short[sb];
      }else {
        if (sb==0)
          sfbw = p_sb_info->bk_sfb_top[sb];
        else 
          sfbw = p_sb_info->bk_sfb_top[sb]-p_sb_info->bk_sfb_top[sb-1];        
      }

      if(msMask[win][sb] != 0  ) {
        vadd( &p_l_spec[sboffs], &p_r_spec[sboffs], &tmp_buffer[sboffs],        1, 1, 1, sfbw );  /* -> L */
        vsub( &p_l_spec[sboffs], &p_r_spec[sboffs], &p_r_spec[sboffs], 1, 1, 1, sfbw );  /* -> R */
        vcopy(&tmp_buffer[sboffs], &p_l_spec[sboffs], 1, 1, sfbw );
      }

      sboffs += sfbw;
    }
  }

}

static void doMsMatrix(Info *p_sb_info, double *p_l_spec, double *p_r_spec, int msMask[8][60])
{
  int sb, win, i;
  int sfbw;
  double sum[1024];
  double diff[1024];
  for( win=0; win<p_sb_info->nsbk; win++ ) {
    int sboffs   = win*p_sb_info->bins_per_sbk[win];
    
    for( sb=0; sb<p_sb_info->sfb_per_sbk[win]; sb++ ) {
      if (p_sb_info->islong==0){
        sfbw = p_sb_info->sfb_width_short[sb];
      }else {
        if (sb==0)
          sfbw = p_sb_info->bk_sfb_top[sb];
        else 
          sfbw = p_sb_info->bk_sfb_top[sb]-p_sb_info->bk_sfb_top[sb-1];        
      }

      if(msMask[win][sb] != 0  ) {
        vadd(&p_l_spec[sboffs], &p_r_spec[sboffs], &sum[sboffs], 1, 1, 1, sfbw);
        vsub(&p_l_spec[sboffs], &p_r_spec[sboffs], &diff[sboffs], 1, 1, 1, sfbw);
	
        for(i = 0; i < sfbw; i++) {
          p_l_spec[sboffs + i] = 0.5 * sum[sboffs + i];
          p_r_spec[sboffs + i] = 0.5 * diff[sboffs + i];
        }
      }
      
      sboffs += sfbw;
    }
  }
  
}

static void  multCoreSpecMsFac(
                               Info       *p_sb_info,
                               int msMask[8][60],
                               double *core_spec,
                               double *out_spec
                               )
{
  int sb, win;
  int sfbw;
  double ms_fac=2;
  for( win=0; win<p_sb_info->nsbk; win++ ) {
    int sboffs   = win*p_sb_info->bins_per_sbk[win];

    for( sb=0; sb<p_sb_info->sfb_per_sbk[win]; sb++ ) {
      if (p_sb_info->islong==0){
        sfbw = p_sb_info->sfb_width_short[sb];
      }else {
        if (sb==0)
          sfbw = p_sb_info->bk_sfb_top[sb];
        else 
          sfbw = p_sb_info->bk_sfb_top[sb]-p_sb_info->bk_sfb_top[sb-1];        
      }
      if( msMask[win][sb] == 0  ) {
        vmult( &core_spec[sboffs], &ms_fac,&out_spec[sboffs] ,1, 0, 1, sfbw );  /* if no ms core = core * fac */
      }else{
        vcopy( &core_spec[sboffs],&out_spec[sboffs] ,1, 1, sfbw );  /* if ms core = core */
      }
      sboffs += sfbw;
    }
  }

}

static int get_tns_vm( Info *info,
                       TNS_frame_info *tns_frame_info,
                       HANDLE_RESILIENCE hResilience,
                       HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                       HANDLE_BUFFER hVm )
{
  int                       f, t, top, res, res2, compress;
  int                       short_flag, s;
  short                     *sp, tmp, s_mask, n_mask;
  TNSfilt                   *tns_filt;
  TNSinfo                   *tns_info;
  static const short        sgn_mask[] = { 0x2, 0x4, 0x8 };
  static const short        neg_mask[] = { (short) 0xfffc, (short)0xfff8, (short)0xfff0 };
  unsigned long             numBits;

  short_flag = (!info->islong);
  tns_frame_info->n_subblocks = info->nsbk;

  for (s=0; s<tns_frame_info->n_subblocks; s++) {
    tns_info = &tns_frame_info->info[s];
    numBits = (short_flag ? 1 : 2);

    if (!(tns_info->n_filt = GetBits( numBits, N_FILT, hResilience, hEscInstanceData, hVm )) )
      continue;     

    tns_info -> coef_res = res = GetBits( 1, COEF_RES, hResilience, hEscInstanceData, hVm ) + 3;

    top = info->sfb_per_sbk[s];
    tns_filt = &tns_info->filt[ 0 ];
    for (f=tns_info->n_filt; f>0; f--)  {
      tns_filt->stop_band = top;
      numBits = ( short_flag ? 4 : 6);

      top = tns_filt->start_band = top - GetBits( numBits, LENGTH, hResilience, hEscInstanceData, hVm );

      numBits = ( short_flag ? 3 : 5);
      tns_filt->order = GetBits( numBits, ORDER, hResilience, hEscInstanceData, hVm );

      if (tns_filt->order)  {
        tns_filt->direction = GetBits( 1, DIRECTION, hResilience, hEscInstanceData, hVm );

        compress = GetBits( 1, COEF_COMPRESS, hResilience, hEscInstanceData, hVm );

        res2 = res - compress;
        s_mask = sgn_mask[ res2 - 2 ];
        n_mask = neg_mask[ res2 - 2 ];

        sp = tns_filt -> coef;
        for (t=tns_filt->order; t>0; t--)  {
          tmp = (short) GetBits( res2, COEF, hResilience, hEscInstanceData, hVm );
          *sp++ = (tmp & s_mask) ? (tmp | n_mask) : tmp;
        }
      }
      tns_filt++;
    }
  }   /* subblock loop */
  return 1;
}

 
static void FSSwitch(
              double  p_core[],
              double  p_rest[], 
              double  p_out[], 
              int          *diff_control, 
              Info         *p_sbinfo,
              int           numDiffBands,
              int        shortDiffWidth,
              int           is_decoder
              )
{
  int sbk, diffBand,x;
  int dc_decoded[8][128];
  short diffBandWidth[200];
  double doubleZero=0.0;
  int numDiffBandsPerSbk;
  int g;

  if (p_sbinfo->islong){
    numDiffBandsPerSbk=numDiffBands;
    diffBandWidth[0]= p_sbinfo->bk_sfb_top[0];  
    for (x=1;x< p_sbinfo->sfb_per_bk;x++){
      if  (p_sbinfo->bk_sfb_top[x]==0)
        CommonExit(-1,"\ninternal FSS error");
      diffBandWidth[x]= p_sbinfo->bk_sfb_top[x] - p_sbinfo->bk_sfb_top[x-1];
    }
  } else {
    numDiffBandsPerSbk=1;
    diffBandWidth[0]= shortDiffWidth;
  }

  g=0;
  for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
    for( diffBand=0; diffBand<numDiffBandsPerSbk; diffBand++ ) {
      dc_decoded[sbk][diffBand] = diff_control[g];
      g++;
    }
  }

  for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
    int offset = sbk*p_sbinfo->bins_per_sbk[sbk];
    int sum    = 0;
    for( diffBand=0; diffBand<numDiffBandsPerSbk; diffBand++ ) {
      if( dc_decoded[sbk][diffBand] == 0 ) {
        if( is_decoder ) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, diffBandWidth[diffBand] ); 
        }
      } else {
        vcopy( &p_rest[offset], &p_out[offset], 1, 1, diffBandWidth[diffBand] ); 
        if( !is_decoder ) {
          vcopy( &doubleZero , &p_core[offset], 0, 1, diffBandWidth[diffBand] ); 
        }
      }
      offset += diffBandWidth[diffBand];
      sum    += diffBandWidth[diffBand];
    }
    vcopy( &p_rest[offset], &p_out[offset], 1, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
    vcopy( &doubleZero ,   &p_core[offset], 0, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
  }
}

static void FSSwitchSt( double  p_core[],/* bands of this spec is mult by 2 if ms is off */ /* firstAacLay */
                        double  p_rest[],  /* firstStLay */
                        double  p_out[], 
                        int*    diff_control, 
                        int     diffContrBands,
                        Info*   p_sbinfo,
                        int     msMask[8][60],
                        int     channel)
{
  int sbk, sfb;
  int dc_decoded[8][128];
  short sfbWidth[200];
  double doubleZero=0.0;

  sfbWidth[0]= p_sbinfo->sbk_sfb_top[0][0];  
  for (sfb=1;sfb< p_sbinfo->sfb_per_sbk[0];sfb++){
    if  (p_sbinfo->sbk_sfb_top[0][sfb]==0)
      CommonExit(-1,"\ninternal FSS error");
    sfbWidth[sfb]= p_sbinfo->sbk_sfb_top[0][sfb] - p_sbinfo->sbk_sfb_top[0][sfb-1];
  }
  
  if (p_sbinfo->islong){
    for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[0]; sfb++ ) {
      if (sfb<diffContrBands) {
        dc_decoded[0][sfb] = diff_control[sfb];
      }else {
        dc_decoded[0][sfb] =1;
      }
    }
  } else {
    /* see "correction  for short" */
    for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
      for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[0]; sfb++ ) {
        if (sfb<diffContrBands) {
          if ((msMask[sbk][sfb]==1)&&(channel==1)){
            /* side channel never uses diffencoding */
            dc_decoded[sbk][sfb] = 1;
          } else {                          
            if ((msMask[sbk][sfb]==1)&&(channel==0)) {
              /* M channel : always diff encoding if there is no celp or tvq core */
              dc_decoded[sbk][sfb] = 0;
            } else {
              /* L,R channel : use diff flag from bitstream */
              dc_decoded[sbk][sfb] = diff_control[sbk];
            }
          }
        }else {
          dc_decoded[sbk][sfb] =1;
        }
      }
    }
  }

  for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
    int offset = sbk*p_sbinfo->bins_per_sbk[sbk];
    int sum    = 0;  
   for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[sbk]; sfb++  ) {
      /* build combined L/R spectrum */
      if( (msMask[sbk][sfb] == 0 ) ) {
        if(  dc_decoded[sbk][sfb] == 0 ) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, sfbWidth[sfb] ); 
        } else {
          vcopy( &p_rest[offset], &p_out[offset], 1, 1, sfbWidth[sfb] ); 
        }
      } else {
      /* build combined M spectrum */
        if( channel == 0 ) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, sfbWidth[sfb] ); 
        } else {
          vcopy( &p_rest[offset], &p_out[offset], 1, 1, sfbWidth[sfb] ); 
        }
      }
      offset += sfbWidth[sfb];
      sum    += sfbWidth[sfb];
    }
    vcopy( &p_rest[offset], &p_out[offset], 1, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
    vcopy( &doubleZero ,   &p_core[offset], 0, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
  }
}

static void FSSwitchStwCore( double  p_core[],/* bands of this spec is mult by 2 if ms is off */ 
                             double  p_rest[], 
                             double  p_out[], 
                             int*    diff_control, 
                             int*    diff_controlLR, 
                             int     diffContrBands,
                             Info*   p_sbinfo,
                             int     msMask[8][60],
                             int     channel )
{
  int sbk, sfb;
  int dc_decoded[8][128];
  short sfbWidth[200];
  double doubleZero=0.0;

  sfbWidth[0]= p_sbinfo->sbk_sfb_top[0][0];  
  for (sfb=1;sfb< p_sbinfo->sfb_per_sbk[0];sfb++){
    if  (p_sbinfo->sbk_sfb_top[0][sfb]==0)
      CommonExit(-1,"\ninternal FSS error");
    sfbWidth[sfb]= p_sbinfo->sbk_sfb_top[0][sfb] - p_sbinfo->sbk_sfb_top[0][sfb-1];
  }
  
  if (p_sbinfo->islong){
    for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[0]; sfb++ ) {
      if (sfb<diffContrBands) {
        dc_decoded[0][sfb] = diff_controlLR[sfb]; 
      }else {
        dc_decoded[0][sfb] =1;
      }
    }
  } else {
    diffContrBands=120; /* some huge number , actually wrong, but corrected below: */
    /* see "correction  for short" */
    for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
      for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[0]; sfb++ ) {
        if (sfb<diffContrBands) {
          if ((msMask[sbk][sfb]==1)&&(channel==1)){
            /* side channel never uses diffencoding */
            dc_decoded[sbk][sfb] = 1;
          } else {                          
            /* L,R or M channel : use diff flag from bitstream */
            if ((msMask[sbk][sfb]==1)&&(channel==0)) {
              /* M channel: */ 
              dc_decoded[sbk][sfb] = diff_control[sbk];
            } else {
              /* L and R channel */
              dc_decoded[sbk][sfb] = diff_controlLR[sbk];
            }
          }
        }else {
          /* all above lower layer bandwith does not use diff spec */
          dc_decoded[sbk][sfb] =1;
        }
      }
    }
  }
  for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
    int offset = sbk*p_sbinfo->bins_per_sbk[sbk];
    int sum    = 0;  
    
    for( sfb=0; sfb<p_sbinfo->sfb_per_sbk[sbk]; sfb++  ) {
      /* build combined L/R spectrum */
      if( (msMask[sbk][sfb] == 0 ) ) {
        if(  dc_decoded[sbk][sfb] == 0 ) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, sfbWidth[sfb] ); 
        } else {
          vcopy( &p_rest[offset], &p_out[offset], 1, 1, sfbWidth[sfb] ); 
        }
      }      else  {
        /* build combined M spectrum */
        if( (channel == 0) && (dc_decoded[sbk][sfb] == 0)) {
          vadd ( &p_core[offset], &p_rest[offset], &p_out[offset], 1, 1, 1, sfbWidth[sfb] ); 
        } else {
          vcopy( &p_rest[offset], &p_out[offset], 1, 1, sfbWidth[sfb] ); 
        }
      }
      offset += sfbWidth[sfb];
      sum    += sfbWidth[sfb];
    }
    vcopy( &p_rest[offset], &p_out[offset], 1, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
    vcopy( &doubleZero ,   &p_core[offset], 0, 1, p_sbinfo->bins_per_sbk[sbk]-sum ); 
  }
  /*correction  for short: diff encoding only up top diff_Short_lines see Table 6.12 */
  if (p_sbinfo->islong==0){
    for( sbk=0; sbk<p_sbinfo->nsbk; sbk++ ) {
      int offset = sbk*p_sbinfo->bins_per_sbk[sbk]+p_sbinfo->shortFssWidth;
      int count = p_sbinfo->bins_per_sbk[sbk]-p_sbinfo->shortFssWidth;
      vcopy( &p_rest[offset], &p_out[offset], 1, 1, count );      
    }
  }
}

static void  decodeDiffCtrl( int diffControl[ ],
                             int fssGroups,
                             HANDLE_RESILIENCE hResilience,
                             HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                             HANDLE_BUFFER hVm ) {

  unsigned long int ultmp, fssIdx, diffControlTmp;
  int fssG,sfb;
  for (sfb=0;sfb<60;sfb++){
    diffControl[sfb]=0;
  }
  sfb=0;
  for (fssG=0;fssG<fssGroups;fssG++){
    diffControlTmp=0;
    fssIdx = GetBits( 2, DIFF_CONTROL, hResilience, hEscInstanceData, hVm );
    switch (fssIdx){
    case 0 : diffControlTmp |=0;
      break;
    case 1 : diffControlTmp |=15;
      break;
    default :
      ultmp = GetBits( 2, DIFF_CONTROL, hResilience, hEscInstanceData, hVm );
      fssIdx= (fssIdx<<2)| ultmp;
      switch (fssIdx){
      case  8:  diffControlTmp |=7;
        break;
      case  9:  diffControlTmp |=8;
        break;
      default:
        ultmp = GetBits( 1, DIFF_CONTROL, hResilience, hEscInstanceData, hVm );
        fssIdx= (fssIdx<<1)| ultmp;
        switch (fssIdx) {
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
          diffControlTmp |= (fssIdx-19);
          break;
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
          diffControlTmp |= (fssIdx-17);
          break;
        default:
          CommonExit(-1,"\nfssdecode error");
        }
        break;
      }
    }
    diffControl[sfb+3]= ((diffControlTmp & 0x1)!=0);
    diffControl[sfb+2]= ((diffControlTmp & 0x2)!=0);
    diffControl[sfb+1]= ((diffControlTmp & 0x4)!=0);
    diffControl[sfb+0]= ((diffControlTmp & 0x8)!=0);
    sfb +=4;      
  }
}

static int decode_grouping( int grouping, short region_len[] )
{
  int i, rlen, mask;

  int no_short_reg = 0;

  mask = 1 << ( 8-2 );
  rlen = 1;  /* min. length of first group is '1' */
  for( i = 1; i<8; i++ ) {
    if( (grouping & mask) == 0) {
      *region_len++ = rlen;
      no_short_reg++;
      rlen = 0;
    }
    rlen++;
    mask >>= 1;
  }
  *region_len = rlen;
  no_short_reg++;

  return( no_short_reg );
}


static void getModeFromOD(OBJECT_DESCRIPTOR *od,
                          int *lowRateChannelPresent,
                          int *layNumChan,
#ifdef I2R_LOSSLESS
                          int *lastSlsLay,
#endif
                          int *lastAacLay,
                          int *firstStLay, 
                          enum CORE_CODEC* coreCoderIndex,
                          int *commonWindow)
{
  
  int nEsNumber, i, aac=0, nAacLay = 0;
  int nLowRateChan = 0;
  int audioDecType;
  int nFirstStereo = -1;
  /* default: no celp enhancement layer */
  int nCelpEnhLay = 0;
 
  nEsNumber   = od->streamCount.value;
  *coreCoderIndex=NO_CORE;
  for (i= 0; i< nEsNumber; i++) {
    audioDecType = od->ESDescriptor[i]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;
    layNumChan[i] = od->ESDescriptor[i]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;
    switch (audioDecType)
      {
      case AAC_SCAL:
      case ER_AAC_SCAL:
        if (( layNumChan[i] ) == 2 && (nFirstStereo < 0))
          nFirstStereo = i;
        nAacLay++;
        aac=1;
        break;
      case TWIN_VQ:
        if (( layNumChan[i] ) == 2 && (nFirstStereo < 0))
          nFirstStereo = i;
        nLowRateChan++;
        *coreCoderIndex=NTT_TVQ;                  
        break;
      case CELP: /* CELP core */
        nLowRateChan++;
        *coreCoderIndex=CC_CELP_MPEG4;

        /* get correct number of celp enhancement layers */
        nCelpEnhLay = od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.numEnhLayers.value;
        break;
      case ER_TWIN_VQ:
        if (( layNumChan[i] ) == 2 && (nFirstStereo < 0))
          nFirstStereo = i;
        nLowRateChan++;
        *coreCoderIndex=NTT_TVQ;                  
        break;
      case ER_CELP:
        nLowRateChan++;
        *coreCoderIndex=CC_CELP_MPEG4;
        /* get correct number of celp enhancement layers */
        nCelpEnhLay = od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.numEnhLayers.value;
        break;
#ifdef I2R_LOSSLESS
      case SLS:
      case SLS_NCORE:
        (*lastSlsLay)++;
        break;
      case AAC_LC:
        if (( layNumChan[i] ) == 2 && (nFirstStereo < 0))
          nFirstStereo = i;
        nAacLay = 1;
        aac=1;
        break;
#endif

      default:
        CommonExit(-1,"audioDecoderType not implemented");
        break;
      }
    switch (od->ESDescriptor[i]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value ) 
      {
      case 1 :
        commonWindow[i]=0;
        break;
      case 2 :
        commonWindow[i]=1;
        break;
      default :
        CommonExit(-1,"wrong channel config ");
        break;
      }
  }

  *lastAacLay = nLowRateChan ? nAacLay+nCelpEnhLay : nAacLay+nCelpEnhLay-1;
  
  if (aac==0) *lastAacLay=-1;
  *lowRateChannelPresent = nLowRateChan;
  *firstStLay = nFirstStereo  ;

}

static void ltp_scal_reconstruct(WINDOW_SEQUENCE blockType,
                                 WINDOW_SHAPE windowShape,
                                 WINDOW_SHAPE windowShapePrev,
                                 int num_channels,
                                 double **p_reconstructed_spectrum,
                                 int blockSizeSamples,
                                 int short_win_in_long,
                                 int nr_of_sfb,
                                 int msMask[8][60],
                                 NOK_LT_PRED_STATUS **nok_lt_status,
                                 Info *info, TNS_frame_info **tns_info,
                                 QC_MOD_SELECT qc_select,int samplFreqIdx)
{
  int i, j,ch;
  double tmpBuf[BLOCK_LEN_LONG], time_signal[BLOCK_LEN_LONG];
  Float tmpbuffer[BLOCK_LEN_LONG];
  NOK_LT_PRED_STATUS *nok_ltp;

  /*
   * Following piece of code reconstructs the spectrum of the 
   * first mono (or stereo, if no mono present) layer for LTP.
   */
  
  if(num_channels == 2)
    doInverseMsMatrix(info, p_reconstructed_spectrum[0],
		      p_reconstructed_spectrum[1], msMask);

  for(ch = 0; ch < num_channels; ch++)
  {
    nok_ltp = nok_lt_status[ch];

    for(i = 0; i < blockSizeSamples; i++)
      tmpbuffer[i] = p_reconstructed_spectrum[ch][i];

    /* Add the LTP contribution to the reconstructed spectrum. */
    nok_lt_predict (info, blockType, windowShape, windowShapePrev,
		    nok_ltp->sbk_prediction_used, nok_ltp->sfb_prediction_used, 
                    nok_ltp, nok_ltp->weight, nok_ltp->delay, tmpbuffer, 
                    blockSizeSamples, blockSizeSamples / short_win_in_long,
                    samplFreqIdx, tns_info[ch], qc_select);

    for(i = 0; i < blockSizeSamples; i++)
      p_reconstructed_spectrum[ch][i] = tmpbuffer[i];

    /* TNS synthesis filtering. */
    info->islong = (blockType != EIGHT_SHORT_SEQUENCE);
    for(i = j = 0; i < tns_info[ch]->n_subblocks; i++)
    {
      tns_decode_subblock(tmpbuffer + j, nr_of_sfb, 
			  info->sbk_sfb_top[i], info->islong,
			  &tns_info[ch]->info[i], qc_select,samplFreqIdx);
      
      j += info->bins_per_sbk[i];
    }
    
    /* Update the time domain buffer of LTP. */
    for(i = 0; i < blockSizeSamples; i++)
      tmpBuf[i] = tmpbuffer[i];
    
    freq2buffer(tmpBuf, time_signal, nok_ltp->overlap_buffer,
		blockType, blockSizeSamples,
                blockSizeSamples/short_win_in_long, windowShape, 
		windowShapePrev, OVERLAPPED_MODE, short_win_in_long);

    nok_lt_update(nok_ltp, time_signal, nok_ltp->overlap_buffer, blockSizeSamples);
  }

  /*
   * The reconstructed spectrum of the lowest layer is processed as 
   * Mid/Side in the upper layers.
   */
  if(num_channels == 2)
    doMsMatrix(info, p_reconstructed_spectrum[0], 
	       p_reconstructed_spectrum[1], msMask);
}

static void DecodeMS( TF_DATA* tfData,
                      int startSfb,
                      int max_sfb,
                      int msMask[8][60],
                      HANDLE_RESILIENCE hResilience,
                      HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                      HANDLE_BUFFER hVm ) {

  int g,sfb;
  int win, gwin;
  int msMaskPres = 0;

  msMaskPres = GetBits( 2, MS_MASK_PRESENT, hResilience, hEscInstanceData, hVm );

  if (msMaskPres==0) {
    win=0;
    for (g=0;g<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups;g++){
      for (gwin=0;gwin<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
        for (sfb=startSfb;sfb<max_sfb;sfb++){
          msMask[win][sfb]=0;
        }
        win++;
      }
    }  
  }
  else if (msMaskPres==1){
    win=0;
    for (g=0;g<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups;g++){
      for (sfb=startSfb;sfb<max_sfb;sfb++){
        msMask[win][sfb] = GetBits( 1, MS_USED, hResilience, hEscInstanceData, hVm );
      }
      for (gwin=1;gwin<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
        for (sfb=startSfb;sfb<max_sfb;sfb++){                
          msMask[win+gwin][sfb]=msMask[win][sfb];/* apply same ms mask to all 
                                                    subwindows of same group */
        }
      }
      win += tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]-> group_len[g] ;
    }          
  }else if (msMaskPres==2){
    win=0;
    for (g=0;g<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups;g++){
      for (gwin=0;gwin<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]-> group_len[g];gwin++){
        for (sfb=startSfb;sfb<max_sfb;sfb++){
          msMask[win][sfb]=1;
        }
        win++;
      }
    }  
  }
}

/***************************************************************************************
   Function     : getFirstAacStream()
   Purpose      : get the first aac stream number
   Arguments    : fd               = FRAME_DATA pointer
   Returns      : number of firstAacStream
   Author       : Thomas Breitung, FhG IIS-A
   last changed : 2003-11-24
****************************************************************************************/

static int getFirstAacStream( FRAME_DATA* fd ) {

  unsigned int stream = 0;
  int firstAacStream = -1;

  for ( stream=0; stream < fd->od->streamCount.value; stream++ ) {
    if ((firstAacStream == -1) && (fd->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_SCAL)) {
      firstAacStream = stream;
      break;
    }
  }
  return firstAacStream;
}

/***************************************************************************************
   Function     : getStreamsPerLayer()
   Purpose      : get the number of streams per layer
   Arguments    : fd                 = FRAME_DATA pointer
                  firstAacStream     = set to first aac layer
                                    
   Returns      : streamsPerAacLayer = field with streams per layer
                  aacLayerNr         = number of aac layer
   Author       : Thomas Breitung, FhG IIS-A
   last changed : 2003-11-24
****************************************************************************************/

static void getStreamsPerLayer( FRAME_DATA* fd, int *firstAacStream, int* numberOfAacLayer, int *streamsPerLayer ) {
  
  unsigned int stream   =  0;
  int coreLayer         =  0; /* core layer present */
  int tmpFirstAacStream = -1;

  for ( stream = 0; stream < fd->od->streamCount.value; stream++ ) {
    /* count number of core streams */
    if ( fd->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value != ER_AAC_SCAL ) {
      streamsPerLayer[0]++;
      coreLayer = 1;
    }  
    /* get number of er aac scal streams per layer */   
    else {
      /* save number of first aac stream */
      if ( tmpFirstAacStream == -1 ) 
        tmpFirstAacStream = stream;
 
#ifdef AAC_ELD
      if (fd->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_ELD)
        streamsPerLayer[coreLayer]++;
      else
#endif
      streamsPerLayer[fd->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.layerNr.value + coreLayer]++;
    }
#ifdef AAC_ELD
      if (fd->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value == ER_AAC_ELD)
        *numberOfAacLayer = 1;
      else
#endif
      *numberOfAacLayer = fd->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.specConf.TFSpecificConfig.layerNr.value + 1;  
  }

  *firstAacStream   = tmpFirstAacStream;
}

/***************************************************************************************
   Function     : mergeScalEp1Layer()
   Purpose      : merge according epConfig1 streams in one aac layer 
                  and get back the right mode from OD
   Arguments    : fd                    = FRAME_DATA pointer
                  hFault                = HANDLE_DECODER_GENERAL pointer
                                    
   Returns      : lowRateChannelPresent = core coder present
                  layNumChan            = number of channels per layer
                  lastAacLay            = last aac layer
                  firstStLay            = first aac stereo layer
                  coreCodecIdx          = core coder index
                  commonWindow          = common window
                  nrOfCoreStreams       = number of core coder streams
                  output_select         = right output select

   Author       : Thomas Breitung, FhG IIS-A
   last changed : 2003-11-24
****************************************************************************************/

static void mergeScalEp1Layer( FRAME_DATA* fd,
                               HANDLE_DECODER_GENERAL hFault,
                               int *lowRateChannelPresent,
                               int *layNumChan,
                               int *lastAacLay,
                               int *firstStLay,
                               enum CORE_CODEC* coreCodecIdx,
                               int *commonWindow,
                               int *nrOfCoreStreams,
                               int *output_select,
                               int epConfig,
                               int *streamsPerLayer ) {

  int coreCoderType;
  int aacLayer         = 0;
  int hFaultLayer      = 0;
  int layerNumber      = 0;
  int numberOfAacLayer = 0;
  int coreLayer        = 0;
  int firstAacStream   = 0;
  int layer, stream    = 0;

  coreCoderType = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;

  switch ( coreCoderType ) {

    case ER_CELP:
      coreLayer = 1;
     *coreCodecIdx = CC_CELP_MPEG4;

      switch ( epConfig ) {
        /* ER_CELP epConfig 1 */
      case 1:
      case 2:
      case 3:
          getStreamsPerLayer( fd, &firstAacStream, &numberOfAacLayer, streamsPerLayer );
         *nrOfCoreStreams =  firstAacStream;
         *output_select   =  firstAacStream + numberOfAacLayer - 1;
         *lastAacLay      = *output_select;
          break;
      }
      break;

    case ER_TWIN_VQ:
      coreLayer = 1;
     *coreCodecIdx = NTT_TVQ;
      switch ( epConfig ) {
        /* ER_TWIN_VQ epConfig 1 */
      case 1:
      case 2:
      case 3:
          getStreamsPerLayer( fd, &firstAacStream, &numberOfAacLayer, streamsPerLayer );
         *nrOfCoreStreams =  firstAacStream;
         *output_select   =  firstAacStream + numberOfAacLayer - 1;
         *lastAacLay      = *output_select;
          break;
      }
      break;

    case ER_AAC_SCAL:
      switch ( epConfig ) {
      case 1:
      case 2:
      case 3:
          getStreamsPerLayer( fd, &firstAacStream, &numberOfAacLayer, streamsPerLayer );
          *lastAacLay = numberOfAacLayer - 1;
          *coreCodecIdx = NO_CORE;
          break; 
      }
  }

  /* merge ep1 aac streams to one aac layer */
  for ( stream=firstAacStream, aacLayer = 0; aacLayer<numberOfAacLayer; aacLayer++ ) {

    int layer      = aacLayer + coreLayer;
    int mergeLayer = aacLayer + firstAacStream;

    /* aacLayerNr starts with 0 in case of NO TVQ core or CELP core */
    BsPrepareScalEp1Stream( hFault[layer].hEscInstanceData, /* use the according ESC data         */
                            fd->layer,                      /* handle of all layer                */
                            mergeLayer,                     /* merged in this layer number        */
                            streamsPerLayer[layer],         /* number of the streams to be merged */
                            stream );                       /* begin with this stream             */

    stream += streamsPerLayer[layer]; /* begin with this stream if you merge the next aac layer */

  }

  /* set OD parameter */
  *lowRateChannelPresent = coreLayer;
  *firstStLay = -1;

  stream = firstAacStream;

  /* fill layNumChan and commonWindow table */   
  for ( layer=0; layer<aacLayer; layer++ ) {

    layNumChan[layer+firstAacStream] = fd->od->ESDescriptor[stream]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;

    if ( layNumChan[layer+firstAacStream] == 2 ) {
      commonWindow[layer+firstAacStream] = 1;
      if ( *firstStLay == -1 )
        *firstStLay = firstAacStream + layer;
    }   
    switch ( epConfig ) {
    case 1:
    case 2:
    case 3:
        stream += streamsPerLayer[layer+coreLayer]; /* one ESDescriptor per er aac scal stream */
        break;
    }
  }
}


void tfScaleableAUDecode ( int          numChannels, 
                           FRAME_DATA*  fd, /* config data , obj descr. etc. */
                           TF_DATA*     tfData,
                           LPC_DATA*    lpcData,                           
                           HANDLE_DECODER_GENERAL hFault,
                           enum AAC_BIT_STREAM_TYPE bitStreamType
                            )
{
  int                short_win_in_long = 8;
  HANDLE_BSBITSTREAM        gc_DummyStream[MAX_TIME_CHANNELS]={0x0};
  
  HANDLE_BSBITSTREAM  layer_stream = NULL;
  
  int                lopLong=0;
  int                lopShort=0;
  float              normBw=0.0f;
  int                shortFssWidth;
  double*            low_spec;
  int                lowRateChannelPresent;
  enum CORE_CODEC    coreCodecIdx = CC_FS1016;
#ifdef I2R_LOSSLESS
  int                lastSlsLay = 0;
#endif
  int                lastAacLay = 0;  
  int                firstAacLay = 0;  
  int                layNumChan[MAX_TF_LAYER];
  int                firstStLay;
  int                lastMonoLay;
  unsigned long      ultmp,codedBlockType=0;
  int                diffControl  [2][ 60 ] = {{0x0}};
  int                diffControlLR[2][ 60 ] = {{0x0}};

  int            commonWindow[MAX_TF_LAYER];
  int            aacLayer = 0;
  int            layer;
  int            channel,ch,calcMonoLay;
  int            diffControlBands=0;
  int            diffControlBandsExt=0,diffControlBandsLR=0 ;
  int            i;
  int            tmp = 0;
  int            msMask[8][60]={{0x0}};
  int            msMaskPres = 0;
  TNS_frame_info tns_info[MAX_TIME_CHANNELS];
  TNS_frame_info tns_info_ext[MAX_TIME_CHANNELS];
  byte           max_sfb[MAX_TF_LAYER] = {0,0,0,0,0,0,0,0,0,0};
  byte           sfbCbMap[MAX_TF_LAYER][MAX_TIME_CHANNELS][MAXBANDS] = {{{0x0}}};
  int            groupInfo = 0;
  int            coreChannels;
  int            tvqTnsPresent;
  int            output_select;
  int            samplFreqIdx=0;
  short*         pFactors[MAX_TF_LAYER][MAX_TIME_CHANNELS];
  byte           max_sfb_ch[Winds];

  int            lastStLayerMaxSfb=0;

  int            tnsChanMonoLayFromRight = 0;

  QC_MOD_SELECT      qc_select = AAC_SCALABLE;

  /* since update to new GetBits function - you need this */
  HANDLE_RESILIENCE         hResilience;
  HANDLE_ESC_INSTANCE_DATA  hEscInstanceData;
  HANDLE_BUFFER             hVm;
  int                       coreStreams = 0;
  int                       elemID = 0;
  int                       epConfig = -1;
  int                       hFaultLayer = 0;
  int                       streamsPerLayer[10] = { 0 }; 

  /* init */
  tvqTnsPresent     = 0;
  coreChannels      = 0;

  {
    int i,j,k;
    for( k=0; k<MAX_TF_LAYER; k++ ) {
      for( i=0; i <MAX_TIME_CHANNELS; i++ ) {
        for( j=0; j<MAXBANDS; j++ ) {
          sfbCbMap[k][i][j] = 0;
        }
      }
    }
  }

  for ( layer=0; layer<=tfData->output_select; layer++ ) {
    ResetReadBitCnt ( hFault[layer].hVm );
  }

  /* has to be set for all AOT's */
  hEscInstanceData = hFault[0].hEscInstanceData;
  hResilience      = hFault[0].hResilience;
  hVm              = hFault[0].hVm;

  /* Init Tns-Flags */
  if (tfData->aacDecoder) {
    SetTnsDataPresent(tfData->aacDecoder, 0, 0);
    SetTnsDataPresent(tfData->aacDecoder, 1, 0);
    SetTnsExtPresent (tfData->aacDecoder, 0, 0);
    SetTnsExtPresent (tfData->aacDecoder, 1, 0);
  }

  layer=0;
  output_select = tfData->output_select;
  epConfig = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value;


  /* merge ep1 streams to the according layer */
  if ( (bitStreamType == SCALABLE_ER) && (epConfig > 0) )
    mergeScalEp1Layer( fd,
                       hFault,
                       &lowRateChannelPresent,
                       layNumChan,
                       &lastAacLay,
                       &firstStLay,
                       &coreCodecIdx,
                       commonWindow,
                       &coreStreams,
                       &output_select,
                       epConfig,
                       streamsPerLayer );
  else {
      getModeFromOD( fd->od,
                     &lowRateChannelPresent,
                     layNumChan,
#ifdef I2R_LOSSLESS
                     &lastSlsLay,
#endif
                     &lastAacLay,
                     &firstStLay,
                     &coreCodecIdx,  
                     commonWindow);
  }
  
  if(coreCodecIdx==NTT_TVQ)
     output_select -= tfData->tvqDecoder->nttDataScl->ntt_NSclLay;
#ifndef I2R_LOSSLESS
  /* this sanity check does not work for SLS */
  if (output_select>lastAacLay) 
    output_select=lastAacLay;
#endif
  if (output_select<0) 
    output_select=0; /* YB 980508 */
  if (firstStLay>=0)
    lastMonoLay=firstStLay-1;/* debug */
  else 
    lastMonoLay=lastAacLay;/* debug */
  if (output_select > lastMonoLay ){
    calcMonoLay=lastMonoLay ;
  } else {
    calcMonoLay=output_select ;      
  }

  /* open the first layer bitbuffer  to read :
     can  be a AAC or TwinVq bitbuffer as core 
  */
  /* 
     According to the new bitstream syntax (98/11/12), 
     there is no more ics_reserved_bit for scalable
  */
  /*
    Yes, there is !! (00/12/18)
  */
  if ( coreCodecIdx==CC_CELP_MPEG4 ) {
    float coreSig[1920];
    int i,k;
    int ifactor=tfData->samplFreqFacCore;  

    /* first aac layer = layer after core layers */
    firstAacLay  = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.numEnhLayers.value + 1;
    /* get number of core channels: two channel configuration does not yet work properly */
    coreChannels = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.channelConfiguration.value;
 
    while ( GetNumVal ( tfData->coreModuloBuffer ) < tfData->block_size_samples ) {

      if (fd->layer[layer].NoAUInBuffer<=0)
        CommonExit(1,"celp decoder error, no more AccessUnits");

      DecLpcFrame(fd,lpcData,&tmp); 

      /*
      if (tmp!=lpcData->bitsPerFrame)
        CommonExit(1,"celp decoder error, wrong num bit read");
      */

      /*    upsampling  */
      for( i=0; i< lpcData->frameNumSample ;i++ ) {
        coreSig[i*ifactor] = lpcData->sampleBuf[0][i] * ifactor;
        for( k=1; k<ifactor; k++ ) {
          coreSig[i*ifactor+k] = 0;
        }
      }            
#ifdef DEBUGPLOT 
    plotSend("c", "timeCoreOrg",  MTV_FLOAT,tfData->block_size_samples/ ifactor,lpcData->sampleBuf[0] , NULL);
#endif
    AddFloatModuloBufferValues( tfData->coreModuloBuffer,coreSig , 
                                lpcData->frameNumSample * tfData->samplFreqFacCore);
    }

    /* prepare decoding of next layer */
    if ( coreStreams > 0 ) {
      layer=coreStreams;
      firstAacLay=coreStreams;
    }
    else {
      layer++;
    }

    /* in case of 2nd celp core layer skip this one */
    if (fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.specConf.celpSpecificConfig.numEnhLayers.value != 0)
    {
      layer++;
    }

    hFaultLayer++;

  } else {
    if (fd->layer[layer].NoAUInBuffer<=0) { CommonWarning("no Access unit; skiping frame"); return ;}
    if ( layer_stream == NULL ) {
      layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf);
      setHuffdec2BitBuffer ( layer_stream );
    }
  }
  if (!(lowRateChannelPresent && (coreCodecIdx==NTT_TVQ))
      && (lastAacLay>=0)
      && (fd->layer[layer].NoAUInBuffer>0)){

    if ( layer_stream == NULL ) {
      layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf);
      setHuffdec2BitBuffer ( layer_stream );
    }

    if ( bitStreamType == SCALABLE_ER ) {
      hEscInstanceData = hFault[hFaultLayer].hEscInstanceData;
      hResilience      = hFault[hFaultLayer].hResilience;
      hVm              = hFault[hFaultLayer].hVm;

      if ( layNumChan[layer] == 1 )
        elemID = 0; /* SCE */
      else 
        elemID = 2; /* CPE & common window */    

      BsSetSynElemId( elemID ); /* set element id */
      BsSetChannel( 1 );        /* left channel */
    }  

    /* ICS reserved bit ( SCE: ESC1 / CPE: ESC0 )*/  
    ultmp = GetBits( LEN_ICS_RESERV, ICS_RESERVED, hResilience, hEscInstanceData, hVm );
    if (ultmp != 0) fprintf(stderr, "WARNING: ICS - reserved bit is not set to 0\n");

    /* window sequence ( SCE: ESC1 / CPE: ESC0 ) */
    codedBlockType = GetBits( LEN_WIN_SEQ, WINDOW_SEQ, hResilience, hEscInstanceData, hVm );

    switch (   codedBlockType )
      {
      case 0: 
        tfData->windowSequence[MONO_CHAN] =  ONLY_LONG_SEQUENCE;
        break;
      case 1: 
        tfData->windowSequence[MONO_CHAN] =  LONG_START_SEQUENCE;
        break;
      case 2: 
        tfData->windowSequence[MONO_CHAN] =  EIGHT_SHORT_SEQUENCE;
        break;
      case 3: 
        tfData->windowSequence[MONO_CHAN] =  LONG_STOP_SEQUENCE;
        break;
      /* LCM
      case 4: 
        tfData->windowSequence[MONO_CHAN] =  LONG_STOPSTART_SEQUENCE;
        break;
      case 5: 
        tfData->windowSequence[MONO_CHAN] =  MODIFIED_LONG_STOP_SEQUENCE;
        break;
      case 6:
        tfData->windowSequence[MONO_CHAN] =  LONG_START_SEQUENCE_AMR;
        break;
      case */
      default: 
        CommonExit(-1,"wrong blocktype %d",   codedBlockType);
      }

    /* window shape ( SCE: ESC1 / CPE: ESC0 ) */
    tfData->windowShape[0] = (WINDOW_SHAPE)GetBits( LEN_WIN_SH, WINDOW_SHAPE_CODE, hResilience, hEscInstanceData, hVm );
    
    if (numChannels==2){
      tfData->windowSequence[1]=tfData->windowSequence[MONO_CHAN];
      tfData->windowShape[1] =   tfData->windowShape[MONO_CHAN];
    }
  }
  
  if ( coreCodecIdx==CC_CELP_MPEG4 ) {
    float coreSig[1024];
    double dummy=0.0;
    int coreBWlines;
    /* forward mdct for celp core */
    ReadFloatModuloBufferValues(tfData->coreModuloBuffer, coreSig, tfData->block_size_samples);
#ifdef DEBUGPLOT
    plotSend("c", "timeCoreDel",  MTV_FLOAT,tfData->block_size_samples,coreSig , NULL);
#endif
    mdct_core ( tfData,
                coreSig,
                tfData->spectral_line_vector[0][0],
                tfData->block_size_samples );
    /* lowpass long: not realy necessary but usefull if upsampled celp layer  should be written directly 
       to file for debugging*/
    if (tfData->windowSequence[MONO_CHAN]== EIGHT_SHORT_SEQUENCE ){
      int sbk;      
      for( sbk=0; sbk<tfData->sfbInfo[codedBlockType]->nsbk; sbk++ ) {
        coreBWlines=(int)((float)tfData->sfbInfo[codedBlockType]->bins_per_sbk[sbk]
                          /(float)tfData->samplFreqFacCore+0.5);
        vcopy( &dummy, &tfData->spectral_line_vector[0][0][coreBWlines
                                                      +tfData->sfbInfo[codedBlockType]->bins_per_sbk[sbk]*sbk]
               , 0,1, tfData->sfbInfo[codedBlockType]->bins_per_sbk[sbk]-coreBWlines );
      }      
    } else {
      coreBWlines=(int)((float)tfData->block_size_samples/(float)tfData->samplFreqFacCore+0.5);
      vcopy( &dummy, &tfData->spectral_line_vector[0][0][coreBWlines], 0,1, tfData->block_size_samples-coreBWlines );
    }
#ifdef DEBUGPLOT 
/*     vcopy( &dummy, &tfData->spectral_line_vector[0][0][0], 0,1, 960 ); */
    plotSend("c", "mdctCore",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[0][0] , NULL);
    plotSend("c", "timeCore",  MTV_FLOAT,tfData->block_size_samples,coreSig , NULL);
#endif
  }
  
  
  /* decode TVQ core layer if present */
  if (lowRateChannelPresent && (coreCodecIdx==NTT_TVQ)) {
    ntt_INDEX     index, index_scl;
    double dummy=0.0;
    
    AUDIO_SPECIFIC_CONFIG *audSpC = &fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig;
  
    {
      int fsampf[16]={96,88,64,48,44,32,24,22,16,12,11,8,-1,-1,-1,-1};
      index.isampf=
        fsampf[ fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value];
      /* HP20001012 samplingFreqencyIndex==0xf not handled */
      index_scl.isampf = index.isampf;
    }
    
    vcopy( &dummy, &tfData->spectral_line_vector[0][0][0], 0,1,tfData->block_size_samples );
    
    index.nttDataBase=tfData->tvqDecoder->nttDataBase;
    index.nttDataScl = tfData->tvqDecoder->nttDataScl;
    index_scl.nttDataBase=tfData->tvqDecoder->nttDataBase;
    index_scl.nttDataScl=tfData->tvqDecoder->nttDataScl;

    index.numChannel =audSpC->channelConfiguration.value;
    index_scl.numChannel =audSpC->channelConfiguration.value;
    index.block_size_samples=
      ( fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.
        specConf.TFSpecificConfig.frameLength.value ? 960:1024) ;
    index_scl.block_size_samples= index.block_size_samples;
    index.w_type=(WINDOW_SEQUENCE)codedBlockType; 
    /* maybe obsolete ???   HP 990201 ... */
    index.ms_mask = 0;
    index.group_code = 0x7F ; /*default */
    index.last_max_sfb[0] = 0;
  /* ... HP */
    coreChannels      = index.numChannel;
    tvqAUDecode(  fd, /* config data , obj descr. etc. */
                  tfData,
                  hFault,
                  qc_select,
                  &index,
                  &index_scl,
                  &layer);

    BsCloseRemove ( layer_stream ,1);
    layer_stream = NULL;

    /*oldcode???*/
    if ((epConfig == 0) || (epConfig == 2)) {
    
      /* index_scl.nttDataScl->ntt_NSclLay == num of tvq enhance layers */
      firstAacLay = layer - index_scl.nttDataScl->ntt_NSclLay; 
      lastMonoLay -= index_scl.nttDataScl->ntt_NSclLay;
      firstStLay -= index_scl.nttDataScl->ntt_NSclLay;

      if(firstStLay < -1) firstStLay = -1;
      if(firstAacLay <-1) firstAacLay = -1;
      if(lastMonoLay <-1) lastMonoLay = -1;

#ifndef _MSC_VER
      /*#warning "max_sfb problem unresolved" */
#endif
      /* im not shure which one of of the following 2 lines is correct ? */
      /*max_sfb[layer-1]=index.max_sfb[0]; */
      /*max_sfb[layer-1]=index_scl.max_sfb[0]; */
      if (layer>0 && firstAacLay>0) {
        max_sfb[firstAacLay-1]=index_scl.max_sfb[layer-1];
      } else {
        firstAacLay = coreStreams;
        if (layer>0 && firstAacLay>0) 
          max_sfb[firstAacLay-1]=index_scl.max_sfb[0];
      }
    }
    else {
      firstAacLay = coreStreams;
      if (layer>0 && firstAacLay>0) 
        max_sfb[firstAacLay-1]=index_scl.max_sfb[0];
    }

    if ((layer>0 && firstAacLay>0) &&
        (output_select>=layer) && (index.numChannel>1)) {
      int iii,jjj;

      /* OK 20031213: TwinVQ always inverts MS, AAC expects M/S data */
      lastStLayerMaxSfb=max_sfb[firstAacLay-1];
      for(iii=0; iii<8; iii++){
        for(jjj=0; jjj<lastStLayerMaxSfb; jjj++){
          msMask[iii][jjj] = index_scl.msMask[iii][jjj];
        }
      }

      doMsMatrix(tfData->sfbInfo[index.w_type],
                 tfData->spectral_line_vector[0][0],
                 tfData->spectral_line_vector[0][1], msMask);
    }

    if (numChannels == 2){
      tfData->windowSequence[1] = tfData->windowSequence[MONO_CHAN];
      tfData->windowShape[1]    = tfData->windowShape[MONO_CHAN];
    } 

    if ( bitStreamType == SCALABLE_ER ) {
      hFaultLayer++;
      /* apply epConfig data to the according layer */
      hEscInstanceData = hFault[hFaultLayer].hEscInstanceData;
      hResilience      = hFault[hFaultLayer].hResilience;
      hVm              = hFault[hFaultLayer].hVm;

      if ( layNumChan[layer] == 1 )
        elemID = 0; /* SCE */
      else 
        elemID = 2; /* CPE & common window */    

      BsSetSynElemId( elemID ); /* set element id */
      BsSetChannel( 1 /* 1:LEFT / 2:RIGHT */ );        /* left channel */
    }

#ifdef DEBUGPLOT 
    plotSend("bevTns", "mdctCore",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[0][0] , NULL);
#endif

  }

  /* allocate scalefactor mem for each aac layer ( necessary for PNS and IS ) */
  for ( aacLayer=firstAacLay; aacLayer<=lastAacLay; aacLayer++ ) {
    pFactors[aacLayer][0] = (short*)calloc(MAXBANDS,sizeof(short));
    pFactors[aacLayer][1] = (short*)calloc(MAXBANDS,sizeof(short));
  }

  tmp = 0;
  aacLayer=firstAacLay;

  if (lastAacLay>=0){
    if (fd->layer[layer].NoAUInBuffer>0){
      samplFreqIdx=fd->od->ESDescriptor[layer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value;

      if (!(!(lowRateChannelPresent && (coreCodecIdx==NTT_TVQ))
            && (lastAacLay>=0)
            && (fd->layer[layer].NoAUInBuffer>0))){

        if ( layer_stream == NULL ) {
          layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf); 
          setHuffdec2BitBuffer ( layer_stream );
        }
        
        ultmp = GetBits( LEN_ICS_RESERV, ICS_RESERVED, hResilience, hEscInstanceData, hVm );
        if (ultmp != 0) fprintf(stderr, "WARNING: ICS - reserved bit is not set to 0\n");

      }

      if (tfData->windowSequence[MONO_CHAN]== EIGHT_SHORT_SEQUENCE ) {

        ultmp = GetBits( 4, MAX_SFB, hResilience, hEscInstanceData, hVm );
        max_sfb[firstAacLay] = (unsigned char) ultmp;

        ultmp = GetBits( 7, SCALE_FACTOR_GROUPING, hResilience, hEscInstanceData, hVm );
        groupInfo = ultmp;

        tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->num_groups = 
        decode_grouping( ultmp, tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->group_len );
 
      }
      else {
        /* MAX_SFB_CODE */
        max_sfb[firstAacLay] = (unsigned char)GetBits( 6, MAX_SFB, hResilience, hEscInstanceData, hVm );
      }
      /* end of ics_info()*/
      if (firstAacLay>0) {
        if (max_sfb[firstAacLay-1]==0) {
          if (tfData->windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
            int tmp_sfb = 0;
            int tmp_line = 0;
            while (tmp_line < tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->shortFssWidth) {
              tmp_line += tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sfb_width_short[tmp_sfb++];
            }
            max_sfb[firstAacLay-1] = tmp_sfb-1;
          } else {
            max_sfb[firstAacLay-1] = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->longFssGroups*4-1;
          }
        }
        if (max_sfb[firstAacLay] < max_sfb[firstAacLay-1]) {
          CommonWarning("max_sfb decreasing in layer %i(?) from %i to %i",firstAacLay,max_sfb[firstAacLay-1],max_sfb[firstAacLay]);
        }
      }
  
      /* rest of aac_scaleable_main_header */  
      if( layNumChan[firstAacLay] == 2 ) {
        /* OK 20031312: fixed behavior for (TwinVQ-)core with MS */
        DecodeMS( tfData, lastStLayerMaxSfb, max_sfb[firstAacLay], msMask, hResilience, hEscInstanceData, hVm );
      }
  
      diffControlBands =0;
      /* tns_channel_mono_layer bit (To DO !!) */

      if((layNumChan[firstAacLay]==2) && (coreChannels == 1) && lowRateChannelPresent && !tvqTnsPresent) {
        tns_info[0].tnsChanMonoLayFromRight = tnsChanMonoLayFromRight = GetBits( 1, TNS_CHANNEL_MONO, hResilience, hEscInstanceData, hVm );
      }
 
      for (ch=0;ch<layNumChan[firstAacLay];ch++){

        BsSetChannel(ch + 1); /* set er_bitbuffer offset to the right position */

        /* TNS data present*/
        ultmp = GetBits( 1, TNS_DATA_PRESENT, hResilience, hEscInstanceData, hVm );
        SetTnsDataPresent(tfData->aacDecoder, ch, ultmp);

        if( bitStreamType != SCALABLE_ER) {
          if( ultmp ) {
            get_tns_vm( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], &tns_info[ch], hResilience, hEscInstanceData, hVm);
          } else {
            clr_tns( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], &tns_info[ch] );
          }
        }

        if (coreCodecIdx!=NO_CORE) {
          if ((ch==0) || ((ch==1) && (coreChannels==2 ) ) ){
            if (tfData->windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
              int g;
              for (g=0;g<8;g++){
                diffControl[ch][g] = GetBits( 1, DIFF_CONTROL_LR, hResilience, hEscInstanceData, hVm);
                if (ch==0){
                  diffControlBands +=1;
                }
              }
            } else {
              if (ch==0){              
                if ((coreCodecIdx==CC_CELP_MPEG4) && (firstAacLay==aacLayer)) {
                  diffControlBands =tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->longFssGroups*4;
                } else {
                  diffControlBands = ((max_sfb[aacLayer-1]+3)/4)*4; /* for tvq */                
                  tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->longFssGroups= (max_sfb[aacLayer-1]+3)/4;
                }
              }
              decodeDiffCtrl( diffControl[ch],
                              tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->longFssGroups, /*  FSSwitch 1st aacLayer */
                              hResilience,
                              hEscInstanceData,
                              hVm);
            }
          }
          if ((coreChannels == 1) && (layNumChan[firstAacLay]==2)){
            int g,sfb;
            /* diffcontrol lr */
            if ((coreCodecIdx==CC_CELP_MPEG4) && (firstAacLay==aacLayer) ) {
              diffControlBands = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->longFssGroups*4;
            } else {
              diffControlBands = max_sfb[aacLayer-1]; /* for tvq */
            }

            /* OK 20040104: see table 4.18 : minimum of the two max_sfbs */
            if (diffControlBands > max_sfb[aacLayer])
              diffControlBands = max_sfb[aacLayer];

            if (tfData->windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
              for (g=0;g<8;g++){
                /* short FSS */
                diffControlLR[ch][g] = GetBits( 1, DIFF_CONTROL_LR, hResilience, hEscInstanceData, hVm);             
              }

            } else {
              for (sfb=0;sfb< diffControlBands  ;sfb++){
                if (msMask[0][sfb]==0) {
                  /* long FSS */
                  diffControlLR[ch][sfb] = GetBits( 1, DIFF_CONTROL_LR, hResilience, hEscInstanceData, hVm);
                } else {
                  /* use the one already decoded above for mid chan 
                     if ms is on  */
                  diffControlLR[0][sfb]= diffControl[0][sfb];
                  /* no diff for side Chan if ms is on   */
                  diffControlLR[1][sfb]= 1;
                }
              }
            }
          }
        } else {
          if(tfData->pred_type == NOK_LTP)
            {
            /* LTP data. */
            nok_lt_decode(tfData->windowSequence[MONO_CHAN], &max_sfb[firstAacLay], 
                          tfData->nok_lt_status[ch]->sbk_prediction_used, 
                          tfData->nok_lt_status[ch]->sfb_prediction_used, 
                          &tfData->nok_lt_status[ch]->weight, tfData->nok_lt_status[ch]->delay,
                          hResilience, 
                          hEscInstanceData, 
                          qc_select, 
                          NO_CORE, 
                          -1, 
                          hVm);
            }
	          else
            {
              /* has to be checked !!! */
              BsGetBit(layer_stream, &ultmp, 1);  /* no prediction data  */
            }
        }
    
      max_sfb_ch[0]=max_sfb[aacLayer];

      /* decode 1st (ER)AAC Layer */
      if (bitStreamType == SCALABLE_ER) {
        AACDecodeFrame( tfData->aacDecoder,
                        layer_stream,
                        gc_DummyStream,
                        tfData->spectral_line_vector[aacLayer], 
                        &(tfData->windowSequence[MONO_CHAN]), 
                        tfData->windowShape,
                        bitStreamType,
                        max_sfb_ch,
                        layNumChan[aacLayer],
                        commonWindow[aacLayer],
                        tfData->sfbInfo, 
                        sfbCbMap[aacLayer],
                        pFactors[aacLayer],
                        &hFault[hFaultLayer],
                        qc_select,
                        tfData->nok_lt_status,
                        fd,
                        layer,
                        ch,    /* er_channel */
                        0    /* extensionLayerFlag */
                        );

        if(!GetTnsDataPresent(tfData->aacDecoder, ch))
          clr_tns ( *(tfData->sfbInfo), &tns_info[ch] );
        else {
          tns_info[ch] = GetTnsData(tfData->aacDecoder, ch);
          tns_info[0].tnsChanMonoLayFromRight = tnsChanMonoLayFromRight;
        }
      }
      else if ( ch == layNumChan[firstAacLay]-1 ) {
        AACDecodeFrame( tfData->aacDecoder,
                        layer_stream,
                        gc_DummyStream,
                        tfData->spectral_line_vector[aacLayer], 
                        &(tfData->windowSequence[MONO_CHAN]), 
                        tfData->windowShape,
                        bitStreamType,
                        max_sfb_ch,
                        layNumChan[aacLayer],
                        commonWindow[aacLayer],
                        tfData->sfbInfo, 
                        sfbCbMap[aacLayer],
                        pFactors[aacLayer],
                        hFault,
                        qc_select,
                        tfData->nok_lt_status,
                        fd,
                        layer,
                        0,     /* er_channel */
                        0   /* extensionLayerFlag */
                        );

#ifdef I2R_LOSSLESS
        for (i = 0; i < layNumChan[firstAacLay]; i++)
          {
            tfData->ll_quantInfo[i][0]->ll_present= 1;
            memset(tfData->ll_quantInfo[i][0]->quantCoef, 0,  1024*sizeof(int)); 
            memset(tfData->ll_quantInfo[i][0]->table, 0,  MAXBANDS*sizeof(byte)); 
            memset(tfData->ll_quantInfo[i][0]->scale_factor, 0,  MAXBANDS*sizeof(short)); 
            
            memcpy(tfData->ll_quantInfo[i][0]->quantCoef, tfData->aacDecoder->sls_quant_channel_temp[i], 1024*sizeof(int)); 
            memcpy(tfData->ll_quantInfo[i][0]->table, sfbCbMap[aacLayer][i], max_sfb[firstAacLay]*sizeof(byte)); 
            memcpy(tfData->ll_quantInfo[i][0]->scale_factor, pFactors[aacLayer][i], max_sfb[firstAacLay]*sizeof(short)); 
            
            tfData->ll_quantInfo[i][0]->info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
            tfData->ll_quantInfo[i][0]->max_sfb = max_sfb[0];
            
            calc_sfb_offset_table(tfData->ll_quantInfo[i][0], tfData->/* aacDecoder->mip-> */osf);
          }
        for (;i<numChannels;i++)
          {
            tfData->ll_quantInfo[i][0]->ll_present = 0;
          }
#endif
      }
      }


      if (tfData->mdctCoreOnly==1) { /*for debugging: "-core_mdct" */
        double dummy=0.0;
        
        vcopy( &dummy, &tfData->spectral_line_vector[aacLayer][0][0], 0,1, tfData->block_size_samples );
        vcopy( &dummy, &tfData->spectral_line_vector[aacLayer][1][0], 0,1, tfData->block_size_samples );
      }

#ifdef DEBUGPLOT 
      plotSend("l", "mdctL1",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[aacLayer][0] , NULL);
      if (layNumChan[aacLayer]==2)
        plotSend("r", "mdctR1",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[aacLayer][1] , NULL);
#endif   

    removeAU( layer_stream,
              BsCurrentBit((HANDLE_BSBITSTREAM)layer_stream),
              fd,
              aacLayer);
    BsCloseRemove ( layer_stream ,1);
    layer_stream = NULL;

    if (layNumChan[firstAacLay] == 2)
      lastStLayerMaxSfb = MAX(lastStLayerMaxSfb,max_sfb[firstAacLay]);
      
    /* end of 1st aac layer */
    layer++;
    hFaultLayer++;
    } 

    /* read and decode aac_scaleable_extension_stream(s) */
    for ( aacLayer=firstAacLay+1;aacLayer<=lastAacLay;aacLayer++)   /* maximum was -1 for I2R_LOSSLESS */
      {

      if (aacLayer!=layer)
        CommonExit(-1,"layer error");
      samplFreqIdx=fd->od->ESDescriptor[aacLayer]->DecConfigDescr.audioSpecificConfig.samplingFreqencyIndex.value;
      if (fd->layer[layer].NoAUInBuffer<=0) continue ;

      if ( layer_stream == NULL ) {
        layer_stream= BsOpenBufferRead(fd->layer[layer].bitBuf); 
        setHuffdec2BitBuffer ( layer_stream );
      }

      if ( bitStreamType == SCALABLE_ER ) {
        /* apply epConfig data to the according layer */
        hEscInstanceData = hFault[hFaultLayer].hEscInstanceData;
        hResilience      = hFault[hFaultLayer].hResilience;
        hVm              = hFault[hFaultLayer].hVm;

        if ( layNumChan[layer] == 1 )
          elemID = 0; /* SCE */
        else 
          elemID = 2; /* CPE & common window */    

        BsSetSynElemId( elemID ); /* set element id */
        BsSetChannel( 1 /* 1:LEFT / 2:RIGHT */ );        /* left channel */
      }
    
      if (tfData->windowSequence[MONO_CHAN]== EIGHT_SHORT_SEQUENCE){
        /* MAX_SFB short */
        ultmp = GetBits( 4, MAX_SFB, hResilience, hEscInstanceData, hVm );
        max_sfb[aacLayer] = (unsigned char) ultmp;
      } else {
        /* MAX_SFB long */
        ultmp = GetBits( 6, MAX_SFB, hResilience, hEscInstanceData, hVm );
        max_sfb[aacLayer] = (unsigned char) ultmp;
      }
      if (max_sfb[aacLayer] < max_sfb[aacLayer-1]) {
        CommonWarning("max_sfb decreasing in layer %i(?) from %i to %i",aacLayer,max_sfb[aacLayer-1],max_sfb[aacLayer]);
      }

      /* JOINT STEREO */
      if( layNumChan[aacLayer] == 2 ) {
        /* kaehleof: including fix from 14496V2 - Cor 2, should be equivalent */
        int last_max_sfb_ms = (layNumChan[aacLayer-1] == 2)?max_sfb[aacLayer-1] : 0;
        DecodeMS( tfData, /*lastStLayerMaxSfb*/last_max_sfb_ms, max_sfb[aacLayer], msMask, hResilience, hEscInstanceData, hVm );
      }

      diffControlBandsExt = 0;            
      diffControlBandsLR  = 0;            

      for (ch=0;ch<layNumChan[aacLayer];ch++){

        BsSetChannel( ch + 1 ); /* select the right esc */

        /* TNS DATA */
        if ( (layNumChan[aacLayer-1]!=layNumChan[aacLayer]) ) {
          if ( (layNumChan[aacLayer-1] == 1) && (layNumChan[aacLayer] == 2)  ){
        
            for (channel=ch; channel<layNumChan[aacLayer]; channel++) {

              ultmp = GetBits( 1, TNS_DATA_PRESENT, hResilience, hEscInstanceData, hVm );
              SetTnsExtPresent(tfData->aacDecoder, channel, ultmp);

              if ( bitStreamType != SCALABLE_ER ) {
                if( ultmp ) {
                  get_tns_vm( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], &tns_info_ext[channel], hResilience, hEscInstanceData, hVm);
                } else {
                  clr_tns( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], &tns_info_ext[channel] );
                }
              }
              else {
                break;
              }
            }
          }
        }

        /* DIFF CONTROL */
        if ( (layNumChan[aacLayer]==2) && ((layNumChan[firstAacLay]==1) ||  (coreChannels==1))  ) {

          int sfb,g;

          if ( (layNumChan[firstStLay-1] == 1) && (layNumChan[aacLayer] == 2)  ){

            for (channel=ch;channel<layNumChan[aacLayer];channel++){            

              diffControlBandsExt = min(max_sfb[aacLayer],max_sfb[lastMonoLay]);
              diffControlBandsLR  = diffControlBandsExt;           
 
              if (tfData->windowSequence[MONO_CHAN] ==  EIGHT_SHORT_SEQUENCE){
                for (g=0;g<8;g++){
                  if (lastStLayerMaxSfb==0){
                    /* short FSS */
                    diffControlLR[channel][g] = GetBits( 1, DIFF_CONTROL_LR, hResilience, hEscInstanceData, hVm);
                  }
                }
              } else {
                for (sfb=lastStLayerMaxSfb;sfb<diffControlBandsLR ;sfb++){
                  if (msMask[0][sfb]==0) {
                    /* long FSS */
                    diffControlLR[channel][sfb] = GetBits( 1, DIFF_CONTROL_LR, hResilience, hEscInstanceData, hVm);
                  } else {
                    diffControlLR[channel][sfb]= 0;/* allways on if ms on  channel */
                  }
                }
              }
              if ( bitStreamType == SCALABLE_ER ) {
                break;
              }
            }
          }
        }

        max_sfb_ch[0]=max_sfb[aacLayer];

        if (aacLayer!=layer)
          CommonExit(-1,"layer error");


        if (bitStreamType == SCALABLE_ER) {
          AACDecodeFrame( tfData->aacDecoder,
                          layer_stream,
                          gc_DummyStream, /* gc_WRstream, */
                          tfData->spectral_line_vector[aacLayer], 
                          &(tfData->windowSequence[MONO_CHAN]), 
                          tfData->windowShape,
                          bitStreamType,
                          max_sfb_ch,
                          layNumChan[aacLayer],
                          commonWindow[aacLayer],
                          tfData->sfbInfo, 
                          sfbCbMap[aacLayer],
                          pFactors[aacLayer],
                          &hFault[hFaultLayer],
                          qc_select,
                          tfData->nok_lt_status,
                          fd,
                          layer,
                          ch,    /* er_channel */
                          1   /* extensionLayerFlag */
                          );

          tns_info[0].tnsChanMonoLayFromRight = tnsChanMonoLayFromRight;
        }
        else {
          AACDecodeFrame( tfData->aacDecoder,
                          layer_stream,
                          gc_DummyStream, /* gc_WRstream, */
                          tfData->spectral_line_vector[aacLayer], 
                          &(tfData->windowSequence[MONO_CHAN]), 
                          tfData->windowShape,
                          bitStreamType,
                          max_sfb_ch,
                          layNumChan[aacLayer],
                          commonWindow[aacLayer],
                          tfData->sfbInfo, 
                          sfbCbMap[aacLayer],
                          pFactors[aacLayer],
                          hFault,
                          qc_select,
                          tfData->nok_lt_status,
                          fd,
                          layer,
                          0,     /* er_channel */
                          1   /* extensionLayerFlag */
                          );
          
#ifdef I2R_LOSSLESS
          for (i = 0; i < layNumChan[aacLayer]; i++) 
            {
              tfData->ll_quantInfo[i][aacLayer]->ll_present = 1;
              memset(tfData->ll_quantInfo[i][aacLayer]->quantCoef, 0,  1024*sizeof(int)); 
              memset(tfData->ll_quantInfo[i][aacLayer]->table, 0,  MAXBANDS*sizeof(byte)); 
              memset(tfData->ll_quantInfo[i][aacLayer]->scale_factor, 0,  MAXBANDS*sizeof(short)); 
              
              memcpy(tfData->ll_quantInfo[i][aacLayer]->quantCoef, tfData->aacDecoder->sls_quant_channel_temp[i], 1024*sizeof(int)); 
              memcpy(tfData->ll_quantInfo[i][aacLayer]->table, sfbCbMap[aacLayer][i], max_sfb[aacLayer]*sizeof(byte)); 
              memcpy(tfData->ll_quantInfo[i][aacLayer]->scale_factor, pFactors[aacLayer][i], max_sfb[aacLayer]*sizeof(short)); 
              
              tfData->ll_quantInfo[i][aacLayer]->info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
              tfData->ll_quantInfo[i][aacLayer]->max_sfb = max_sfb[aacLayer];
              
              calc_sfb_offset_table(tfData->ll_quantInfo[i][aacLayer], tfData->/* aacDecoder->mip-> */osf);
            }
          for (i;i<numChannels;i++)
            {
              tfData->ll_quantInfo[i][aacLayer]->ll_present = 0;
            }
#endif
          
          
          break;
        }
      }

      if (tfData->mdctCoreOnly==1) { /*for debugging: "-core_mdct" */
        double dummy=0.0;
        
        vcopy( &dummy, &tfData->spectral_line_vector[aacLayer][0][0], 0,1, tfData->block_size_samples );
        vcopy( &dummy, &tfData->spectral_line_vector[aacLayer][1][0], 0,1, tfData->block_size_samples );
      }

      removeAU( layer_stream,
                BsCurrentBit((HANDLE_BSBITSTREAM)layer_stream),
                fd,
                aacLayer);

      BsCloseRemove ( layer_stream,1 );
      layer_stream = NULL;

      if (layNumChan[layer] == 2)
        lastStLayerMaxSfb = MAX(lastStLayerMaxSfb,max_sfb[layer]);

      layer++;
      hFaultLayer++;

#ifdef DEBUGPLOT 
      plotSend("l", "mdctL2",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[aacLayer][0] , NULL);
      if (layNumChan[aacLayer]==2)
        plotSend("r", "mdctL2",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[aacLayer][1] , NULL);
#endif    

    }

#ifdef I2R_LOSSLESS  
    /*********  Extract LLE Info  **********/
    if (lastSlsLay > 0) {
      SLSDecodeFrame(   
                     tfData->aacDecoder,
                     tfData,
                     tfData->spectral_line_vector[aacLayer],
                     tfData->windowSequence,
                     tfData->windowShape,
                     max_sfb,
                     layNumChan[lastSlsLay],
                     commonWindow[aacLayer],
                     msMask,
                     fd,
                     hFault,
                     lastSlsLay
                     );
      /* xxxxxxxxxxxxxxxxxxxxx new */
    }

    {
      int aot = fd->od->ESDescriptor[output_select]->DecConfigDescr.audioSpecificConfig.audioDecoderType.value;    /* to chk: which layer ? */

      if ((aot == SLS) || (aot == SLS_NCORE)) {
        /* clean up */
        for ( aacLayer=firstAacLay; aacLayer<=lastAacLay; aacLayer++ ) {
          free(pFactors[aacLayer][0]);
          free(pFactors[aacLayer][1]);
        }
        return;
      }
    }
#endif

    /* recalc bk_sfb_top because offsets refer to grouped sfbs (see calc_gsfb_table())*/
    if (tfData->windowSequence[MONO_CHAN] == EIGHT_SHORT_SEQUENCE) {
      int i, j, k, n;
      Info *ip =  tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
      const short *sfbands;
   
      ip->sfb_per_bk = 0;   
      k = 0;
      n = 0;
      for (i=0; i<ip->nsbk; i++) {
        /* compute bins_per_sbk */
        ip->bins_per_sbk[i] = ip->bins_per_bk / ip->nsbk;
 	    
        /* compute sfb_per_bk */
        ip->sfb_per_bk += ip->sfb_per_sbk[i];
 
        /* construct default (non-interleaved) bk_sfb_top[] */
        sfbands = ip->sbk_sfb_top[i];
        for (j=0; j < ip->sfb_per_sbk[i]; j++)
          ip->bk_sfb_top[j+k] = sfbands[j] + n;
 
        n += ip->bins_per_sbk[i];
        k += ip->sfb_per_sbk[i];
      }	    
    }  

  if (&tns_info[0] != NULL)   {
    if ( (coreCodecIdx!=NO_CORE) && (output_select > 0) && (tfData->tnsBevCore==0)  ) {
    /* tns encoder for mono/core  layer */
    int k, j;
    Info *info =tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
    info->islong = (tfData->windowSequence[MONO_CHAN] != EIGHT_SHORT_SEQUENCE);
    if (layNumChan[firstAacLay]==2)
      ch =  tns_info[0].tnsChanMonoLayFromRight;
    else
      ch = 0;
    for (i=j=0; i<tns_info[ch].n_subblocks; i++) {
      Float tmp_spec[1024];
      for( k=0; k < info->bins_per_sbk[i]; k++ ) {
        tmp_spec[k] = tfData->spectral_line_vector[0][0][j+k];
      }
      tns_encode_subblock( tmp_spec,
                           max_sfb[firstAacLay],   /* max_sfb[wn] Attention: wn and not ch !! */
                           info->sbk_sfb_top[i],
                           info->islong,
                           &(tns_info[ch].info[i]),
                           qc_select,samplFreqIdx);      
        
      for( k=0; k < info->bins_per_sbk[i]; k++ ) {
        tfData->spectral_line_vector[0][0][j+k] = tmp_spec[k];
      }
        
      j += info->bins_per_sbk[i];
    }
#ifdef DEBUGPLOT 
    plotSend("aftTns", "mdctCore",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[0][0] , NULL);
#endif
  }
 } 
 
#ifndef TEST_NEW_FUNCTIONS

  /* compute PNS and restore spectrum */
  {
    int sfb, maxSfb, lineIdx=0;
    int prevSfbCb,sfbCb, sfbWidth;
#ifdef PNS
    double energy, zero = 0.0;

    /* if (tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->islong) { */

#endif

      for (aacLayer=firstAacLay+1; aacLayer <= calcMonoLay; aacLayer++) {       
        /* for the first layer do nothing */
        maxSfb = (max_sfb[aacLayer-1]<max_sfb[aacLayer])?max_sfb[aacLayer-1]:max_sfb[aacLayer];
        
        for (ch=0;ch<layNumChan[aacLayer];ch++) {
          int sbk;
          for( sbk=0; sbk<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk; sbk++ ) {
            
            int offset = sbk*tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_sbk[sbk];
            lineIdx=0;

            for (sfb=0; sfb<maxSfb; sfb++) {
              prevSfbCb = (int)sfbCbMap[aacLayer-1][ch][sfb];/* does not work for mono-stereo combination */
              sfbCb     = (int)sfbCbMap[aacLayer][ch][sfb];
              
              if (tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->islong==0){
                sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sfb_width_short[sfb];
              }
              else {              
                if (sfb==0) {
                  sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[sbk][sfb];
                }
                else {
                  sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[sbk][sfb]-
                    tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[sbk][sfb-1];
                }
              }

#ifdef PNS
              if (prevSfbCb == NOISE_HCB) {
                energy = scalprod(&tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 
                                  &tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 1, 1, sfbWidth);
                /* if sfb of enhancement layer has MDCT lines, set sfb of previous layer to 0 */
                if (energy > 0) {
                  vcopy(&zero, &tfData->spectral_line_vector[aacLayer-1][ch][offset+lineIdx], 0 ,1 ,sfbWidth);
                }
                else {
                  sfbCbMap[aacLayer][ch][sfb] = sfbCbMap[aacLayer-1][ch][sfb];
                  vcopy(&tfData->spectral_line_vector[aacLayer-1][ch][offset+lineIdx],
                        &tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 1, 1, sfbWidth);
                  vcopy(&zero,&tfData->spectral_line_vector[aacLayer-1][ch][offset+lineIdx], 0, 1, sfbWidth);  
                }
                if (sfbCb == NOISE_HCB) {
                  /* CommonWarning("Two consecutive layers have PNS in same SFB !"); */
                } 
              }
#endif /*PNS*/
              lineIdx += sfbWidth; 
            }
          }
        }
        
      }

#ifdef PNS 
      for (ch=0;ch<layNumChan[firstAacLay];ch++) {
        lineIdx=0;
        {
          int sbk;
          int minSfb;
	  int offset;
          for( sbk=0; sbk<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk; sbk++ ) {
            maxSfb = (max_sfb[firstAacLay]<max_sfb[calcMonoLay])?max_sfb[calcMonoLay]:max_sfb[firstAacLay];
            minSfb = (max_sfb[firstAacLay]<max_sfb[calcMonoLay])?max_sfb[firstAacLay]:max_sfb[calcMonoLay];
            offset = sbk*tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_sbk[sbk];
            lineIdx=0;
            for (sfb=minSfb; sfb<maxSfb; sfb++) {
              
              sfbCbMap[firstAacLay][0][sfb]  = sfbCbMap[calcMonoLay][0][sfb];
            }
          }
        }
      }
#endif      
  }

#endif /*TEST_NEW_FUNCTIONS*/

  }

#ifndef TEST_NEW_FUNCTIONS

  /* combine all mono layers */
  for (aacLayer=firstAacLay+1 ; aacLayer <= calcMonoLay ; aacLayer++) {


      vadd(   tfData->spectral_line_vector[firstAacLay][MONO_CHAN],
              tfData->spectral_line_vector[aacLayer][MONO_CHAN],tfData->spectral_line_vector[firstAacLay][MONO_CHAN],  
              1,1,1, tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk );

  }
#ifdef DEBUGPLOT 
    plotSend("m", "mdctAllMono",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
             tfData->spectral_line_vector[firstAacLay][0] , NULL);
    
#endif    

  /* LTP processing. */
  if(coreCodecIdx == NO_CORE && tfData->pred_type == NOK_LTP)
  {
    TNS_frame_info *tns_info_ltp[2];
    

    tns_info_ltp[0] = &tns_info[0];
    tns_info_ltp[1] = &tns_info[1];
    
    ltp_scal_reconstruct(tfData->windowSequence[MONO_CHAN],
			 *tfData->windowShape, *tfData->prev_windowShape,
			 layNumChan[firstAacLay],
			 tfData->spectral_line_vector[firstAacLay],
			 tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk,
			 short_win_in_long,
			 max_sfb[firstAacLay], msMask, tfData->nok_lt_status, 
			 tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], 
			 tns_info_ltp, qc_select,samplFreqIdx);
  }

  /* combine all stereo layers */
  /* compute PNS and restore spectrum */
#ifdef PNS    
  {
    int sfb, maxSfb, lineIdx=0;
    int prevSfbCb,sfbCb;
    short sfbWidth;
    double energy, zero = 0.0;  
    int currentChannel, otherChannel;

    for (aacLayer=firstStLay+1; aacLayer <= output_select; aacLayer++) {       
      /* for the first layer do nothing */
      maxSfb = (max_sfb[aacLayer-1]<max_sfb[aacLayer])?max_sfb[aacLayer-1]:max_sfb[aacLayer];

      for (ch=0;ch<layNumChan[aacLayer];ch++) {
        lineIdx=0;

        if (ch == 0) {currentChannel =0; otherChannel = 1;}
        if (ch == 1) {currentChannel =1; otherChannel = 0;}
        
        {
          int sbk;
          int offset;
	      for( sbk=0; sbk<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk; sbk++ ) {
            
            offset = sbk*tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_sbk[sbk];
            lineIdx=0;
            for (sfb=0; sfb<maxSfb; sfb++) {
              
              prevSfbCb = (int)sfbCbMap[aacLayer-1][ch+1][sfb];
              sfbCb     = (int)sfbCbMap[aacLayer][ch+1][sfb];
              if (tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->islong==0){
                sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sfb_width_short[sfb];
              }
              else {              
                if (sfb==0) {
                  sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[sbk][sfb];
                }
                else {
                  sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[sbk][sfb]-
                    tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[sbk][sfb-1];
                }
              }

              if ( (int)sfbCbMap[aacLayer][currentChannel+1][sfb]  == 0 
                   && (int)sfbCbMap[aacLayer][otherChannel+1][sfb] == 13 &&
                   msMask[sbk][sfb] == 1 ){
                vcopy (  &tfData->spectral_line_vector[aacLayer][currentChannel][offset+lineIdx], 
                         &tfData->spectral_line_vector[aacLayer][otherChannel][offset+lineIdx],
                         1,1 ,sfbWidth);
                sfbCbMap[aacLayer][currentChannel+1][sfb] = 
                  sfbCbMap[aacLayer][otherChannel+1][sfb];
                msMask[sbk][sfb] = 0;
              }
              if ( (int)sfbCbMap[aacLayer][currentChannel+1][sfb] == 13 
                   && (int)sfbCbMap[aacLayer][otherChannel+1][sfb] == 0 &&
                   msMask[sbk][sfb] == 1 ){
                vcopy (  &tfData->spectral_line_vector[aacLayer][currentChannel][offset+lineIdx], 
                         &tfData->spectral_line_vector[aacLayer][otherChannel][offset+lineIdx],
                         1,1 ,sfbWidth);
                sfbCbMap[aacLayer][otherChannel+1][sfb] = 
                    sfbCbMap[aacLayer][currentChannel+1][sfb];
                msMask[sbk][sfb] = 0;
              } 
              

              if (prevSfbCb == NOISE_HCB) {
                energy = scalprod(&tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 
                                  &tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 1, 1, sfbWidth);
                /* if sfb of enhancement layer has MDCT lines, set sfb of previous layer to 0 */
                if ( energy > 0 || msMask[sbk][sfb] == 1 ){/*  || sfbCb == 0){ */ /*   || sfbCb == 0 || sfbCb != 0 ){ */
                  vcopy(&zero, &tfData->spectral_line_vector[aacLayer-1][ch][offset+lineIdx], 0 ,1 ,sfbWidth);
                }
                else if (sfbCb == NOISE_HCB ) {
                  vcopy(&zero,&tfData->spectral_line_vector[aacLayer-1][ch][offset+lineIdx], 0, 1, sfbWidth); 
                }
                else {
                  if ( sbk == tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk -1 )
                    sfbCbMap[aacLayer][ch+1][offset+sfb] = sfbCbMap[aacLayer-1][ch+1][offset+sfb];
                  vcopy(&tfData->spectral_line_vector[aacLayer-1][ch][offset+lineIdx],
                        &tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 1, 1, sfbWidth);
                  vcopy(&zero,&tfData->spectral_line_vector[aacLayer-1][ch][offset+lineIdx], 0, 1, sfbWidth);
                }
              }
              else if ( prevSfbCb != NOISE_HCB && sfbCb == NOISE_HCB) {
                if ( sbk == tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk -1 )
                  sfbCbMap[aacLayer][ch+1][offset+sfb] = sfbCbMap[aacLayer-1][ch+1][offset+sfb];
                vcopy(&tfData->spectral_line_vector[aacLayer-1][ch][offset+lineIdx],
                      &tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 1, 1, sfbWidth);
                vcopy(&zero,&tfData->spectral_line_vector[aacLayer-1][ch][offset+lineIdx], 0, 1, sfbWidth);
              }
              lineIdx += sfbWidth; 
              /* offset += sfbWidth; */
            }
          }
        }
      }
    }

    for (ch=0;ch<layNumChan[firstStLay];ch++) {
      lineIdx=0;
      {
        int sbk;
        int minSfb;
	      int offset;
        for( sbk=0; sbk<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk; sbk++ ) {

          maxSfb = (max_sfb[firstStLay]<max_sfb[output_select])?max_sfb[output_select]:max_sfb[firstStLay];
          minSfb = (max_sfb[firstStLay]<max_sfb[output_select])?max_sfb[firstStLay]:max_sfb[output_select];
          offset = sbk*tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_sbk[sbk];
          lineIdx=0;
          for (sfb=minSfb; sfb<maxSfb; sfb++) {
            
            sfbCbMap[firstStLay][ch+1][sfb]  = sfbCbMap[output_select][ch+1][sfb];
          }
        }
      }
    }
  }
#endif  
  if (firstStLay!=-1 ) {
    for (aacLayer=firstStLay+1 ; aacLayer <= output_select ; aacLayer++) {
      for (ch=0;ch<layNumChan[aacLayer];ch++){

        vadd(   tfData->spectral_line_vector[firstStLay][ch],tfData->spectral_line_vector[aacLayer][ch],
                tfData->spectral_line_vector[firstStLay][ch],  
                1,1,1, tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk );
      }
    }

#ifdef DEBUGPLOT 
    plotSend("l", "mdctAllStereol",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
             tfData->spectral_line_vector[firstStLay][0] , NULL);
    plotSend("r", "mdctAllStereor",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
             tfData->spectral_line_vector[firstStLay][1] , NULL);
    
#endif    
  }

#ifdef PNS
  {
    /* Mono - Stereo Combination  */
    int sfb, maxSfb, lineIdx=0;
    int prevSfbCb,sfbCb, sfbWidth;
    double energy, zero = 0.0;
    int offset;
    int sbk;

    if ( firstStLay != firstAacLay ){
      aacLayer=firstStLay;       
      
      maxSfb = (max_sfb[calcMonoLay]<max_sfb[aacLayer])?max_sfb[calcMonoLay]:max_sfb[aacLayer];
      
      for (ch=0;ch<layNumChan[aacLayer];ch++) {
        lineIdx=0;
        for( sbk=0; sbk<tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk; sbk++ ) {
          lineIdx=0;
          offset = sbk*tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_sbk[sbk];
            
          for (sfb=0; sfb<maxSfb; sfb++) {
            prevSfbCb = (int)sfbCbMap[firstAacLay][0][sfb];             /* Mono Layer = output layer */
            sfbCb     = (int)sfbCbMap[firstStLay][ch+1][sfb];

            if (tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->islong==0){
              sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sfb_width_short[sfb];
            }
            else {              
              if (sfb==0) {
                sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[sbk][sfb];
              }
              else {
                sfbWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[sbk][sfb]-
                  tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[sbk][sfb-1];
              }
            }
            
            if (prevSfbCb == NOISE_HCB) {
              energy = scalprod(&tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 
                                &tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 1, 1, sfbWidth);
              /* if sfb of enhancement layer has MDCT lines, set sfb of previous layer to 0 */
              if (energy > 0 || msMask[sbk][sfb] == 1 ){ /*  sfbCb != NOISE_HCB) {*/
                vcopy(&zero, &tfData->spectral_line_vector[firstAacLay][0][offset+lineIdx], 0 ,1 ,sfbWidth);
              }
              /* if enhancement layer has no MDCT lines, copy lines of mono-layer onto it */
              else if ( energy <= 0 ){/* && msMask[sbk][sfb] == 0 && sfbCb == 0 ){ */ /*zero codebook */
                sfbCbMap[aacLayer][ch+1][sfb] = sfbCbMap[firstAacLay][0][sfb];
                vcopy(&tfData->spectral_line_vector[firstAacLay][0][offset+lineIdx], 
                      &tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 1 ,1 ,sfbWidth);
                if ( ch > 0 )
                  vcopy(&zero, &tfData->spectral_line_vector[firstAacLay][0][offset+lineIdx], 0 ,1 ,sfbWidth);
              } 
              if ( sfbCb == NOISE_HCB ) { /* better pns value in upper layer */
                if ( ch > 0 )
                  vcopy(&zero,&tfData->spectral_line_vector[firstAacLay][0][offset+lineIdx], 0, 1, sfbWidth);
              }

            }
            else if ( sfbCb == NOISE_HCB && prevSfbCb != NOISE_HCB ){ /* && prevSfbCb != 0 ){ */
              vcopy(&zero,&tfData->spectral_line_vector[aacLayer][ch][offset+lineIdx], 0, 1, sfbWidth);
            }
            lineIdx += sfbWidth; 
          }
        }
      }
    }
  }
#endif

#endif /*TEST_NEW_FUNCTIONS*/


#ifdef TEST_NEW_FUNCTIONS
  {

    int msMaskIs[8][60];

    memcpy(msMaskIs,msMask,sizeof(msMaskIs));

    if (firstStLay!=-1 ) {
      MapMask(firstStLay, output_select, sfbCbMap, msMask, msMaskIs, tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]);
    }

    DecodePns(firstAacLay,calcMonoLay,firstStLay,output_select,tfData->spectral_line_vector,sfbCbMap,pFactors,tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]);

    if (firstStLay!=-1 ) {
      DecodeIntensity(firstStLay,output_select,tfData->spectral_line_vector,sfbCbMap,pFactors,tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],max_sfb,tfData->nok_lt_status);
    }

    /* LTP processing. */
    
    if(coreCodecIdx == NO_CORE && tfData->pred_type == NOK_LTP) {

      TNS_frame_info *tns_info_ltp[2];
    
      tns_info_ltp[0] = &tns_info[0];
      tns_info_ltp[1] = &tns_info[1];
    
      ltp_scal_reconstruct(tfData->windowSequence[MONO_CHAN],
                           *tfData->windowShape, *tfData->prev_windowShape,
                           layNumChan[firstAacLay],
                           tfData->spectral_line_vector[firstAacLay],
                           tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk,
                           short_win_in_long,
                           max_sfb[firstAacLay], msMaskIs, tfData->nok_lt_status,
                           tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], 
                           tns_info_ltp, qc_select,samplFreqIdx);
    }

    AddSpectralLines(firstAacLay,calcMonoLay,firstStLay,output_select,tfData->spectral_line_vector,sfbCbMap,msMask,tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],max_sfb);
  }

#endif /*TEST_NEW_FUNCTIONS*/

  /* fss for aac mono layer */
  low_spec=tfData->spectral_line_vector[0][0];
  if ( (output_select > 0) && (lastMonoLay>0)  && (layNumChan[firstAacLay]==1)){
    double tmp[1024];
    if (coreCodecIdx==NTT_TVQ) {
      if (max_sfb[firstAacLay-1]>0)
        tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->shortFssWidth =
          tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[0][max_sfb[firstAacLay-1]-1];
      else 
        tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->shortFssWidth = 0;
    }
    
    shortFssWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->shortFssWidth;

#ifdef DEBUGPLOT
	plotSend("bef", "DiffM",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[firstAacLay][0] , NULL);
#endif
    if (lowRateChannelPresent) {
      FSSwitch( low_spec, tfData->spectral_line_vector[firstAacLay][MONO_CHAN],
                tmp,
                 diffControl[MONO_CHAN], tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]] ,
                diffControlBands , shortFssWidth,1 );    
      vcopy (  tmp, tfData->spectral_line_vector[firstAacLay][MONO_CHAN],1,1 ,
               tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk);
    }
    diffControlBands=diffControlBandsExt;
    low_spec=tfData->spectral_line_vector[firstAacLay][MONO_CHAN];
#ifdef DEBUGPLOT
    plotSend("aft", "DiffM",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[firstAacLay][0] , NULL);
#endif
  } 
  if(lastAacLay>=0){

    if ( ( layNumChan[firstAacLay]==1 )  && ( layNumChan[lastAacLay]==2 ) ) {
      vcopy (  tfData->spectral_line_vector[firstAacLay][MONO_CHAN] , 
               tfData->spectral_line_vector[firstAacLay][MONO_CHAN+1],
               1,1 ,tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk);        
    }
  }
  /* fss for aac stereo layer */
  if ( (output_select >= firstStLay ) && (firstStLay!=-1 ) && (layNumChan[firstAacLay]==1)){
    /* aac mono + aac stereo */
    double tmp_spec[1024];
    /* inverse ms matrix */
    
    multCoreSpecMsFac(tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],msMask,low_spec,tmp_spec);
    if (layNumChan[firstAacLay]==2 ){
      shortFssWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->shortFssWidth;
    }else{
      if  (max_sfb[aacLayer-1]!=0) {
        shortFssWidth = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[0][ max_sfb[firstStLay-1]-1] ; 
      } else {
        shortFssWidth=0;
      }
    }
    for (ch=0;ch<layNumChan[firstStLay];ch++){                    
      double tmp_aac_spec[1024];

      vcopy (  tfData->spectral_line_vector[firstStLay][ch],
               tmp_aac_spec , 
               1,1 ,tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk);        

      FSSwitchSt( tmp_spec, /* firstAacLay */
                  tmp_aac_spec, /* firstStLay */
                  tfData->spectral_line_vector[firstAacLay][ch],
                  diffControlLR[ch], 
                  diffControlBandsLR,
                  tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]], 
                  msMask,
                  ch );    
    }
#ifdef DEBUGPLOT 
      plotSend("l", "mdctBevMS",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
               tfData->spectral_line_vector[firstStLay][0] , NULL);
      if (layNumChan[lastAacLay]==2)
        plotSend("r", "mdctBevMS",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
                 tfData->spectral_line_vector[firstStLay][1] , NULL);

#endif    
    doInverseMsMatrix( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],tfData->spectral_line_vector[firstAacLay][0], 
                       tfData->spectral_line_vector[firstAacLay][1],  msMask );
  } else if ((output_select >= firstStLay ) && (firstStLay!=-1 ) ){
    /* core + aac stereo , no aac mono layer */
    double tmp_spec[1024];
#ifdef DEBUGPLOT 
    plotSend("l", "mdctBevMS",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
             tfData->spectral_line_vector[firstStLay][0] , NULL);
    if (layNumChan[lastAacLay]==2)
      plotSend("r", "mdctBevMS",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
               tfData->spectral_line_vector[firstStLay][1] , NULL);
    
#endif    
    
    if (lowRateChannelPresent){
      double tmp_aac_spec[1024];
      multCoreSpecMsFac(tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],msMask,low_spec,tmp_spec);
      for (ch=0;ch<layNumChan[firstStLay];ch++){                 
#ifdef DEBUGPLOT
        if (ch==0){
          plotSend("bef", "DiffM",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[firstStLay][0] , NULL);
          plotSend("cor", "DiffM",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tmp_spec , NULL);
        }
#endif
        if (coreCodecIdx==NTT_TVQ) {
          if ((firstAacLay>0)&&(max_sfb[firstAacLay-1]>0))
            tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->shortFssWidth =
              tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[0][max_sfb[firstAacLay-1]-1];
          else
            tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->shortFssWidth = 0;
        }
        
        vcopy (  tfData->spectral_line_vector[firstStLay][ch],
                 tmp_aac_spec , 
                 1,1 ,tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk);        
    
        FSSwitchStwCore( tmp_spec, /* core spec */
                         tmp_aac_spec, /* input */
                         tfData->spectral_line_vector[firstAacLay][ch], /* output */
                         diffControl[ch], 
                         diffControlLR[ch], 
                         diffControlBands,
                         tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]] ,
                         msMask,
                         ch );    
#ifdef DEBUGPLOT
        if (ch==0){
          plotSend("aft", "DiffM",  MTV_DOUBLE_SQA,tfData->block_size_samples,  tfData->spectral_line_vector[firstAacLay][0] , NULL);
        }
#endif
      }
    }
    doInverseMsMatrix( tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]],
                       tfData->spectral_line_vector[firstAacLay][0], 
                       tfData->spectral_line_vector[firstAacLay][1],  msMask );

  }


  if (output_select) {
    if (layNumChan[output_select]==2){
      for (ch=0;ch<layNumChan[lastAacLay];ch++){
        vcopy( tfData->spectral_line_vector[firstAacLay][ch] ,   tfData->spectral_line_vector[0][ch] , 1, 1,  
               tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk ); 
      }        
    } else {
      for (ch=0;ch<numChannels;ch++){
        vcopy( tfData->spectral_line_vector[firstAacLay][MONO_CHAN] ,   tfData->spectral_line_vector[0][ch] , 1, 1,  
               tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk ); 
      }        
    }
  } else {
    if ((lastAacLay>=0)&&(layNumChan[lastAacLay]==2)&&(layNumChan[output_select]==1)){
      vcopy( tfData->spectral_line_vector[0][0] ,   tfData->spectral_line_vector[0][1] , 1, 1,  
             tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk ); 
    }
  }

  /* Bandwidth control */
  if ((lopLong != 0) && (max_sfb[lastAacLay] != 0) && (output_select == 0)) {
    int maxline;
      
    if (normBw == 0.0F) {
      maxline = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->sbk_sfb_top[0][max_sfb[lastAacLay]-1];
    }
    else {
      maxline = (int)((float)(tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk 
                              /tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk)*normBw + 0.5);
    } 
    for (ch=0; ch<layNumChan[lastAacLay]; ch++) {
      dec_lowpass(tfData->spectral_line_vector[0][ch], lopLong, lopShort, maxline,
                  tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->bins_per_bk, tfData->windowSequence[MONO_CHAN],
                  tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]]->nsbk);
    }
  }
#ifdef DEBUGPLOT 
      plotSend("l", "mdctOutBefTns_L",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
               tfData->spectral_line_vector[0][0] , NULL);
      if (layNumChan[lastAacLay]==2)
        plotSend("r", "mdctOutBefTns",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
                 tfData->spectral_line_vector[0][1] , NULL);

#endif    

  /* tns decoder for tns of main header */
  if  ( ! ( 
           (output_select == 0) && (lowRateChannelPresent) && (tfData->tnsBevCore==0) 
           ) )  {
    int tnsMaxSfb;
    TNS_frame_info tns_stereo_info[2];
    if (layNumChan[firstAacLay] ==1 ){
      tns_stereo_info[0] = tns_info[0];
      tns_stereo_info[1] = tns_info[0];      
    } else {
      tns_stereo_info[0] = tns_info[0];
      tns_stereo_info[1] = tns_info[1];      
    }

    tnsMaxSfb = max_sfb[output_select];
      
    for (ch=0;ch<layNumChan[lastAacLay];ch++){      
      int k, j;
      Info *info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];

      if (GetTnsExtPresent(tfData->aacDecoder, ch) == 1) 
        tnsMaxSfb = max_sfb[min(output_select,lastMonoLay)];
      
      
      for (i=j=0; i<tns_stereo_info[ch].n_subblocks; i++) {
        Float tmp_spec[1024];
        if ((GetTnsExtPresent(tfData->aacDecoder, ch) != 1)
            || ( (tns_info_ext[ch].info[i].n_filt>0) && (max_sfb[lastMonoLay] <= tns_info_ext[ch].info[i].filt[0].start_band) ) ){
          for( k=0; k<info->bins_per_sbk[i]; k++ ) {
            tmp_spec[k] = tfData->spectral_line_vector[0][ch][j+k];
          }
          /* print_tns( &(tns_stereo_info[ch].info[i])); */
          tns_decode_subblock( tmp_spec,
                               tnsMaxSfb,  
                               info->sbk_sfb_top[i],
                               info->islong,
                               &(tns_stereo_info[ch].info[i]),
                               qc_select,samplFreqIdx);      
          
          for( k=0; k<info->bins_per_sbk[i]; k++ ) {
            tfData->spectral_line_vector[0][ch][j+k] = tmp_spec[k];
          }
        }
        j += info->bins_per_sbk[i];
      }
    }
  }
#ifdef DEBUGPLOT 
      plotSend("l", "mdctOutBefTnsE",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
               tfData->spectral_line_vector[0][0] , NULL);
      if (layNumChan[lastAacLay]==2)
        plotSend("r", "mdctOutBefTnsE",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
                 tfData->spectral_line_vector[0][1] , NULL);

#endif    


  /* tns decoder for tns of 1st extension header */
  if ( ( (firstStLay>=1) && (output_select >= firstStLay) && (!lowRateChannelPresent) ) 
       || ( (firstStLay>=2) && (output_select >= firstStLay) && (lowRateChannelPresent))
       ) {
    TNS_frame_info tns_stereo_info[2];
    
    tns_stereo_info[0] = tns_info_ext[0];
    tns_stereo_info[1] = tns_info_ext[1];      
    
    for (ch=0;ch<layNumChan[lastAacLay];ch++){      
      int k, j;
      Info *info = tfData->sfbInfo[tfData->windowSequence[MONO_CHAN]];
      if (GetTnsExtPresent(tfData->aacDecoder, ch) == 1){
        for (i=j=0; i<tns_stereo_info[ch].n_subblocks; i++) {
          Float tmp_spec[1024];
          for( k=0; k<info->bins_per_sbk[i]; k++ ) {
            tmp_spec[k] = tfData->spectral_line_vector[0][ch][j+k];
          }
          tns_decode_subblock( tmp_spec,
                               max_sfb[output_select],  
                               info->sbk_sfb_top[i],
                               info->islong,
                               &(tns_stereo_info[ch].info[i]),
                               qc_select,samplFreqIdx);      
          
          for( k=0; k<info->bins_per_sbk[i]; k++ ) {
            tfData->spectral_line_vector[0][ch][j+k] = tmp_spec[k];
          }
          
          j += info->bins_per_sbk[i];
        }
      }
    }
  }

#ifdef DEBUGPLOT 
      plotSend("l", "mdctOutL",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
               tfData->spectral_line_vector[0][0] , NULL);
      if (layNumChan[lastAacLay]==2)
        plotSend("r", "mdctOutR",  MTV_DOUBLE_SQA,tfData->block_size_samples,  
                 tfData->spectral_line_vector[0][1] , NULL);

#endif    

#ifdef DEBUGPLOT
  plotSend("c", "mdctCorex",  MTV_DOUBLE,tfData->block_size_samples, tfData->spectral_line_vector[0][0] , NULL);

  plotSend("l", "mdctSpecL",  MTV_DOUBLE_SQA,tfData->block_size_samples, tfData->spectral_line_vector[0][0] , NULL);
  /*   plotSend("r", "mdctSpecR" , MTV_DOUBLE,1024, spectral_line_vector[firstAacLay][1] , NULL); */
#endif  
  

#ifdef CT_SBR

  /* rescue maxSfb ( output layer ) in aacDecoder struct */
  setMaxSfb( tfData->aacDecoder, max_sfb[output_select] );

#endif

  /* clean up */
  for ( aacLayer=firstAacLay; aacLayer<=lastAacLay; aacLayer++ ) {
    free(pFactors[aacLayer][0]);
    free(pFactors[aacLayer][1]);
  }

}



