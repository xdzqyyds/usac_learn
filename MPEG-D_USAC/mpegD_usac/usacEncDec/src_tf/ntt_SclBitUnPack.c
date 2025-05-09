/*****************************************************************************/
/* This software module was originally developed by                          */
/*   Akio Jin (NTT),                                                         */
/*   Takeshi Norimatsu,                                                      */
/*   Mineo Tsushima,                                                         */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/* and edited by                                                             */
/*   Naoki Iwakami (NTT) on 1997-04-18,                                      */
/*   Akio Jin (NTT),                                                         */
/*   Mineo Tsushima, (Matsushita Electric Industrial Co Ltd.)                */
/*   and Tomokazu Ishikawa (Matsushita Electric Industrial Co Ltd.)          */
/*   on 1997-10-23,                                                          */
/*   Takehiro Moriya (NTT) on 2000-8-11,                                     */
/*   Takeshi Mori (NTT) on 2000-11-21,                                       */
/*   Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-24                           */
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
#include "common_m4a.h"


int ntt_SclBitUnPack(HANDLE_BSBITSTREAM stream,
		     ntt_INDEX   *index_scl,
		     int         mat_shift[],
         int         decoded_bits,
         FRAME_DATA* fd,
		     int         *nextLayer,
		     int         bits_avail_fr,
		     int         iscl)
{
  /*--- Variables ---*/
  int		bitcount, itmp, isf, i_ch, idiv, iptop;
  /* frame */
  int nsf = 0;
  int n_ch;
  /* shape */
  int bits_side_info = 0;
  int vq_bits   = 0;
  int ndiv, bits0, bits1, bits;
  /* Bark-scale envelope */
  int fw_n_bit  = 0;
  int fw_ndiv   = 0;
  int align_side = 0;
  unsigned int dummy;
  int epFlag=0;

  bitcount = 0;

  epFlag = fd->od->ESDescriptor[*nextLayer]->DecConfigDescr.audioSpecificConfig.epConfig.value & 1;

  /*--- Set parameters ---*/
  n_ch = index_scl->numChannel;
  switch (index_scl->w_type){
  case ONLY_LONG_SEQUENCE: 
  case LONG_START_SEQUENCE: case LONG_STOP_SEQUENCE:
    /* frame */
    nsf = 1;
    /* available bits */
    bits_side_info = ntt_NMTOOL_BITS_SCLperCH*index_scl->numChannel;
 
    if((epFlag) && (index_scl->er_tvq==1))
      vq_bits = index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl] - decoded_bits - bits_side_info;
    else
      vq_bits = bits_avail_fr - bits_side_info;

    index_scl->nttDataScl->ntt_VQTOOL_BITS_SCL[iscl] = vq_bits;
    /* Bark-scale envelope */
    fw_n_bit = ntt_FW_N_BIT_SCL;
    fw_ndiv = ntt_FW_N_DIV_SCL;
    /* flags */
    break;
  case EIGHT_SHORT_SEQUENCE:
    /* frame */
    nsf = ntt_N_SHRT;
    /* shape */
    bits_side_info =  ntt_NMTOOL_BITS_S_SCLperCH * index_scl->numChannel;
                                 /*--- bug fixed A.Jin 1997.10.21 ---*/
    if((epFlag) && (index_scl->er_tvq==1))
      vq_bits = index_scl->nttDataScl->ntt_NBITS_FR_SCL[iscl] - decoded_bits - bits_side_info;
    else
      vq_bits = bits_avail_fr - bits_side_info;

    index_scl->nttDataScl->ntt_VQTOOL_BITS_S_SCL[iscl] = vq_bits;
    /* Bark-scale envelope */
    fw_n_bit = 0;
    fw_ndiv = 0;
    /* flags */
    break;
  default:
    CommonExit ( 1, "window type not supported" );
  }

  align_side = 0;

  /*--- Postfilter ---*/

  /* Tomokazu Ishikawa */
  if(iscl>=0){
    for(i_ch=0; i_ch<index_scl->numChannel; i_ch++){
      BsGetBitInt(stream, (unsigned int *)&(mat_shift[i_ch]),mat_SHIFT_BIT);
      bitcount += mat_SHIFT_BIT;
      /*
      printf("mat_shift[%d][%d] %d\n",iscl,i_ch, mat_shift[iscl][i_ch]);
      */
    }
  }
