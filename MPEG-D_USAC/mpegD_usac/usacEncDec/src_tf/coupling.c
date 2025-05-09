/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS 
and edited by
Eugen Dodenhoeft (Fraunhofer IIS),
Yoshiaki Oikawa (Sony Corporation),
Mitsuyuki Hatanaka (Sony Corporation)
in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "allHandles.h"
#include "block.h"
#include "buffers.h"
#include "buffer.h"
#include "common_m4a.h"
#include "coupling.h"
#include "huffdec2.h"
#include "huffdec3.h"
#include "interface.h"
#include "nok_prediction.h"
#include "tf_main.h"

#include "nok_lt_prediction.h"
#include "port.h"
#include "allVariables.h"
#include "all.h"                 /* structs */
#include "monopredStruct.h"      /* structs */
#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */


#if (CChans > 0)

static	double	    cc_gain_scale[4];
static	int	    *cc_lpflag[CChans];
static	int	    *cc_prstflag[CChans];
static	PRED_STATUS *cc_sp_status[CChans];
static	double	    *cc_prev_quant[CChans];
static	byte	    *cc_group[CChans];
static  int         target[Chans];
static	int	    nok_use_monopred;
/* long term prediction */
static  NOK_LT_PRED_STATUS *nok_cc_ltp[Chans];
static int last_cc_rstgrp_num[CChans] = {0};
static int cc_have_malloc = 0;
int counter = 0;



void
init_cc(int *use_monopred)
{
  int i, ch;
  cc_gain_scale[3] = 2.0;

  /* predictor selection */
  if (use_monopred)
    nok_use_monopred = 1; 
  else
    nok_use_monopred = 0;
    
  /* initialize gain scale */
  for (i=2; i>=0; i--)
    cc_gain_scale[i] = sqrt(cc_gain_scale[i+1]);

  /* coupling channel predictors */
  for (ch = 0; ch < CChans; ch++) 
    {
      cc_lpflag[ch] = (int*)mal1(MAXBANDS*sizeof(*cc_lpflag[0]));
      cc_prstflag[ch] = (int*)mal1((LEN_PRED_RSTGRP+1)*sizeof(*cc_prstflag[0]));
      cc_sp_status[ch] = (PRED_STATUS*)mal1(LN*sizeof(*cc_sp_status[0]));
      
      nok_cc_ltp[ch]  = (NOK_LT_PRED_STATUS *)mal1(sizeof(*nok_cc_ltp[0]));
      cc_prev_quant[ch] = (double*)mal1(LN2*sizeof(*cc_prev_quant[0]));
      cc_group[ch] = (byte*)mal1(MAXBANDS*sizeof(*cc_group[0]));
      
      if (nok_use_monopred) 
        {
          for (i = 0; i < LN2; i++) 
            {
              init_pred_stat(&cc_sp_status[ch][i],
                             PRED_ORDER,PRED_ALPHA,PRED_A,PRED_B);
              cc_prev_quant[ch][i] = 0.;
            } 
        } 
      else 
        {
          for (i = 0; i < LN2; i++) 
            {
              cc_prev_quant[ch][i] = 0.;
            }
        }
      nok_init_lt_pred (nok_cc_ltp[ch]);  
    }
}

