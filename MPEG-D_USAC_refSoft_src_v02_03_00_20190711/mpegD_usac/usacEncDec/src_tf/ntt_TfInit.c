/**********************************************************************
 MPEG-4 Audio VM

 This software module was originally developed by
 
 Naoki Iwakami (NTT)
  
 and edited by
 Naoki Iwakami and Satoshi Miki (NTT) on 1996-05-01,                     
 Naoki Iwakami (NTT) on 1996-08-27,                                     
 Naoki Iwakami (NTT) on 1996-12-06,                                      
 Naoki Iwakami (NTT) on 1997-04-18,                                      
 Takehiro Moriya (NTT) on 1997-08-01,                                    
 Naoki Iwakami (NTT) on 1997-08-25,                                      
 Akio Jin (NTT) on 1997-10-23,                                           
 Kazuaki Chikira (NTT) on 2000-08-11,                                    
 Markus Werner       (SEED / Software Development Karlsruhe) 
 Olaf Kaehler (Fraunhofer IIS-A) on 2003-11-24
 
 in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
 ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
 implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
 as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
 users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
 software module or modifications thereof for use in hardware or
 software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
 standards. Those intending to use this software module in hardware or
 software products are advised that this use may infringe existing
 patents. The original developer of this software module and his/her
 company, the subsequent editors and their companies, and ISO/IEC have
 no liability for use of this software module or modifications thereof
 in an implementation. Copyright is not released for non MPEG-2
 AAC/MPEG-4 Audio conforming products. The original developer retains
 full right to use the code for his/her own purpose, assign or donate
 the code to a third party and to inhibit third party from using the
 code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
 copyright notice must be included in all copies or derivative works.
 
 Copyright (c) 2000.
 
 $Id: ntt_TfInit.c,v 1.1.1.1 2009-05-29 14:10:17 mul Exp $ 
 Twin VQ Decoder module
**********************************************************************/

#include <stdio.h>

#include "allHandles.h"

#include "nok_ltp_common.h"      /* structs */
#include "tf_mainStruct.h"       /* structs */
#include "tns.h"                 /* structs */

#include "common_m4a.h"
#include "ntt_conf.h"
#include "ntt_scale_conf.h"

static int ntt_crb_tbl_16[] = {
8,   16,  24,  32,  40,  48,  56,
64,  72,  80,  88,  100, 112, 124,
136, 148, 160, 172, 184, 196, 212,
228, 244, 260, 280, 300, 320, 344,
368, 396, 424, 456, 492, 532, 572,
616, 664, 716, 772, 832, 896, 1024
};   /* 42 scfbands AAC 16*/

  static int ntt_crb_tbl_24[] = {
4,    8,  12,  16,  20,  24,  28,
32,  36,  40,  44,  52,  60,  68,
76,  84,  92, 100, 108, 116, 124,
136,148, 160, 172, 188, 204, 220,
240,260, 284, 308, 336, 364, 396,
432,468, 508, 552, 600, 652, 1024
};   /* 42 scfbands AAC 24*/

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b)) 
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif


void ntt_base_init(float sampling_rate,
		float bit_rate,
		float t_bit_rate_scl,
		long numChannel,
		int block_size_samples,
		ntt_INDEX* ntt_index
		)
{
  /*--- Variables ---*/
  int    LSP_TBIT;
  int    GAIN_TBIT,  GAIN_TBIT_S;
  int    FW_TBIT;
  int    lsp_csize[2];
  int    lsp_cdim[2];
  float  ntt_bps;
  ntt_DATA_base  *nttDataBase;
  int    nbits_fr;
  int  ntt_N_DIV_P, ntt_PIT_TBIT;
 
  
  
  nttDataBase = (ntt_DATA_base *)calloc(1,sizeof(ntt_DATA_base));

  
  /*--- Set sampling frequency mode ---*/
  ntt_index->isampf = (int)(sampling_rate+0.5)/1000;


  /*--- Set bitrate mode ---*/
  ntt_bps = (bit_rate-t_bit_rate_scl)/(float)numChannel; 
  ntt_index->numChannel = numChannel;
  ntt_index->block_size_samples=block_size_samples;
  ntt_index->nttDataBase = nttDataBase;
  ntt_index->nttDataScl = NULL;

  /*----------------------------------------------------------------------*/
  /* PROGRAM MODE                                                         */
  /*----------------------------------------------------------------------*/

  /* number of bits for gain quantization tool */
  GAIN_TBIT = ntt_GAIN_BITS * numChannel;
  GAIN_TBIT_S = (ntt_GAIN_BITS + ntt_SUB_GAIN_BITS * ntt_N_SHRT) * numChannel;
  lsp_csize[0] = ntt_NC0;
  lsp_csize[1] = ntt_NC1;
  lsp_cdim[0]= ntt_N_PR;
  lsp_cdim[1]= ntt_N_PR;

  /*----------------------------------------------------------------------*/
  /* LSP QUANTIZATION                                                     */
  /*----------------------------------------------------------------------*/

  /* codebook memory allocation and reading */
  ntt_index->nttDataBase->lsp_code_base =
    (double *)malloc((ntt_NC0+ntt_NC1)*ntt_N_PR_MAX*sizeof(double));
  ntt_index->nttDataBase->lsp_fgcode_base = 
    (double *)malloc(ntt_N_MODE*ntt_MA_NP*ntt_N_PR_MAX*sizeof(double));
  ntt_get_code(ntt_LSPCODEBOOK, ntt_LSP_NSTAGE,
	       lsp_csize, lsp_cdim,
	       (double (*)[ntt_N_PR_MAX])ntt_index->nttDataBase->lsp_code_base,
	       (double (*)[ntt_MA_NP][ntt_N_PR_MAX])ntt_index->nttDataBase->lsp_fgcode_base);
  /* number of bits for LSP quantization tool */
  LSP_TBIT =
    (ntt_LSP_BIT0 + ntt_LSP_BIT1 + ntt_LSP_BIT2 * ntt_LSP_SPLIT) * numChannel;

  ntt_index->nttDataBase->prev_lsp_code = (int *)calloc(ntt_LSP_NIDX_MAX*numChannel,sizeof(int));
  ntt_index->nttDataBase->ma_np= 0;
 
  /*----------------------------------------------------------------------*/
  /* FORWARD ENVELOPE QUANTIZATION                                        */
  /*----------------------------------------------------------------------*/

  /*--- Load bark-scale subband tables ---*/
  if(sampling_rate <= 16000.){
    ntt_index->nttDataBase->ntt_crb_tbl = ntt_crb_tbl_16;
    if ((block_size_samples == 960)){ ntt_crb_tbl_16[41]=960; }
  }
  else{
    ntt_index->nttDataBase->ntt_crb_tbl = ntt_crb_tbl_24;
    if ((block_size_samples == 960)){ ntt_crb_tbl_24[41]=960; }
  }
    

  if (ntt_FW_N_BIT > 0)
    FW_TBIT =
      (ntt_FW_N_BIT*ntt_FW_N_DIV+ntt_FW_ARQ_NBIT) * numChannel;
  else
    FW_TBIT = 0;
   
  ntt_index->nttDataBase->prev_fw_code = (int *)calloc(ntt_FW_N_DIV*numChannel,sizeof(int));
  ntt_index->nttDataBase->prev_w_type= -1;
  
  
  /*----------------------------------------------------------------------*/
  /* PITCH FILTER                                                         */
  /*----------------------------------------------------------------------*/
    ntt_N_DIV_P  = ntt_N_DIV_PperCH * numChannel;
    ntt_PIT_TBIT = ntt_PIT_TBITperCH * numChannel;

  /*----------------------------------------------------------------------*/
  /* BITRATE CONTROL                                                      */
  /*----------------------------------------------------------------------*/
{
  float ftmp;
  int ntt_bps_i;
  int bandUpper_i;

  /* Number of bits per frame */
  ftmp = (float)(block_size_samples*numChannel)*ntt_bps; /****/
  ftmp /= (float)sampling_rate;
  ftmp += 0.5;
  nbits_fr = (int)ftmp;
#if BYTE_ALIGN
  /* kaehleof 20040303 : round up for byte align!!! */
  nbits_fr = ((nbits_fr+7)/8)*8; /* BYTE-A T.Ishikawa 981118 */
#endif
  ntt_bps_i = (nbits_fr*(int)sampling_rate/block_size_samples/(int)numChannel);
  bandUpper_i = (95 * ntt_bps_i) / ntt_index->isampf;
  bandUpper_i = min(100000, bandUpper_i);
  bandUpper_i *= 16384;
  bandUpper_i += 1562;
  bandUpper_i /= 3125;
  ntt_index->nttDataBase->ntt_NBITS_FR = nbits_fr;
  ntt_index->nttDataBase->bandLower =  0.0;
  ntt_index->nttDataBase->bandUpper =  (double)bandUpper_i / 16384.0/32.;
}

  /*----------------------------------------------------------------------*/
  /* READING CODEBOOK TABLES                                              */
  /*----------------------------------------------------------------------*/
{
  int    cb_len_max;
  /*--- Interleaved vector quantization ---*/
  /* long frame */
  cb_len_max = ntt_CB_LEN_READ + ntt_CB_LEN_MGN;
  ntt_index->nttDataBase->ntt_codev0 = (double *)malloc(ntt_CB_SIZE*cb_len_max*sizeof(double));
  ntt_index->nttDataBase->ntt_codev1 = (double *)malloc(ntt_CB_SIZE*cb_len_max*sizeof(double));

  ntt_get_cdbk(ntt_CB_NAME0, ntt_index->nttDataBase->ntt_codev0,
	       ntt_CB_SIZE, ntt_CB_LEN_READ, cb_len_max);
  ntt_get_cdbk(ntt_CB_NAME1, ntt_index->nttDataBase->ntt_codev1,
	       ntt_CB_SIZE, ntt_CB_LEN_READ, cb_len_max);
  /* short frame */
  cb_len_max = ntt_CB_LEN_READ_S + ntt_CB_LEN_MGN;
  ntt_index->nttDataBase->ntt_codev0s = (double *)malloc(ntt_CB_SIZE*cb_len_max*sizeof(double));
  ntt_index->nttDataBase->ntt_codev1s = (double *)malloc(ntt_CB_SIZE*cb_len_max*sizeof(double));
  ntt_get_cdbk(ntt_CB_NAME0s, ntt_index->nttDataBase->ntt_codev0s,
	       ntt_CB_SIZE, ntt_CB_LEN_READ_S, cb_len_max);
  ntt_get_cdbk(ntt_CB_NAME1s, ntt_index->nttDataBase->ntt_codev1s,
	       ntt_CB_SIZE, ntt_CB_LEN_READ_S, cb_len_max);
  /*--- Bark-scale envelope ---*/
  /* long frame */
  ntt_index->nttDataBase->ntt_fwcodev =
    (double *)malloc(ntt_FW_CB_SIZE*ntt_FW_CB_LEN*sizeof(double));
  ntt_get_cdbk(ntt_FW_CB_NAME, ntt_index->nttDataBase->ntt_fwcodev,
	       ntt_FW_CB_SIZE, ntt_FW_CB_LEN, ntt_FW_CB_LEN);
  ntt_index->nttDataBase->ntt_codevp0 =
    (double *)malloc(ntt_PIT_CB_SIZE*2*ntt_CB_LEN_P_MAX*sizeof(double));
  
  ntt_get_cdbk(ntt_PIT_CB_NAME, ntt_index->nttDataBase->ntt_codevp0, ntt_PIT_CB_SIZE*2, 
	       ntt_CB_LEN_P, ntt_CB_LEN_P_MAX );

}

{
  /*--- Sin and Cosine table ---*/
 
  double dt, theta;
  int    ismp;
  
  ntt_index->nttDataBase->cos_TT = (double *)calloc(block_size_samples*2+1,sizeof(double));

  dt = PI/(double)(4*block_size_samples);
  for ( ismp=0; ismp<block_size_samples*2; ismp++ ){
    theta = dt*ismp;
    ntt_index->nttDataBase->cos_TT[ismp]= cos(theta);
  }
   ntt_index->nttDataBase->cos_TT[block_size_samples*2] =0.;
}
  /*--- Set the interleaving tables ---*/
{
  int j, i;
  short serial;
  int i_div, acci;

  short *ntt_BIT_REVp, *ntt_SMP_ACCp;
  int bits[ntt_N_DIV_P_MAX*2], length[ntt_N_DIV_P_MAX*2];

  ntt_index->nttDataBase->pleave0 = (short *)calloc(20,sizeof(short));
  ntt_index->nttDataBase->pleave1 = (short *)calloc(2048,sizeof(short));

  ntt_SMP_ACCp = ntt_index->nttDataBase->pleave0;
  ntt_BIT_REVp = ntt_index->nttDataBase->pleave1;


  ntt_BIT_REVp[0] = 0;
  ntt_vec_lenp(numChannel, bits, length);
  serial =0;
  for(i = 0 ; i < ntt_N_DIV_P ; i++) {
    for(j=0; j<length[0]-1; j++){
	    if((numChannel==1) ||((ntt_N_DIV_P%numChannel)!=0)) 
        acci = i+ntt_N_DIV_P*j;
	    else                 
        acci = ((i+j)%ntt_N_DIV_P)+ntt_N_DIV_P*j;
	    ntt_BIT_REVp[serial++] = acci/numChannel + (acci%numChannel)*ntt_N_FR_P;
    } 
    for(j=length[0]-1; j<length[i]; j++){
	    acci = i+ntt_N_DIV_P*j;
	    ntt_BIT_REVp[serial++] = acci/numChannel + (acci%numChannel)*ntt_N_FR_P;
    } 
  }
  acci=0;
  for (i_div=0; i_div<ntt_N_DIV_P; i_div++){
    ntt_SMP_ACCp[i_div] = acci; 
    acci += length[i_div]; 
  }
 
}

}

void ntt_base_free(ntt_DATA_base *nttDataBase)
{

  free(nttDataBase->lsp_code_base);
  free(nttDataBase->lsp_fgcode_base);
  free(nttDataBase->prev_lsp_code);
  free(nttDataBase->prev_fw_code);
  free(nttDataBase->ntt_codev0);
  free(nttDataBase->ntt_codev1);
  free(nttDataBase->ntt_codev0s);
  free(nttDataBase->ntt_codev1s);
  free(nttDataBase->ntt_fwcodev);
  free(nttDataBase->ntt_codevp0);
  free(nttDataBase->cos_TT);
  free(nttDataBase->pleave0);
  free(nttDataBase->pleave1);
}



