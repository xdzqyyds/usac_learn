/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Naoki Iwakami (NTT)                                                     */
/* and edited by                                                             */
/*   Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     */
/*   Naoki Iwakami (NTT) on 1996-08-27,                                      */
/*   Naoki Iwakami (NTT) on 1996-12-06,                                      */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Naoki Iwakami (NTT) on 1997-08-25                                       */
/*   Takehiro Moriya (NTT) on 1998-07-16                                     */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-23                           */
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

/* 06-dec-96   NI   provided 6 kbit/s support */
/* 18-apr-97   NI   merged long, medium, & short sequences into one sequence */
/* 25-aug-97   NI   added bandwidth control */

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "bitstream.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
#include "all.h"
#include "nok_ltp_enc.h"



static int tvq_BitPack_baselayerinfo(ntt_INDEX *index,
		int n_ch,
		int flag_pitch,
		HANDLE_BSBITSTREAM stream)
{
  int i_ch;
  int idiv;
  int	bits1_p[ntt_N_DIV_P_MAX], length_p[ntt_N_DIV_P_MAX];
  int	bits0_p[ntt_N_DIV_P_MAX];
  int bitcount=0;

  BsPutBit(stream, (unsigned int)index->bandlimit, 1);
  bitcount += 1;

  switch (index->w_type){
  case ONLY_LONG_SEQUENCE: 
  case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    /* frame */
    BsPutBit(stream, (unsigned int)index->pitch_q, 1);
    bitcount += 1;
    /*--- Postprocess ---*/
    BsPutBit(stream, (unsigned int)index->pf, 1);
    bitcount += 1;
    break;
  default:
    break;
  }

  if(index->bandlimit == 1){
    for (i_ch=0; i_ch<n_ch; i_ch++){
      if (ntt_BLIM_BITS_H > 0){
        BsPutBit(stream, (unsigned int)index->blim_h[i_ch], ntt_BLIM_BITS_H);
        bitcount += ntt_BLIM_BITS_H;
      }
      if (ntt_BLIM_BITS_L > 0){
        BsPutBit(stream, (unsigned int)index->blim_l[i_ch], ntt_BLIM_BITS_L);
        bitcount += ntt_BLIM_BITS_L;
      }
    }
  }
  
  /*--- Pitch excitation ---*/
  /* set bits for pitch */
  if(index->pitch_q == 1){
    ntt_vec_lenp( index->numChannel, bits1_p, length_p );
    for ( idiv=0; idiv<ntt_N_DIV_PperCH*n_ch; idiv++ ){
      bits0_p[idiv] = (bits1_p[idiv]+1)/2;
      bits1_p[idiv] /= 2;
    }
  }
 
  if (flag_pitch){
    for ( idiv=0; idiv<ntt_N_DIV_PperCH*n_ch; idiv++ ){
      BsPutBit(stream, (unsigned int)index->pls[idiv], bits0_p[idiv]);/*CB0*/
      BsPutBit(stream, (unsigned int)index->pls[idiv+ntt_N_DIV_PperCH*n_ch],
	      bits1_p[idiv]);/*CB1*/
      bitcount += bits0_p[idiv] + bits1_p[idiv];
    }
    for (i_ch=0; i_ch<n_ch; i_ch++){
      BsPutBit(stream, (unsigned int)index->pit[i_ch], ntt_BASF_BIT);
      BsPutBit(stream, (unsigned int)index->pgain[i_ch], ntt_PGAIN_BIT);
      bitcount += ntt_BASF_BIT + ntt_PGAIN_BIT;
    }
  }
  return bitcount;
}


static int tvq_BitPack_extlayerinfo(int n_ch,
		int mat_shift[ntt_N_SUP_MAX],
		HANDLE_BSBITSTREAM stream)
{
  int i_ch;
  int bitcount = 0;
  for(i_ch=0; i_ch<n_ch; i_ch++){
    BsPutBit(stream,mat_shift[i_ch],mat_SHIFT_BIT);
    bitcount += mat_SHIFT_BIT;
  }
  return bitcount;
}