static int cc_hufffac(
                      Info *info, 
                      byte *group, 
                      byte *cb_map, 
#ifdef MPEG4V1
                      short global_gain,
#endif
                      short *factors,
                      unsigned short dataFlag,
                      HANDLE_RESILIENCE hResilience,
                      HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
                      HANDLE_BUFFER hVm)
{
    Hcb *hcb;
    Huffman *hcw;
    int i, b, bb, t, n, fac, is_pos;
#ifdef MPEG4V1
    int noise_pcm_flag = 1;
    int noise_nrg;
#endif

    /* clear array for the case of max_sfb == 0 */
    shortclr(factors, MAXBANDS);

    /* scale factors are dpcm relative to global gain
     * intensity positions are dpcm relative to zero
     */
    fac = 0;
    is_pos = 0;
#ifdef MPEG4V1
    noise_nrg = global_gain - NOISE_OFFSET;
#endif

    /* get scale factors */
    hcb = &book[BOOKSCL];
    hcw = hcb->hcw;
    bb = 0;
    for(b = 0; b < info->nsbk; ){
	n = info->sfb_per_sbk[b];
	b = *group++;
	for(i = 0; i < n; i++){
	    switch (cb_map[i]) {
	    case ZERO_HCB:	    /* zero book */
		factors[i] = fac;	/* no difference from band to band for zero length codes */
		break;
	    default:		    /* spectral books */
		/* decode scale factor */
              t = decode_huff_cw(hcw, /*Huffman*/
                                 dataFlag, /*dataFlag */
                                 hResilience, /*hResilience */
                                 hEscInstanceData, /*hEPInfo */
                                 hVm /*hVm */);
                                 
		if(t >= MAXFAC || t < 0)
		    return 0;
		fac += t - MIDFAC;    /* 1.5 dB */
		factors[i] = fac;
		break;
	    case BOOKSCL:	    /* invalid books */
#ifndef MPEG4V1
              /*case RESERVED_HCB:*/
#endif
              /*return 0;*/
	    case INTENSITY_HCB:	    /* intensity books */
	    case INTENSITY_HCB2:
		/* decode intensity position */
		t =decode_huff_cw(hcw, /*Huffman*/
                                 dataFlag, /*dataFlag */
                                 hResilience, /*hResilience */
                                 hEscInstanceData, /*hEPInfo */
                                 hVm /*hVm */);
                  
		is_pos += t - MIDFAC;
		factors[i] = is_pos;
		break;
#ifdef MPEG4V1
	    case NOISE_HCB:	    /* noise books */
                /* decode noise energy */
                if (noise_pcm_flag) {
                  noise_pcm_flag = 0;
                  t = getbits( NOISE_PCM_BITS ) - NOISE_PCM_OFFSET;
                }
                else
                  t = decode_huff_cw(hcw) - MIDFAC;
                noise_nrg += t;
                if (debug['f'])
                  PRINT(SE,"\n%3d %3d (noise, code %d)", i, noise_nrg, t);
                factors[i] = noise_nrg;
                break;
#endif


    	    }
	    /*if (debug['f'])
	     *  PRINT(SE,"%3d: %3d %3d\n", i, cb_map[i], factors[i]);*/
	}
	/* if (debug['f'])
	 *  PRINT(SE,"\n");*/

	/* expand short block grouping */
	if (!(info->islong)) {
	    for(bb++; bb < b; bb++) {
		for (i=0; i<n; i++) {
		    factors[i+n] = factors[i];
		}
		factors += n;
	    }
	}
	cb_map += n;
	factors += n;
    }
    return 1;
}




