/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT),                                                         */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18                                       */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
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

#include "bitstream.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"
//#include "mat_def_ntt.h"

int ntt_SclBitPack(ntt_INDEX *index_scl,
		   HANDLE_BSBITSTREAM stream,
		   int         iscl)
{
  /*--- Variables ---*/
  int	bitcount, itmp, isf, i_ch, idiv, iptop;
  /* frame */
  int nsf = 0;
  int n_ch;
  /* shape */
  int vq_bits = 0;
  int ndiv, bits0, bits1, bits;
  /* Bark-scale envelope */
  int fw_n_bit = 0;
  int fw_ndiv = 0;

  bitcount = 0;

  /*--- Set parameters ---*/
  n_ch = index_scl->numChannel;
  switch (index_scl->w_type){
  case ONLY_LONG_SEQUENCE: 
  case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    /* frame */
    nsf = 1;
    /* available bits */
    vq_bits = index_scl->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl];
    /* Bark-scale envelope */
    fw_n_bit = ntt_FW_N_BIT_SCL;
    fw_ndiv = ntt_FW_N_DIV_SCL;
    /* flags */
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* frame */
    nsf = ntt_N_SHRT;
    /* shape */
    vq_bits = index_scl->nttDataScl->ntt_VQTOOL_BITS_S_SCL[iscl];
    /* Bark-scale envelope */
    fw_n_bit = 0;
    fw_ndiv =  0;
    /* flags */
    break;
 default:
    fprintf( stderr,"Fatal error! %d: no such window type.\n", index_scl->w_type );
        exit(1);
  }


  if(iscl >= 0){
    for(i_ch=0; i_ch<n_ch; i_ch++){
     BsPutBit(stream,mat_shift[iscl][i_ch],mat_SHIFT_BIT);
     bitcount += mat_SHIFT_BIT;
    }
   }