static int tvq_BitPack_gain(ntt_INDEX *index,
		int n_ch,
		int nsf,
		int iscl,
		HANDLE_BSBITSTREAM stream)
{
  int bitcount = 0;
  int i_ch;
  int iptop;
  int isf;
  int gain_bits, sub_gain_bits;
  if (iscl < 0) {
    gain_bits = ntt_GAIN_BITS;
    sub_gain_bits = ntt_SUB_GAIN_BITS;
  } else {
    gain_bits = ntt_GAIN_BITS_SCL;
    sub_gain_bits = ntt_SUB_GAIN_BITS_SCL;
  }
  /*--- Gain ---*/
  if (nsf == 1){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      BsPutBit(stream, (unsigned int)index->pow[i_ch], gain_bits);
      bitcount += gain_bits;
    }
  } else {
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      iptop = (nsf+1)*i_ch;
      BsPutBit(stream, (unsigned int)index->pow[iptop], gain_bits);
      for ( isf=0; isf<nsf; isf++ ){
	BsPutBit(stream, (unsigned int)index->pow[iptop+isf+1],
		 sub_gain_bits);
      }
      bitcount += gain_bits + sub_gain_bits * nsf;
    }
  }
  return bitcount;
}


static int tvq_BitPack_lsp(ntt_INDEX *index,
		int n_ch,
		int iscl,
		HANDLE_BSBITSTREAM stream)
{
  int i_ch;
  int itmp;
  int bitcount = 0;
  int lsp_bit0, lsp_bit1, lsp_bit2, lsp_split;
  if (iscl<0) {
    lsp_bit0 = ntt_LSP_BIT0;
    lsp_bit1 = ntt_LSP_BIT1;
    lsp_bit2 = ntt_LSP_BIT2;
    lsp_split = ntt_LSP_SPLIT;
  } else {
    lsp_bit0 = ntt_LSP_BIT0_SCL;
    lsp_bit1 = ntt_LSP_BIT1_SCL;
    lsp_bit2 = ntt_LSP_BIT2_SCL;
    lsp_split = ntt_LSP_SPLIT_SCL;
  }
  /*--- LSP ---*/
  for ( i_ch=0; i_ch<n_ch; i_ch++ ){
    /* pred. switch */
    BsPutBit(stream, (unsigned int)index->lsp[i_ch][0], lsp_bit0);
    /* first stage */
    BsPutBit(stream, (unsigned int)index->lsp[i_ch][1], lsp_bit1);
    /* second stage */
    for ( itmp=0; itmp<lsp_split; itmp++ )
      BsPutBit(stream, (unsigned int)index->lsp[i_ch][itmp+2], lsp_bit2);
    bitcount += lsp_bit0 + lsp_bit1 + lsp_bit2 * lsp_split;
  }
  return bitcount;
}


static int tvq_BitPack_forward_env_code(ntt_INDEX *index,
		int n_ch,
		int fw_n_bit,
		int nsf,
		int fw_ndiv,
		HANDLE_BSBITSTREAM stream)
{
  int i_ch;
  int idiv;
  int isf;
  int itmp;
  int bitcount = 0;
  /*--- Forward envelope code ---*/
  if (fw_n_bit > 0){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	for ( idiv=0; idiv<fw_ndiv; idiv++ ){
	  itmp = idiv+(isf+i_ch*nsf)*fw_ndiv;
	  BsPutBit(stream, (unsigned int)index->fw[itmp], fw_n_bit);
	  bitcount += fw_n_bit;
	}
      }
    }
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	BsPutBit(stream, (unsigned int)index->fw_alf[i_ch*nsf+isf],
		 ntt_FW_ARQ_NBIT);
	bitcount += ntt_FW_ARQ_NBIT;
      }
    }
  }
  return bitcount;
}


static int tvq_BitPack_shape_vector_code(ntt_INDEX *index,
		int vq_bits,
		HANDLE_BSBITSTREAM stream)
{
  int ndiv;
  int idiv;
  int bits, bits0, bits1;
  int bitcount = 0;
  /*--- Shape vector code ---*/
  ndiv = (int)((vq_bits+ntt_MAXBIT*2-1)/(ntt_MAXBIT*2));
  for ( idiv=0; idiv<ndiv; idiv++ ){
    /* set bits */
    bits = (vq_bits+ndiv-1-idiv)/ndiv;
    bits0 = (bits+1)/2;
    bits1 = bits/2;
    /* read stream */
    BsPutBit(stream, (unsigned int)index->wvq[idiv], bits0);
    BsPutBit(stream, (unsigned int)index->wvq[idiv+ndiv], bits1);
    bitcount += bits0 + bits1;
  }
  return bitcount;
}