int getcc ( MC_Info*       mip,
            Info*          info,
            HANDLE_AACDECODER hDec,
            PRED_TYPE pred_type,
            WINDOW_SEQUENCE*  cc_wnd, 
            Wnd_Shape*     cc_wnd_shape, 
            double**        cc_coef,
            double*         cc_gain[CChans][Chans],
            byte*          cb_map[Chans],
            enum AAC_BIT_STREAM_TYPE bitStreamType,
            QC_MOD_SELECT            qc_select,
            HANDLE_BSBITSTREAM gc_stream,
            HANDLE_BUFFER  hVm,
            HANDLE_BUFFER  hHcrSpecData,
            HANDLE_HCR     hHcrInfo,
            HANDLE_ESC_INSTANCE_DATA hEscInstanceData,
            HANDLE_RESILIENCE hResilience,
            HANDLE_CONCEALMENT hConcealment
            )
{
  int i, j, k, m, n, cpe, tag, cidx, ch, nele, nch, samplFreqIdx, dataFlag;
  int shared[Chans];
  int cc_l, cc_r, cc_dom, cc_gain_ele_sign, ind_sw_cc, cgep, scl_idx, sign, fac;
  byte cc_max_sfb[CChans];
  short global_gain, factors[MAXBANDS];
  double scale,cc_scale;
  TNS_frame_info tns;
  int target_multiple[Chans]; /* if coupling channel goes several times onto one channel,
                                 keep the gains separately */

  samplFreqIdx = mip->sampling_rate_idx;
  
  for (i=0;i<Chans;i++)
    target_multiple[i]=0;
  
  tag = GetBits(LEN_TAG,
                ELEMENT_INSTANCE_TAG,  
                hResilience,
                hEscInstanceData,
                hVm );

  dataFlag = 1; /* decode spectral data */

  if (default_config) {
    cidx = mip->ncch;
  }
  else {
    cidx = XCChans;	    /* default is scratch index */
    for (i=0; i<mip->ncch; i++) {
      if (mip->cch_tag[i] == tag)
        cidx = i;
    }
  }
    
  if (cidx >= CChans) {
    CommonWarning( "Unanticipated coupling channel\n");
    return -1;
  }

  /*  get ind_sw_cce flag */
  ind_sw_cc = GetBits(LEN_IND_SW_CCE,
                           IND_SW_CCE_FLAG,  
                           hResilience,
                           hEscInstanceData,
                           hVm );
  mip->cc_ind[cidx] = ind_sw_cc;
     
  /* coupled (target) elements */
  nele = GetBits(LEN_NCC,
                 NUM_COUPLED_ELEMENTS,  
                 hResilience,
                 hEscInstanceData,
                 hVm );
  nch = 0;
  for (i=0; i<nele+1; i++) {
    cpe = GetBits(LEN_IS_CPE,
                  CC_TARGET_IS_CPE,  
                  hResilience,
                  hEscInstanceData,
                  hVm );

    tag = GetBits(LEN_TAG,
                  CC_TARGET_TAG_SELECT,  
                  hResilience,
                  hEscInstanceData,
                  hVm );
    ch = ch_index(mip, cpe, tag);
    
    
    if (!cpe) {
      target[nch] = ch;
      
      shared[nch++] = 0;
    }
    else {
      cc_l = GetBits(LEN_CC_LR,
                     CC_L,  
                     hResilience,
                     hEscInstanceData,
                     hVm );

      cc_r = GetBits(LEN_CC_LR,
                     CC_R,  
                     hResilience,
                     hEscInstanceData,
                     hVm );

      j = (cc_r<<1) | cc_l; /* 0: l=0, r=0
                               1: l=1, r=0
                               2: l=0, r=1
                               3: l=1, r=1 */
      
     
      
      switch(j) {
      
      case 0:	    /* shared gain list */
        target[nch] = ch;
        target[nch+1] = mip->ch_info[ch].paired_ch;
        shared[nch] = 1;
        shared[nch+1] = 1;
        nch += 2;
        break;
      case 1:	    /* left channel gain list */
        target[nch] = ch;
        shared[nch] = 0;
        nch += 1;
        break;
      case 2:	    /* right channel gain list */
        target[nch] = mip->ch_info[ch].paired_ch;
        shared[nch] = 0;
        nch += 1;
        break;
      case 3:	    /* two gain lists */
        target[nch] = ch;
        target[nch+1] = mip->ch_info[ch].paired_ch;
        shared[nch] = 0;
        shared[nch+1] = 0;
        nch += 2;
        break;
      }
    }
  }
  
 
  
    cc_dom = GetBits(LEN_CC_DOM,
                  CC_DOMAIN,  
                   hResilience,
                   hEscInstanceData,
                   hVm );

  cc_gain_ele_sign = GetBits(LEN_CC_SGN,
                             GAIN_ELEMENT_SIGN,  
                             hResilience,
                             hEscInstanceData,
                             hVm );

  scl_idx = GetBits(LEN_CCH_GES,
                    GAIN_ELEMENT_SCALE,  
                    hResilience,
                    hEscInstanceData,
                    hVm );

  /*
   * coupling channel bitstream
   * (equivalent to SCE)
   */
  fltclr(cc_coef[cidx], LN2);
  /*memcpy(info, winmap[cc_wnd[cidx]], sizeof(Info));*/
  if (!getics ( hDec, 
                info, 
                0, /* common_window - always 0 while using CC */ 
                &cc_wnd[cidx],  
                &cc_wnd_shape[cidx].this_bk,
                cc_group[cidx],
                &cc_max_sfb[cidx],
                pred_type, /* pred_type */
                cc_lpflag[cidx],
                cc_prstflag[cidx],
                cb_map[cidx], /* cb_map */ 
                cc_coef[cidx], /* coef */ 
                -1, /*max_spec_coefficients*/ 
                &global_gain, /*global gain*/ 
                factors, /*factors*/ 
                nok_cc_ltp[cidx], /*nok_ltp*/ 
                &tns, /*tns*/ 
                gc_stream, /* gc_streamCh */ 
                bitStreamType, /*bitStreamType*/ 
                
                hResilience,
                hHcrSpecData,
                hHcrInfo,
                hEscInstanceData,
                hConcealment,
                qc_select,
                
                hVm, /* handle buffer */
                /* just useful in scalable layers */
                0,       /* extensionLayerFlag */
                0        /* er_channel */
#ifdef I2R_LOSSLESS
                , NULL
#endif
                ) )
  
    return -1;

      
  /* coupling for first target channel(s) is already at
   * correct scale
   */
  ch = shared[0] ? 2 : 1;
  for (j=0; j<ch; j++) 
    {
      for (i=0; i<MAXBANDS; i++)
        cc_gain[cidx][target[j]][i] = 1.0;
      target_multiple[target[j]]++; 
    }
  /*
   * bitstreams for target channel scale factors
   */
  for (; ch<nch;ch++ ) {
    /* if needed, get common gain element present */ 
    cgep = ind_sw_cc ? 1 : GetBits(LEN_CCH_CGP,
                                   COMMON_GAIN_ELEMENT_PRESENT,  
                                   hResilience,
                                   hEscInstanceData,
                                   hVm );
    if (cgep) {
      /* common gain */
      int t;
      Hcb *hcb;
      Huffman *hcw;

      /*  get just one scale factor */
      hcb = &book[BOOKSCL];
      hcw = hcb->hcw;
      t = decode_huff_cw(hcw, 
                         0, 
                         hResilience, 
                         hEscInstanceData,
                         hVm);
      fac = t - MIDFAC; 
      
      /* recover stepsize */
      scale = cc_gain_scale[scl_idx];
      scale = pow(scale, -fac);
      
      /* copy to gain array */
      n = shared[ch] ? 2 : 1;
      for (m=0; m<n; m++) 
        {
          k=0;
          for (i=0; i<info->nsbk; i++)
            
              for (j=0; j<info->sfb_per_sbk[i]; j++)
                cc_gain[cidx][target[ch]][k++] = scale;
          if(n==2)
            ch++;
          
                }
            }
    else {
      /* must be dependently switched cce */
      ind_sw_cc = 0;
	    
      /* get scale factors
       * use sectioning of coupling channel
       */
      
      cc_hufffac(info,
                 cc_group[cidx],
                 cb_map[cidx], 
#ifdef MPEG4V
                 global_gain,
#endif
                 factors,
                 dataFlag,
                 hResilience,
                 hEscInstanceData,
                 hVm);
      

      /* recover sign and stepsize */
      
      cc_scale = cc_gain_scale[scl_idx];
      k=0;
      for (i=0; i<info->nsbk; i++) {
        for (j=0; j<info->sfb_per_sbk[i]; j++) {
          fac = factors[k];
          if (cc_gain_ele_sign) {
            sign = 1-(fac & 1);
            fac >>= 1;
          }
          else {
            sign = 1;
          }
          scale = pow(cc_scale, -fac);
          scale *= (sign==0) ? -1 : 1;
          
          cc_gain[cidx][target[ch]][k++] = scale;
        }
      }  

      /* shared gain lists */
      target_multiple[target[ch]]++;
      if ( shared[ch] ) {
        for (i=0; i<MAXBANDS; i++)
          cc_gain[cidx][target[ch+1]][i] = 
            cc_gain[cidx][target[ch]][i];
        ch++;
        target_multiple[target[ch]]++;
      }
    }
  }
  
  /* process coupling channel the same as other channels, 
   * except that it can only be a SCE
   */

  if(mc_info.profile == LTP_Profile) 
    {
           
	    nok_lt_predict (
                      info, 
                      cc_wnd[cidx], 
                      cc_wnd_shape[cidx].this_bk,
                      cc_wnd_shape[cidx].prev_bk,
                      nok_cc_ltp[cidx]->sbk_prediction_used,
                      nok_cc_ltp[cidx]->sfb_prediction_used,
                      nok_cc_ltp[cidx], 
                      nok_cc_ltp[cidx]->weight,
                      nok_cc_ltp[cidx]->delay, 
                      cc_coef[cidx],
                      BLOCK_LEN_LONG, 
                      BLOCK_LEN_SHORT,
                      mip->sampling_rate_idx,
                      &tns,
                      qc_select);
      
    } 


  else {
    /* process coupling channel the same as other channels, 
     * except that it can only be a SCE
     */

    
    predict(info, 
            cc_lpflag[cidx], 
            cc_sp_status[cidx],
            hConcealment,
            cc_coef[cidx]);
    
    predict_reset(info, 
                  cc_prstflag[cidx], 
                  cc_sp_status, 
                  cidx, 
                  cidx,
		  last_cc_rstgrp_num);
  }

  for (i=j=0; i<tns.n_subblocks; i++) 
    {
           tns_decode_subblock( 
                          &cc_coef[cidx][j], 
                          cc_max_sfb[cidx],
                          info->sbk_sfb_top[i], 
                          info->islong, 
                          &(tns.info[i]), 
                          qc_select, 
                          samplFreqIdx);

      j += info->bins_per_sbk[i];
    }
  
  /* invert the cch to ch mapping */
  for (i=0; i<nch; i++) 
    {
      mip->ch_info[target[i]].ncch = cidx+1;
      mip->ch_info[target[i]].cch[cidx] = cidx;
      mip->ch_info[target[i]].cc_dom[cidx] = cc_dom;
      mip->ch_info[target[i]].cc_ind[cidx] = ind_sw_cc;
    }

  
  
  if (default_config)
   mip->ncch++;
 
 return 1;
}

