/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Heiko Purnhagen (Uni Hannover) on 1996-11-15,                           */
/*   Naoki Iwakami (NTT) on 1996-12-06,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-08-25,                                      */
/*   Naoki Iwakami (NTT) on 1997-10-26,                                      */
/*   Takehiro Moriya (NTT) on 1998-07-16, on 2000-8-11                       */
/*   Takeshi Mori (NTT) on 2000-11-21                                        */
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

/* 15-nov-96   HP   adapted to new bitstream module */
/* 06-dec-96   NI   provided 6 kbit/s support */
/* 18-apr-97   NI   merged long, medium, & short sequences into one sequence */
/* 25-aug-97   NI   bugfixes */
/* 16-jul-98   TM   AAC-TwinVQ convergence */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "bitstream.h"
#include "ntt_conf.h"  /* fixed by NTT 20001121 */
#include "all.h"
#include "obj_descr.h"
#include "aac.h"
#include "nok_lt_prediction.h"
#include "common_m4a.h"

int ntt_BitUnPack ( HANDLE_BSBITSTREAM   stream,
                    int           available_bits,
                    int           decoded_bits,
                    FRAME_DATA*    fd,
                    WINDOW_SEQUENCE   windowSequence,
                    ntt_INDEX*    index)
{
  /*--- Variables ---*/
  int		bitcount, itmp, isf, i_ch, idiv, iptop;
  /* frame */
  int nsf = 0;
  int n_ch;
  /* shape */
  int bits_side_info = 0;
  int  vq_bits, ndiv, bits0, bits1, bits;
  /* Bark-scale envelope */
  int fw_n_bit   = 0;
  int fw_ndiv    = 0;
  int flag_pitch = 0;
  
  int	bits1_p[ntt_N_DIV_P_MAX], length_p[ntt_N_DIV_P_MAX];
  int	bits0_p[ntt_N_DIV_P_MAX];
  int align_side = 0;
  unsigned int dummy;
  int pitch_bits=0, bandlim_bits=0;
  int epFlag=0;
  index->w_type = windowSequence;
  bitcount = 0;

  epFlag = fd->od->ESDescriptor[0]->DecConfigDescr.audioSpecificConfig.epConfig.value & 0;

  if((epFlag) && (index->er_tvq==1))
    vq_bits = index->nttDataBase->ntt_NBITS_FR - decoded_bits;
  else
    vq_bits = available_bits;

  /*--- Set parameters ---*/
  n_ch = index->numChannel;
  switch (index->w_type){
  case ONLY_LONG_SEQUENCE: 
  case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    /* frame */
    nsf = 1;
  /*--- pitch_q flag ---*/
    BsGetBitInt(stream, (unsigned int *)&(index->bandlimit), 1);
    bitcount += 1;
    BsGetBitInt(stream, (unsigned int *)&(index->pitch_q), 1);
    bitcount += 1;
    BsGetBitInt(stream, (unsigned int *)&(index->pf), 1);
    bitcount += 1;
    /* available bits */
    bits_side_info =  ntt_NMTOOL_BITSperCH* index->numChannel+3;  /*ppc,band,postp*/
    vq_bits  -= bits_side_info;
    /* added by NTT 20001121 */
    if(index->pitch_q == 1){
      vq_bits -= ntt_PIT_TBITperCH*index->numChannel;
      pitch_bits = ntt_PIT_TBITperCH*index->numChannel;
    }
    if(index->bandlimit == 1){
      vq_bits -= ntt_BWID_BITS*index->numChannel;
      bandlim_bits = ntt_BWID_BITS*index->numChannel;
    }
    index->nttDataBase->ntt_VQTOOL_BITS = vq_bits;
    /* Bark-scale envelope */
    fw_n_bit = ntt_FW_N_BIT;
    fw_ndiv = ntt_FW_N_DIV;
    /* flags */
    flag_pitch = index->pitch_q;
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* frame */
    nsf = ntt_N_SHRT;
    /* shape */
    BsGetBitInt(stream, (unsigned int *)&(index->bandlimit), 1);
    bitcount += 1;
    bits_side_info = ntt_NMTOOL_BITS_SperCH*index->numChannel+1;
    vq_bits -=  bits_side_info;
    /* added by NTT 20001121 */
    if(index->bandlimit == 1){
      vq_bits -= ntt_BWID_BITS*index->numChannel;
      bandlim_bits = ntt_BWID_BITS*index->numChannel;
    }
    index->nttDataBase->ntt_VQTOOL_BITS_S = vq_bits;
    fw_n_bit = 0;
    fw_ndiv = 0;
    /* flags */
    flag_pitch = 0;
    break;
  default:
    CommonExit ( 1, "window type not supported" );
  }

  /*the bits in between the escs are removed when merging them*/
  align_side=0;



  /* set bits for pitch */
  if(flag_pitch){
    ntt_vec_lenp( index->numChannel, bits1_p, length_p );
    for ( idiv=0; idiv<ntt_N_DIV_PperCH*index->numChannel; idiv++ ){
      bits0_p[idiv] = (bits1_p[idiv]+1)/2;
      bits1_p[idiv] /= 2;
    }
  }
    for (i_ch=0; i_ch<n_ch; i_ch++){
    if(index->bandlimit == 1){
      BsGetBitInt(stream, (unsigned int *)&(index->blim_h[i_ch]),
		  ntt_BLIM_BITS_H);
      bitcount += ntt_BLIM_BITS_H;
      BsGetBitInt(stream, (unsigned int *)&(index->blim_l[i_ch]),
		  ntt_BLIM_BITS_L);
      bitcount += ntt_BLIM_BITS_L;
    }
  }
  
  if (flag_pitch){
      for ( idiv=0; idiv<ntt_N_DIV_PperCH*index->numChannel; idiv++ ){
	BsGetBitInt(stream, (unsigned int *)&(index->pls[idiv]),
		    bits0_p[idiv]);       /*CB0*/
	BsGetBitInt(stream, 
	   (unsigned int *)&(index->pls[idiv+ntt_N_DIV_PperCH*index->numChannel]),
		    bits1_p[idiv]);       /*CB1*/
	bitcount += bits0_p[idiv] + bits1_p[idiv];
      }
      for (i_ch=0; i_ch<n_ch; i_ch++){
	BsGetBitInt(stream, (unsigned int *)&(index->pit[i_ch]), ntt_BASF_BIT);
	BsGetBitInt(stream, (unsigned int *)&(index->pgain[i_ch]),
		    ntt_PGAIN_BIT);
	bitcount += ntt_BASF_BIT + ntt_PGAIN_BIT;
      }
  }
if(index->er_tvq==1){ /* Object type =21 */
  /*--- Gain ---*/
  if (nsf == 1){
    for (i_ch=0; i_ch<n_ch; i_ch++){
      BsGetBitInt(stream, (unsigned int *)&(index->pow[i_ch]), ntt_GAIN_BITS);
      bitcount += ntt_GAIN_BITS;
    }
  }
  else{
    for (i_ch=0; i_ch<n_ch; i_ch++){
      iptop = (nsf+1)*i_ch;
      BsGetBitInt(stream, (unsigned int *)&(index->pow[iptop]), ntt_GAIN_BITS);
      for ( isf=0; isf<nsf; isf++ ){
	BsGetBitInt(stream, (unsigned int *)&(index->pow[iptop+isf+1]),
		    ntt_SUB_GAIN_BITS);
      }
      bitcount += ntt_GAIN_BITS + ntt_SUB_GAIN_BITS * nsf;
    }
  }
  /*--- LSP ---*/
  for (i_ch=0; i_ch<n_ch; i_ch++){
    /* pred. switch */
    BsGetBitInt(stream, (unsigned int *)&(index->lsp[i_ch][0]), ntt_LSP_BIT0);
    /* first stage */
    BsGetBitInt(stream, (unsigned int *)&(index->lsp[i_ch][1]), ntt_LSP_BIT1);
    /* second stage */
    for (itmp=0; itmp<ntt_LSP_SPLIT; itmp++)
      BsGetBitInt(stream, (unsigned int *)&(index->lsp[i_ch][itmp+2]),
		  ntt_LSP_BIT2);
    bitcount += ntt_LSP_BIT0 + ntt_LSP_BIT1 + ntt_LSP_SPLIT * ntt_LSP_BIT2;
  }
  
  /*--- Forward envelope code ---*/
  if (fw_n_bit > 0){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	for ( idiv=0; idiv<fw_ndiv; idiv++ ){
	  itmp = idiv+(isf+i_ch*nsf)*fw_ndiv;
	  BsGetBitInt(stream, (unsigned int *)&(index->fw[itmp]), fw_n_bit);
	  bitcount += fw_n_bit;
	}
      }
    }
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	BsGetBitInt(stream, (unsigned int *)&(index->fw_alf[i_ch*nsf+isf]),
		    ntt_FW_ARQ_NBIT);
	bitcount += ntt_FW_ARQ_NBIT;
      }
    }
  }
  
  if(epFlag){
      BsGetBitInt(stream, &dummy, align_side);
  }

  /*--- Shape vector code ---*/
  ndiv = (int)((vq_bits+ntt_MAXBIT*2-1)/(ntt_MAXBIT*2));
  for ( idiv=0; idiv<ndiv; idiv++ ){
    /* set bits */
    bits = (vq_bits+ndiv-1-idiv)/ndiv;
    bits0 = (bits+1)/2;
    bits1 = bits/2;
    /* read stream */
    BsGetBitInt(stream, (unsigned int *)&(index->wvq[idiv]), bits0);
    BsGetBitInt(stream, (unsigned int *)&(index->wvq[idiv+ndiv]), bits1);
    bitcount += bits0 + bits1;
  }

}
else{ /* object type = 7 */
  /*--- Shape vector code ---*/
  ndiv = (int)((vq_bits+ntt_MAXBIT*2-1)/(ntt_MAXBIT*2));
  for ( idiv=0; idiv<ndiv; idiv++ ){
    /* set bits */
    bits = (vq_bits+ndiv-1-idiv)/ndiv;
    bits0 = (bits+1)/2;
    bits1 = bits/2;
    /* read stream */
    BsGetBitInt(stream, (unsigned int *)&(index->wvq[idiv]), bits0);
    BsGetBitInt(stream, (unsigned int *)&(index->wvq[idiv+ndiv]), bits1);
    bitcount += bits0 + bits1;
  }
  
  /*--- Forward envelope code ---*/
  if (fw_n_bit > 0){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	for ( idiv=0; idiv<fw_ndiv; idiv++ ){
	  itmp = idiv+(isf+i_ch*nsf)*fw_ndiv;
	  BsGetBitInt(stream, (unsigned int *)&(index->fw[itmp]), fw_n_bit);
	  bitcount += fw_n_bit;
	}
      }
    }
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	BsGetBitInt(stream, (unsigned int *)&(index->fw_alf[i_ch*nsf+isf]),
		    ntt_FW_ARQ_NBIT);
	bitcount += ntt_FW_ARQ_NBIT;
      }
    }
  }
  
  /*--- Gain ---*/
  if (nsf == 1){
    for (i_ch=0; i_ch<n_ch; i_ch++){
      BsGetBitInt(stream, (unsigned int *)&(index->pow[i_ch]), ntt_GAIN_BITS);
      bitcount += ntt_GAIN_BITS;
    }
  }
  else{
    for (i_ch=0; i_ch<n_ch; i_ch++){
      iptop = (nsf+1)*i_ch;
      BsGetBitInt(stream, (unsigned int *)&(index->pow[iptop]), ntt_GAIN_BITS);
      for ( isf=0; isf<nsf; isf++ ){
	BsGetBitInt(stream, (unsigned int *)&(index->pow[iptop+isf+1]),
		    ntt_SUB_GAIN_BITS);
      }
      bitcount += ntt_GAIN_BITS + ntt_SUB_GAIN_BITS * nsf;
    }
  }
  /*--- LSP ---*/
  for (i_ch=0; i_ch<n_ch; i_ch++){
    /* pred. switch */
    BsGetBitInt(stream, (unsigned int *)&(index->lsp[i_ch][0]), ntt_LSP_BIT0);
    /* first stage */
    BsGetBitInt(stream, (unsigned int *)&(index->lsp[i_ch][1]), ntt_LSP_BIT1);
    /* second stage */
    for (itmp=0; itmp<ntt_LSP_SPLIT; itmp++)
      BsGetBitInt(stream, (unsigned int *)&(index->lsp[i_ch][itmp+2]),
		  ntt_LSP_BIT2);
    bitcount += ntt_LSP_BIT0 + ntt_LSP_BIT1 + ntt_LSP_SPLIT * ntt_LSP_BIT2;
  }
}
  /*--- Pitch excitation ---*/
  return(bitcount);
  
}