static void tvq_BitPack_setup_param(ntt_INDEX *index, int *n_ch, int *nsf,
			int *vq_bits, int *fw_n_bit, int *fw_ndiv,
			int *flag_pitch, int iscl)
{
  *n_ch = index->numChannel;
  switch (index->w_type){
  case ONLY_LONG_SEQUENCE: 
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE:
    /* frame */
    *nsf = 1;
    if (iscl<0) {
      /* available bits */
      *vq_bits = index->nttDataBase->ntt_VQTOOL_BITS;
      /* Bark-scale envelope */
      *fw_n_bit = ntt_FW_N_BIT;
      *fw_ndiv = ntt_FW_N_DIV;
    } else {
      *vq_bits = index->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl];
      *fw_n_bit = ntt_FW_N_BIT_SCL;
      *fw_ndiv = ntt_FW_N_DIV_SCL;
    }
    *flag_pitch = index->pitch_q;
    /* flags */
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* frame */
    *nsf = ntt_N_SHRT;
    if (iscl<0) {
      /* shape */
      *vq_bits = index->nttDataBase->ntt_VQTOOL_BITS_S;
      /* Bark-scale envelope */
      *fw_n_bit = 0;
      *fw_ndiv = 0;
    } else {
      *vq_bits = index->nttDataScl->ntt_VQTOOL_BITS_S_SCL[iscl];
      *fw_n_bit = 0;
      *fw_ndiv = 0;
    }
    /* flags */
    *flag_pitch = 0;
    break;
  default:
    break; 
  }
}




int ntt_BitPack(ntt_INDEX   *index,
		HANDLE_BSBITSTREAM *stream_for_esc,
		int mat_shift[ntt_N_SUP_MAX],
		int iscl)
{
  /*--- Variables ---*/
  /* frame */
  int nsf, n_ch;
  /* shape */
  int  vq_bits;
  /* Bark-scale envelope */
  int fw_n_bit, fw_ndiv;
  int flag_pitch;

  int	bitcount;
  
  nsf=fw_n_bit=fw_ndiv= flag_pitch= vq_bits= 0; 

  /*--- Set parameters ---*/
  tvq_BitPack_setup_param(index, &n_ch, &nsf,
			&vq_bits, &fw_n_bit, &fw_ndiv,
			&flag_pitch, iscl);

  bitcount = 0;

  if (iscl<0)
    bitcount += tvq_BitPack_baselayerinfo(index, n_ch, flag_pitch,
			stream_for_esc[0]);
  else
    bitcount += tvq_BitPack_extlayerinfo(n_ch, mat_shift,
			stream_for_esc[0]);

  if(index->er_tvq ==1){/* objecttype =21 */
    /*--- GAIN ---*/
    bitcount += tvq_BitPack_gain(index, n_ch, nsf, iscl, stream_for_esc[0]);

    /*--- LSP ---*/
    bitcount += tvq_BitPack_lsp(index, n_ch, iscl, stream_for_esc[0]);

    /*--- Forward envelope code ---*/
    bitcount += tvq_BitPack_forward_env_code(index, n_ch, fw_n_bit, nsf,
			fw_ndiv, stream_for_esc[0]);

    /*--- Shape vector code ---*/
    bitcount += tvq_BitPack_shape_vector_code(index, vq_bits,
			stream_for_esc[1]);

  } else {/*objecttype=7*/
    /*--- Shape vector code ---*/
    bitcount += tvq_BitPack_shape_vector_code(index, vq_bits,
			stream_for_esc[0]);

    /*--- Forward envelope code ---*/
    bitcount += tvq_BitPack_forward_env_code(index, n_ch, fw_n_bit, nsf,
			fw_ndiv, stream_for_esc[0]);

    /*--- Gain ---*/
    bitcount += tvq_BitPack_gain(index, n_ch, nsf, iscl, stream_for_esc[0]);

    /*--- LSP ---*/
    bitcount += tvq_BitPack_lsp(index, n_ch, iscl, stream_for_esc[0]);
  }

  return(bitcount);
}