#if (ICChans > 0)
/* transform independently switched coupling channels into
 * time domain
 */
void ind_coupling(MC_Info *mip, 
                  WINDOW_SEQUENCE *wnd, 
                  Wnd_Shape *wnd_shape,
                  WINDOW_SEQUENCE *cc_wnd, 
                  Wnd_Shape *cc_wnd_shape, 
                  double *cc_coef[CChans], 
                  double *cc_state[ICChans],
                  int blockSize
                  )
{
  int cidx;
  
  for (cidx=0; cidx<mip->ncch; cidx++) 
    {
      if (mip->cc_ind[cidx])  
        { 
          double data[LN2];
          
          freq2buffer(cc_coef[cidx],
                      data,
                      cc_state[cidx], 
                      cc_wnd[cidx], 
                      blockSize,
                      blockSize/8,
                      cc_wnd_shape[cidx].this_bk, 
                      cc_wnd_shape[cidx].prev_bk, 
                      OVERLAPPED_MODE,
                      8
                      );
          fltcpy(cc_coef[cidx], data, LN2);
        }
    }
}

#endif
	

static void mix_cc(Info *info,
		double *coef, 
		double *cc_coef, 
		double *cc_gain, 
		int ind,
		int win,
		byte* cb_map)
{
  
  int nsbk, sbk, sfb, nsfb, k, top;
  double scale;

  if (!ind) {
    /* frequency-domain coupling */
    k = 0;
    nsbk = info->nsbk;
    for (sbk=0; sbk<nsbk; sbk++) 
      {
        nsfb = info->sfb_per_sbk[sbk];
        for (sfb=0; sfb<nsfb; sfb++) 
          {
            top = info->bk_sfb_top[sbk*nsfb+sfb];
            scale = *cc_gain++;
            if (scale == 0 || (cb_map[sfb] == ZERO_HCB)) 
              {
                /* no coupling */
                k = top;
              }
            else {
              /* mix in coupling channel */
              for (; k<top;k++) 
                {
                  coef[k] += cc_coef[k] * scale;
                }
            }
          }
      }
  }
  else {
    /* time-domain coupling (coef[] is actually data[]!) */
           	  
    scale = *cc_gain;
    for (k=0; k<LN2; k++)
      {
	coef[k] += cc_coef[k] * scale;
      }
  }	
}
   