if(index_scl->er_tvq==1){ /*objecttype=21*/

  /*--- Gain ---*/
  if (nsf == 1){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      BsPutBit(stream, (unsigned int)index_scl->pow[i_ch],
	       ntt_GAIN_BITS_SCL);
      bitcount += ntt_GAIN_BITS_SCL;
    }
  }
  else{
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      iptop = (nsf+1)*i_ch;
      BsPutBit(stream, (unsigned int)index_scl->pow[iptop],
	       ntt_GAIN_BITS_SCL);
      for ( isf=0; isf<nsf; isf++ ){
	BsPutBit(stream, (unsigned int)index_scl->pow[iptop+isf+1],
		 ntt_SUB_GAIN_BITS_SCL);
      }
      bitcount += ntt_GAIN_BITS_SCL + ntt_SUB_GAIN_BITS_SCL * nsf;
    }
  }
  /*--- LSP ---*/
  for ( i_ch=0; i_ch<n_ch; i_ch++ ){
    /* pred. switch */
    BsPutBit(stream, (unsigned int)index_scl->lsp[i_ch][0],
	     ntt_LSP_BIT0_SCL);
    /* first stage */
    BsPutBit(stream, (unsigned int)index_scl->lsp[i_ch][1],
	     ntt_LSP_BIT1_SCL);
    /* second stage */
    for ( itmp=0; itmp<ntt_LSP_SPLIT_SCL; itmp++ )
      BsPutBit(stream, (unsigned int)index_scl->lsp[i_ch][itmp+2],
	       ntt_LSP_BIT2_SCL);
    bitcount += 
      ntt_LSP_BIT0_SCL + ntt_LSP_BIT1_SCL 
	+ ntt_LSP_BIT2_SCL * ntt_LSP_SPLIT_SCL;
  }
  
  /*--- Forward envelope code ---*/
  if (fw_n_bit > 0){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	for ( idiv=0; idiv<fw_ndiv; idiv++ ){
	  itmp = idiv+(isf+i_ch*nsf)*fw_ndiv;
	  BsPutBit(stream, (unsigned int)index_scl->fw[itmp], fw_n_bit);
	  bitcount += fw_n_bit;
	}
      }
    }
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	BsPutBit(stream, (unsigned int)index_scl->fw_alf[i_ch*nsf+isf],
		 ntt_FW_ARQ_NBIT);
	bitcount += ntt_FW_ARQ_NBIT;
      }
    }
  }
  /*--- Shape vector code ---*/
  ndiv = (int)((vq_bits+ntt_MAXBIT*2-1)/(ntt_MAXBIT*2));
  for ( idiv=0; idiv<ndiv; idiv++ ){
    /* set bits */
    bits = (vq_bits+ndiv-1-idiv)/ndiv;
    bits0 = (bits+1)/2;
    bits1 = bits/2;
    /* read stream */
    BsPutBit(stream, (unsigned int)index_scl->wvq[idiv], bits0);
    BsPutBit(stream, (unsigned int)index_scl->wvq[idiv+ndiv], bits1);
    bitcount += bits0 + bits1;
  }
}
else{ /*objecttype=7*/
  /*--- Shape vector code ---*/
  ndiv = (int)((vq_bits+ntt_MAXBIT*2-1)/(ntt_MAXBIT*2));
  for ( idiv=0; idiv<ndiv; idiv++ ){
    /* set bits */
    bits = (vq_bits+ndiv-1-idiv)/ndiv;
    bits0 = (bits+1)/2;
    bits1 = bits/2;
    /* read stream */
    BsPutBit(stream, (unsigned int)index_scl->wvq[idiv], bits0);
    BsPutBit(stream, (unsigned int)index_scl->wvq[idiv+ndiv], bits1);
    bitcount += bits0 + bits1;
  }
  /*--- Forward envelope code ---*/
  if (fw_n_bit > 0){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	for ( idiv=0; idiv<fw_ndiv; idiv++ ){
	  itmp = idiv+(isf+i_ch*nsf)*fw_ndiv;
	  BsPutBit(stream, (unsigned int)index_scl->fw[itmp], fw_n_bit);
	  bitcount += fw_n_bit;
	}
      }
    }
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	BsPutBit(stream, (unsigned int)index_scl->fw_alf[i_ch*nsf+isf],
		 ntt_FW_ARQ_NBIT);
	bitcount += ntt_FW_ARQ_NBIT;
      }
    }
  }
  /*--- Gain ---*/
  if (nsf == 1){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      BsPutBit(stream, (unsigned int)index_scl->pow[i_ch],
	       ntt_GAIN_BITS_SCL);
      bitcount += ntt_GAIN_BITS_SCL;
    }
  }
  else{
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      iptop = (nsf+1)*i_ch;
      BsPutBit(stream, (unsigned int)index_scl->pow[iptop],
	       ntt_GAIN_BITS_SCL);
      for ( isf=0; isf<nsf; isf++ ){
	BsPutBit(stream, (unsigned int)index_scl->pow[iptop+isf+1],
		 ntt_SUB_GAIN_BITS_SCL);
      }
      bitcount += ntt_GAIN_BITS_SCL + ntt_SUB_GAIN_BITS_SCL * nsf;
    }
  }
  /*--- LSP ---*/
  for ( i_ch=0; i_ch<n_ch; i_ch++ ){
    /* pred. switch */
    BsPutBit(stream, (unsigned int)index_scl->lsp[i_ch][0],
	     ntt_LSP_BIT0_SCL);
    /* first stage */
    BsPutBit(stream, (unsigned int)index_scl->lsp[i_ch][1],
	     ntt_LSP_BIT1_SCL);
    /* second stage */
    for ( itmp=0; itmp<ntt_LSP_SPLIT_SCL; itmp++ )
      BsPutBit(stream, (unsigned int)index_scl->lsp[i_ch][itmp+2],
	       ntt_LSP_BIT2_SCL);
    bitcount += 
      ntt_LSP_BIT0_SCL + ntt_LSP_BIT1_SCL 
	+ ntt_LSP_BIT2_SCL * ntt_LSP_SPLIT_SCL;
  }
}
  
  return(bitcount);

}
