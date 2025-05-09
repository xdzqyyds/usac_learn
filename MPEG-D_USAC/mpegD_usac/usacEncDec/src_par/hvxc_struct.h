
/*

This software module was originally developed by

    Akira Inoue (Sony Corporation)

    in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
    This software module is an implementation of a part of one or more
    MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
    standard (ISO/IEC 14496-3).
    ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
    free license to this software module or modifications thereof for use
    in hardware or software products claiming conformance to the MPEG-4
    Audio standards (ISO/IEC 14496-3).
    Those intending to use this software module in hardware or software
    products are advised that this use may infringe existing patents.
    The original developer of this software module and his/her company,
    the subsequent editors and their companies, and ISO/IEC have no
    liability for use of this software module or modifications thereof in
    an implementation.
    Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
    conforming products. The original developer retains full right to use
    the code for his/her own purpose, assign or donate the code to a third
    party and to inhibit third party from using the code for non MPEG-4
    Audio (ISO/IEC 14496-3) conforming products.
    This copyright notice must be included in all copies or derivative works.

    Copyright (c)1996.

*/

#ifndef _hvxc_struct_h
#define _hvxc_struct_h

#include "hvxc.h"

#ifdef __cplusplus
extern "C" {
#endif
  
/* definition of HVXC decoder status struct(AI 990129) */
struct HvxcDecStatusStruct
{
  /* AI 990616 */
  float **sampleBuf;
  int frameNumSample;
  int delayNumSample;

  /* configuration variables */
  int varMode;
  int rateMode;
  int extensionFlag;

  /* version 2 configuration (YM 990616) */
  int numClass;		/* for "EP" tool */
  int crcResultEP[2];
  int crcResultShape[2];
  int crcResult4k;
  int epStatus[2];
  int crcErrCnt;
  float mute;

  int exfrm;
  unsigned char chin[20];
  unsigned char encBitDouble[2][10];


  /* version 2 configuration (YM 990728) */
  int vrScalFlag;
  
  int decMode;
  int decDelayMode;
  int testMode;		/* HVXC test_mode (for decoder conformance testing) */
  float speedFact;
  float pitchFact;
  
  /* pan_lspdec() in pan_lspdec.c(for decMode == DEC0K) */
  int flagLspPred;

  /* DecParFrameHvx() in dec_par.c */
  int nbs;
  int fr0;
  float targetP;

  int idVUV2[2];
  int bgnFlag2[2];
  float pch2[2], am2[2][SAMPLE/2][3];
  float qLsp2[2][LPCORDER];
  float qRes2[2][FRM];

  IdCelp idCelp2[2];
  float	qLspQueue[4][LPCORDER]; 

  /* in hvxcDec.c */
  float ipc_coef[SAMPLE];
  float ipc_coefLD[SAMPLE];
  
  /* Bit2PrmVR() in hvxcDec.c */
  int prevVUV;
  
  /* IPC_UnPackBit2Prm(), IPC_UnPackBit2PrmVR() in hvxcDec.c */
  EncPrm encPrm;

  /* IPC_DecParams1st(), IPC_DecParams1st() in hvxcDec.c */
  int dec_frm;
  float prevQLsp[LPCORDER];
  float prevQLsp2[LPCORDER];
  int bgnCnt;
  int prevGainId;
  
  /* IPC_SynthSC() in hvxcDec.c */
  int syn_frm;
  int idVUVs[2];
  float pchs[2];
  float qLsps[2][P];
  int muteflag;
  
  /* harm_sew_dec() in hvxcQAmDec.c */
  float feneqold;
  
  /* IPC_vExt_fft() in hvxcVExtGenDec.c */
  int old_old_vuv;
  float wave2[SAMPLE/2];
  float pha2[SAMPLE/2];

  /* IPC_UvAdd() in hvxcVExtGenDec.c */
  float wnsozp[FRM*2];

  /* IPC_uvExt() in hvxcUVExtGenDec.c */
  float old_qRes[FRM/2];
  
  /* calc_syn_cont2v() in hvxcSynVDec.c */
  float lpv_mem2[P+1];

  /* calc_syn_cont2u() in hvxcSynUVDec.c */
  float lpu_mem2[P+1];
  
  /* IPC_posfil_v() in hvxcFltDec.c */
  float pfv_mem1[P+1];
  float pfv_mem2[P+1];
  float pfv_hpm;
  float pfv_oldgain;
  float pfv_alphaipFIRold[P+1];
  float pfv_alphaipIIRold[P+1];

  /* IPC_posfil_u(), IPC_posfil_u_LD() in hvxcFltDec.c */
  float pfu_mem1[P+1];
  float pfu_mem2[P+1];
  float pfu_hpm;
  float pfu_oldgain;
  float pfu_alphaipFIRold[P+1];
  float pfu_alphaipIIRold[P+1];
  float hpf_posfil_uv_state[4];
  
  /* IPC_bpf() in hvxcFltDec.c */
  float hpf_dec_state[4];
  float lpf3400_state[4];
  double hpf4ji_state[8];
  double lpf3400_coef[5];      
};

typedef struct HvxcDecStatusStruct HVXC_DATA; 

#ifdef __cplusplus
}
#endif

#endif		/* _hvxc_struct_h */