void coupling ( Info *info, 
                MC_Info *mip, 
                double **coef, 
                double **cc_coef, 
                double *cc_gain[CChans][Chans], 
                int ch, 
                int cc_dom, 
                WINDOW_SEQUENCE *cc_wnd,
                byte** cb_map,
                int cc_ind,
                int tl_channel)
{
  int i, j, cch, mix;
  int wn = 0;
  Ch_Info *cip = &mip->ch_info[ch];
  j=cip->ncch;
      
  for (i=0; i<j; i++) 
    {   
      mix = (cc_ind && cip->cc_ind[i]) || 
        (!cc_ind && !cc_dom && !cip->cc_ind[i] && !cip->cc_dom[i]) ||
        (!cc_ind && cc_dom && !cip->cc_ind[i] && cip->cc_dom[i]);
      
      if (mix) 
        {       
	  cch = cip->cch[i];
          		  
	  if (tl_channel > 0) /* independently switched coupling, mixing in time_domain */
	  {
		  mix_cc(info,coef[tl_channel], cc_coef[cch], cc_gain[cch][ch], /*ind*/ cc_ind, wn,cb_map[cch]);
	  }
	  else /* dependently switched coupling, mixing in frequency domain */
	  {
		  mix_cc(info,coef[ch],cc_coef[cch],cc_gain[cch][ch],/*ind*/ cc_ind,wn,cb_map[cch]);
	  }
        }
    }
}

#endif	/* (CChans > 0) */