if(index_scl->er_tvq==1){
  /*--- Gain ---*/
  if (nsf == 1){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      BsGetBitInt(stream, (unsigned int *)&(index_scl->pow[i_ch]),
		  ntt_GAIN_BITS_SCL);
      bitcount += ntt_GAIN_BITS_SCL;
    }
  }
  else{
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      iptop = (nsf+1)*i_ch;
      BsGetBitInt(stream, (unsigned int *)&(index_scl->pow[iptop]),
		  ntt_GAIN_BITS_SCL);
      bitcount += ntt_GAIN_BITS_SCL;
      for ( isf=0; isf<nsf; isf++ ){
	BsGetBitInt(stream, (unsigned int *)&(index_scl->pow[iptop+isf+1]),
		    ntt_SUB_GAIN_BITS_SCL);
	bitcount += ntt_SUB_GAIN_BITS_SCL;
      }
    }
  }
  /*--- LSP ---*/
  for ( i_ch=0; i_ch<n_ch; i_ch++ ){
    /* pred. switch */
    BsGetBitInt(stream, (unsigned int *)&(index_scl->lsp[i_ch][0]),
		ntt_LSP_BIT0_SCL);
    bitcount += ntt_LSP_BIT0_SCL;
    /* first stage */
    BsGetBitInt(stream, (unsigned int *)&(index_scl->lsp[i_ch][1]),
		ntt_LSP_BIT1_SCL);
    bitcount += ntt_LSP_BIT1_SCL;
    /* second stage */
    for ( itmp=0; itmp<ntt_LSP_SPLIT_SCL; itmp++ ){
      BsGetBitInt(stream, (unsigned int *)&(index_scl->lsp[i_ch][itmp+2]),
		  ntt_LSP_BIT2_SCL);
      bitcount += ntt_LSP_BIT2_SCL;
    }
  }
  /*--- Forward envelope code ---*/
  if (fw_n_bit > 0){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	for ( idiv=0; idiv<fw_ndiv; idiv++ ){
	  itmp = idiv+(isf+i_ch*nsf)*fw_ndiv;
	  BsGetBitInt(stream, (unsigned int *)&(index_scl->fw[itmp]), fw_n_bit);
	  bitcount += fw_n_bit;
	}
      }
    }
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	BsGetBitInt(stream, (unsigned int *)&(index_scl->fw_alf[i_ch*nsf+isf]),
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
    BsGetBitInt(stream, (unsigned int *)&(index_scl->wvq[idiv]), bits0);
    BsGetBitInt(stream, (unsigned int *)&(index_scl->wvq[idiv+ndiv]), bits1);
    bitcount += bits0 + bits1;
  }
}
else{
  /*--- Shape vector code ---*/
  ndiv = (int)((vq_bits+ntt_MAXBIT*2-1)/(ntt_MAXBIT*2));
  for ( idiv=0; idiv<ndiv; idiv++ ){
    /* set bits */
    bits = (vq_bits+ndiv-1-idiv)/ndiv;
    bits0 = (bits+1)/2;
    bits1 = bits/2;
    /* read stream */
    BsGetBitInt(stream, (unsigned int *)&(index_scl->wvq[idiv]), bits0);
    BsGetBitInt(stream, (unsigned int *)&(index_scl->wvq[idiv+ndiv]), bits1);
    bitcount += bits0 + bits1;
  }
  /*--- Forward envelope code ---*/
  if (fw_n_bit > 0){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	for ( idiv=0; idiv<fw_ndiv; idiv++ ){
	  itmp = idiv+(isf+i_ch*nsf)*fw_ndiv;
	  BsGetBitInt(stream, (unsigned int *)&(index_scl->fw[itmp]), fw_n_bit);
	  bitcount += fw_n_bit;
	}
      }
    }
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      for ( isf=0; isf<nsf; isf++ ){
	BsGetBitInt(stream, (unsigned int *)&(index_scl->fw_alf[i_ch*nsf+isf]),
		    ntt_FW_ARQ_NBIT);
	bitcount += ntt_FW_ARQ_NBIT;
      }
    }
  }
  
  /*--- Gain ---*/
  if (nsf == 1){
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      BsGetBitInt(stream, (unsigned int *)&(index_scl->pow[i_ch]),
		  ntt_GAIN_BITS_SCL);
      bitcount += ntt_GAIN_BITS_SCL;
    }
  }
  else{
    for ( i_ch=0; i_ch<n_ch; i_ch++ ){
      iptop = (nsf+1)*i_ch;
      BsGetBitInt(stream, (unsigned int *)&(index_scl->pow[iptop]),
		  ntt_GAIN_BITS_SCL);
      bitcount += ntt_GAIN_BITS_SCL;
      for ( isf=0; isf<nsf; isf++ ){
	BsGetBitInt(stream, (unsigned int *)&(index_scl->pow[iptop+isf+1]),
		    ntt_SUB_GAIN_BITS_SCL);
	bitcount += ntt_SUB_GAIN_BITS_SCL;
      }
    }
  }
  /*--- LSP ---*/
  for ( i_ch=0; i_ch<n_ch; i_ch++ ){
    /* pred. switch */
    BsGetBitInt(stream, (unsigned int *)&(index_scl->lsp[i_ch][0]),
		ntt_LSP_BIT0_SCL);
    bitcount += ntt_LSP_BIT0_SCL;
    /* first stage */
    BsGetBitInt(stream, (unsigned int *)&(index_scl->lsp[i_ch][1]),
		ntt_LSP_BIT1_SCL);
    bitcount += ntt_LSP_BIT1_SCL;
    /* second stage */
    for ( itmp=0; itmp<ntt_LSP_SPLIT_SCL; itmp++ ){
      BsGetBitInt(stream, (unsigned int *)&(index_scl->lsp[i_ch][itmp+2]),
		  ntt_LSP_BIT2_SCL);
      bitcount += ntt_LSP_BIT2_SCL;
    }
  }
}
  return(bitcount);
  
}

