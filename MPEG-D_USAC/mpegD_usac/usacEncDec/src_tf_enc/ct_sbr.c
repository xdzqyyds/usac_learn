/****************************************************************************

SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Coding Technologies

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3
standards for reference purposes and its performance may not have been
optimized. This software module is an implementation of one or more tools as
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications
thereof for use in products claiming conformance to audiovisual and
image-coding related ITU Recommendations and/or ISO/IEC International
Standards. ISO/IEC gives users the same free license to this software module or
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its
use may infringe existing patents. ISO/IEC have no liability for use of this
software module or modifications thereof. Copyright is not released for
products that do not conform to audiovisual and image-coding related ITU
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its
own purpose, assign or donate the code to a third party and to inhibit third
parties from using the code for products that do not conform to audiovisual and
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 2002.

$Id: ct_sbr.c,v 1.16.4.2 2012-04-24 12:36:55 frd Exp $

*******************************************************************************/
/*!
  \file

  \brief  Sbr encoding functions $Revision: 1.17 $

*/

/* 20060107 */
#undef PARAMETRICSTEREO /*temporally disabled for BSAC Extensions*/

#include <math.h>
#include <assert.h>

#include "ct_sbr.h"
#include "interface.h" /* ID_FIL, LEN_SE_ID */
#include "ct_freqsca.h"
#ifdef PARAMETRICSTEREO
#include "ct_psenc.h"
#endif

#ifdef SONY_PVC
#include "sony_pvcenc_prepro.h"
#endif /* SONY_PVC */

#ifdef SONY_PVC_ENC
#include "sony_pvcenc.h"
#include "sony_pvclib.h"
#endif

#define EPS                                     1e-12
#define ILOG2                                   1.442695041f /* 1/LOG2 */

#define    LEN_EX_TYPE  (4)

#define SI_SBR_STD_EXTENSION     13  /* 1101 */
#define SI_SBR_CRC_EXTENSION     14  /* 1110 */

#define SI_SBR_HDR_BIT            1

/* header */

#define SI_SBR_PROTOCOL_VERSION_BITS            2
#define SI_SBR_AMP_RES_BITS                     1
#define SI_SBR_START_FREQ_BITS                  4
#define SI_SBR_STOP_FREQ_BITS                   4
#define SI_SBR_XOVER_BAND_BITS                  3
#define SI_USAC_SBR_XOVER_BAND_BITS             4
#define SI_SBR_PRE_FLAT_BITS                    1
#define SI_SBR_RESERVED_BITS_HDR                1
#define SI_SBR_HEADER_EXTRA_1_BITS              1
#define SI_SBR_HEADER_EXTRA_2_BITS              1
#ifdef SONY_PVC_ENC
#define SI_SBR_HEADER_EXTRA_3_BITS              1
#define SI_PVC_MODE_BITS		        2
#endif

/* data */
#define SI_SBR_PATCHING_MODE                    1
#define SI_SBR_OVERSAMPLING_FLAG                1
#define SI_SBR_COUPLING_BITS                    1

#define SI_SBR_PITCH_FLAG                       1
#define SI_SBR_PITCH_ESTIMATE                   7

#define SBR_CLA_BITS    2
#define SBR_RES_BITS    1
#define FIXFIX  0

#define SI_SBR_INVF_MODE_BITS                   2


#define SI_SBR_RESERVED_D2_BITS                 1
#define SI_SBR_ADD_HARMONIC_ENABLE_BITS         1

#define SI_SBR_START_ENV_BITS_AMP_RES_3_0       6
#define SI_SBR_START_NOISE_BITS_AMP_RES_3_0     5


#define SI_SBR_EXTENDED_DATA_BITS               1

#define TEMP_SHAPE_BITS			        1
#define INTER_TEMP_SHAPE_MODE_BITS	        2

#ifdef SONY_PVC_ENC
#define SI_PVC_DIVMODE_BITS                     3
#define SI_PVC_NSMODE_BITS                      1
#define SI_PVC_GRID_INFO_BITS                   1
#define SI_PVC_ID_BITS	                        7
#endif

/* direction: freq
   contents : codewords
   raw table: HuffCode2.m
   built by : FH 00-02-04 */

/* 20060107 */
const int v_Huff_envelopeLevelC11F[63] = {
  0x000FFFF0, 0x000FFFF1, 0x000FFFF2, 0x000FFFF3, 0x000FFFF4, 0x000FFFF5,
    0x000FFFF6, 0x0003FFF3,
  0x0007FFF5, 0x0007FFEE, 0x0007FFEF, 0x0007FFF6, 0x0003FFF4, 0x0003FFF2,
    0x000FFFF7, 0x0007FFF0,
  0x0001FFF5, 0x0003FFF0, 0x0001FFF4, 0x0000FFF7, 0x0000FFF6, 0x00007FF8,
    0x00003FFB, 0x00000FFD,
  0x000007FD, 0x000003FD, 0x000001FD, 0x000000FD, 0x0000003E, 0x0000000E,
    0x00000002, 0x00000000,
  0x00000006, 0x0000001E, 0x000000FC, 0x000001FC, 0x000003FC, 0x000007FC,
    0x00000FFC, 0x00001FFC,
  0x00003FFA, 0x00007FF9, 0x00007FFA, 0x0000FFF8, 0x0000FFF9, 0x0001FFF6,
    0x0001FFF7, 0x0003FFF5,
  0x0003FFF6, 0x0003FFF1, 0x000FFFF8, 0x0007FFF1, 0x0007FFF2, 0x0007FFF3,
    0x000FFFF9, 0x0007FFF7,
  0x0007FFF4, 0x000FFFFA, 0x000FFFFB, 0x000FFFFC, 0x000FFFFD, 0x000FFFFE,
    0x000FFFFF
};


/* direction: freq
   contents : codeword lengths
   raw table: HuffCode2.m
   built by : FH 00-02-04 */

/* 20060107 */
const int v_Huff_envelopeLevelL11F[63] = {
  0x00000014, 0x00000014, 0x00000014, 0x00000014, 0x00000014, 0x00000014,
    0x00000014, 0x00000012,
  0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000012, 0x00000012,
    0x00000014, 0x00000013,
  0x00000011, 0x00000012, 0x00000011, 0x00000010, 0x00000010, 0x0000000F,
    0x0000000E, 0x0000000C,
  0x0000000B, 0x0000000A, 0x00000009, 0x00000008, 0x00000006, 0x00000004,
    0x00000002, 0x00000001,
  0x00000003, 0x00000005, 0x00000008, 0x00000009, 0x0000000A, 0x0000000B,
    0x0000000C, 0x0000000D,
  0x0000000E, 0x0000000F, 0x0000000F, 0x00000010, 0x00000010, 0x00000011,
    0x00000011, 0x00000012,
  0x00000012, 0x00000012, 0x00000014, 0x00000013, 0x00000013, 0x00000013,
    0x00000014, 0x00000013,
  0x00000013, 0x00000014, 0x00000014, 0x00000014, 0x00000014, 0x00000014,
    0x00000014
};


static int countSbrHdr(int nChan, SBR_MODE sbrMode)
{
  int bit_count = 0;

  bit_count += SI_SBR_AMP_RES_BITS +
               SI_SBR_START_FREQ_BITS +
               SI_SBR_STOP_FREQ_BITS +
               SI_SBR_PRE_FLAT_BITS +
               SI_SBR_RESERVED_BITS_HDR +
               SI_SBR_HEADER_EXTRA_1_BITS +
               SI_SBR_HEADER_EXTRA_2_BITS;
#ifdef SONY_PVC_ENC
  bit_count += SI_SBR_HEADER_EXTRA_3_BITS + SI_PVC_MODE_BITS;
#endif

  switch (sbrMode ) {
    case SBR_TF:
      bit_count += SI_SBR_XOVER_BAND_BITS;
      break;
    case SBR_USAC:
    case SBR_USAC_HARM:
      bit_count += SI_USAC_SBR_XOVER_BAND_BITS;
      break;
    default:
      break;
  }

  return bit_count;
}


/*	static void writeSbrHdr(BsBitStream *ptrBS, SBR_CHAN *sbrChan, int nChan)*/
static void writeSbrHdr(HANDLE_BSBITSTREAM ptrBS, SBR_CHAN *sbrChan, int nChan, SBR_MODE sbrMode
#ifdef SONY_PVC_ENC
	,int pvc_mode
#endif
)
{
  BsPutBit(ptrBS, 1, SI_SBR_AMP_RES_BITS);
  BsPutBit(ptrBS, sbrChan->startFreq, SI_SBR_START_FREQ_BITS);
  BsPutBit(ptrBS, sbrChan->stopFreq, SI_SBR_STOP_FREQ_BITS);
  switch (sbrMode ) {
  case SBR_TF:
    BsPutBit(ptrBS, 0,  SI_SBR_XOVER_BAND_BITS);
    break;
  case SBR_USAC:
  case SBR_USAC_HARM:
    BsPutBit(ptrBS, 0,  SI_USAC_SBR_XOVER_BAND_BITS);
    break;
  default:
    break;
  }
  BsPutBit(ptrBS, 1,  SI_SBR_PRE_FLAT_BITS);
  BsPutBit(ptrBS, 0,  SI_SBR_RESERVED_BITS_HDR);
  BsPutBit(ptrBS, 0,  SI_SBR_HEADER_EXTRA_1_BITS);
  BsPutBit(ptrBS, 0,  SI_SBR_HEADER_EXTRA_2_BITS);
#ifdef SONY_PVC_ENC
  BsPutBit(ptrBS, 1,  SI_SBR_HEADER_EXTRA_3_BITS);
  BsPutBit(ptrBS, pvc_mode,  SI_PVC_MODE_BITS);
#endif
}


static 
int BsPutCntBit (HANDLE_BSBITSTREAM stream,   /* in: bit stream */
                 unsigned long data,          /* in: bits to write */
                 int numBit){

  int nBitsWritten = 0;

  if(stream){
    if(0 == BsPutBit(stream, data, numBit)){
      nBitsWritten = numBit;
    }
  } else {
    nBitsWritten = numBit;
  }

  return nBitsWritten;
}

static int writeSbrInfoUsac(HANDLE_BSBITSTREAM ptrBS, SBR_CHAN *sbrChan, int bs_pvc
#ifdef SONY_PVC_ENC
                            ,int pvc_mode
#endif
                            ){

  int nBitsWritten = 0;

  nBitsWritten += BsPutCntBit(ptrBS, 1, SI_SBR_AMP_RES_BITS);
  nBitsWritten += BsPutCntBit(ptrBS, 0, SI_USAC_SBR_XOVER_BAND_BITS);
  nBitsWritten += BsPutCntBit(ptrBS, 1, SI_SBR_PRE_FLAT_BITS);
  if(bs_pvc){
    nBitsWritten += BsPutCntBit(ptrBS, pvc_mode,  SI_PVC_MODE_BITS);
  }

  return nBitsWritten;
}

static int writeSbrHeaderDataUsac(HANDLE_BSBITSTREAM ptrBS, SBR_CHAN *sbrChan){

  int nBitsWritten = 0;

  nBitsWritten += BsPutCntBit(ptrBS, sbrChan->startFreq, SI_SBR_START_FREQ_BITS);
  nBitsWritten += BsPutCntBit(ptrBS, sbrChan->stopFreq , SI_SBR_STOP_FREQ_BITS);
  nBitsWritten += BsPutCntBit(ptrBS, 0                 , SI_SBR_HEADER_EXTRA_1_BITS);
  nBitsWritten += BsPutCntBit(ptrBS, 0                 , SI_SBR_HEADER_EXTRA_2_BITS);
  
  return nBitsWritten;
}


static int writeSbrHdrUsac(HANDLE_BSBITSTREAM ptrBS, SBR_CHAN *sbrChan, int bUsacIndependencyFlag, int sbrInfoPresent, int sbrHeaderPresent, int bs_pvc
#ifdef SONY_PVC_ENC
                            ,int pvc_mode
#endif
                            )
{
  
  int nBitsWritten = 0;

  if(bUsacIndependencyFlag){
    /* independency flags overwrites decision */
    sbrInfoPresent   = 1;
    sbrHeaderPresent = 1;
  } else {
    nBitsWritten += BsPutCntBit(ptrBS, sbrInfoPresent, 1);
    if(sbrInfoPresent){
      nBitsWritten += BsPutCntBit(ptrBS, sbrHeaderPresent, 1);
    } else {
      sbrHeaderPresent = 0;
    }
  }

  if(sbrInfoPresent){
    nBitsWritten += writeSbrInfoUsac(ptrBS, sbrChan, bs_pvc, pvc_mode);
  }

  if(sbrHeaderPresent){
    int sbrUseDfltHeader = 0;
    nBitsWritten +=  BsPutCntBit(ptrBS, sbrUseDfltHeader, 1);
    if(!sbrUseDfltHeader){
      nBitsWritten += writeSbrHeaderDataUsac(ptrBS, sbrChan);
    }
  }

  return nBitsWritten;
}


static int countSbrGrid(
#ifdef SONY_PVC_ENC
	int pvc_mode
#else
	void
#endif
)
{
  int bit_count=0;

#ifdef SONY_PVC_ENC
  if(pvc_mode){

#ifndef SONY_PVC_BITPACK_NOFIX
    bit_count += 4; /* bs_noise_position */
    bit_count += 1; /* bs_var_len_hf = 0 */
#else
	bit_count += 1; /* bs_var_len_hf = 0 */
	bit_count += 4; /* bs_noise_position */
#endif

  }else{
    bit_count += SBR_CLA_BITS; /* frame_class */
    bit_count += 2; /* temp */
    bit_count += SBR_RES_BITS; /* freq_res */
  }
#else
  bit_count += SBR_CLA_BITS; /* frame_class */
  bit_count += 2; /* temp */
  bit_count += SBR_RES_BITS; /* freq_res */
#endif

  return bit_count;
}

/*	static void writeSbrGrid(BsBitStream *ptrBS) */
static void writeSbrGrid(HANDLE_BSBITSTREAM ptrBS
#ifdef SONY_PVC_ENC
	,int pvc_mode
#endif
)
{
#ifdef SONY_PVC_ENC
  if(pvc_mode){

#ifndef SONY_PVC_BITPACK_NOFIX
    BsPutBit(ptrBS, 8, 4); /* bs_noise_position = 8 */
    BsPutBit(ptrBS, 0, 1); /* bs_var_len_hf = 0 */
#else
	BsPutBit(ptrBS, 0, 1); /* bs_var_len_hf = 0 */
	BsPutBit(ptrBS, 8, 4); /* bs_noise_position = 8 */
#endif

  }else{
    BsPutBit(ptrBS, FIXFIX, SBR_CLA_BITS);
    BsPutBit(ptrBS, 1, 2); /* 2 envelopes per frame */
    BsPutBit(ptrBS, 1, SBR_RES_BITS);
  }
#else
  BsPutBit(ptrBS, FIXFIX, SBR_CLA_BITS);
  BsPutBit(ptrBS, 1, 2); /* 2 envelopes per frame */
  BsPutBit(ptrBS, 1, SBR_RES_BITS);
#endif
}

static int countSbrDtdf(
#ifdef SONY_PVC_ENC
                        int pvc_mode
#endif
                       ,const int bUsacIndependenceFlag
) {
  int bit_count=0;
  int env = 0, noise = 0;

  if (bUsacIndependenceFlag){
    env = 1;
    noise = 1;
  }

#ifdef SONY_PVC_ENC
  if(pvc_mode == 0){
#endif
    for (env=env; env<2; env++) {
      bit_count += 1;
    }
#ifdef SONY_PVC_ENC
  }
#endif

  for (noise=noise; noise<2; noise++) {
    bit_count += 1;
  }

  return bit_count;
}

/*static void writeSbrDtdf(BsBitStream *ptrBs)*/
static void writeSbrDtdf(HANDLE_BSBITSTREAM ptrBs
#ifdef SONY_PVC_ENC
                         ,int pvc_mode
#endif
                         ,int const bUsacIndependenceFlag
)
{
  int env = 0, noise = 0;

  if(bUsacIndependenceFlag){
    env = 1;
    noise = 1;
  }

#ifdef SONY_PVC_ENC
  if(pvc_mode == 0){
#endif
  for (env=env; env<2; env++)
    BsPutBit(ptrBs, 0, 1); /* delta f */
#ifdef SONY_PVC_ENC
  }
#endif

  for (noise=noise; noise<2; noise++)
    BsPutBit(ptrBs, 0, 1); /* delta f */
}

static int countSbrEnvelope(SBR_CHAN *sbrChan)
{
  int bit_count=0;
  int env, band;
  int delta;
  int inter_tes_mode = 1;

  for (env=0; env<2; env++) {
    bit_count += SI_SBR_START_ENV_BITS_AMP_RES_3_0;
    for (band=1; band<sbrChan->numBands; band++) {
      delta = sbrChan->envData[env][band] - sbrChan->envData[env][band-1] + 31;
      bit_count += v_Huff_envelopeLevelL11F[delta];
    }
    if(sbrChan->bs_interTes){
      if(inter_tes_mode > 0)
        bit_count += TEMP_SHAPE_BITS + INTER_TEMP_SHAPE_MODE_BITS;
      else
        bit_count += TEMP_SHAPE_BITS;
    }
  }

  return bit_count;
}

/*static void writeSbrEnvelope(BsBitStream *ptrBs, SBR_CHAN *sbrChan)*/
static void writeSbrEnvelope(HANDLE_BSBITSTREAM ptrBs, SBR_CHAN *sbrChan)
{
  int env, band;
  int delta;
  int inter_tes_mode = 1;

  for (env=0; env<2; env++) {
    BsPutBit(ptrBs, sbrChan->envData[env][0],
             SI_SBR_START_ENV_BITS_AMP_RES_3_0);
    for (band=1; band<sbrChan->numBands; band++) {
      delta = sbrChan->envData[env][band] - sbrChan->envData[env][band-1] + 31;
      BsPutBit(ptrBs, v_Huff_envelopeLevelC11F[delta],
               v_Huff_envelopeLevelL11F[delta]);
    }
    if(sbrChan->bs_interTes){
      if(inter_tes_mode > 0){
        BsPutBit(ptrBs, 1, TEMP_SHAPE_BITS);
        BsPutBit(ptrBs, inter_tes_mode, INTER_TEMP_SHAPE_MODE_BITS);
      }
      else
        BsPutBit(ptrBs, 0, TEMP_SHAPE_BITS);
    }
  }
}

static int countSbrNoise(SBR_CHAN *sbrChan)
{
  int bit_count=0;
  int noise, band;
  int delta;

  for (noise=0; noise<2; noise++) {
    bit_count += SI_SBR_START_NOISE_BITS_AMP_RES_3_0;
    for (band=1; band<sbrChan->numNoiseBands; band++) {
      delta = sbrChan->noiseData[noise][band] - sbrChan->noiseData[noise][band]
              + 31;
      bit_count += v_Huff_envelopeLevelL11F[delta];
    }
  }

  return bit_count;
}

/*static void writeSbrNoise(BsBitStream *ptrBs, SBR_CHAN *sbrChan)*/
static void writeSbrNoise(HANDLE_BSBITSTREAM ptrBs, SBR_CHAN *sbrChan)
{
  int noise, band;
  int delta;

  for (noise=0; noise<2; noise++) {
    BsPutBit(ptrBs, sbrChan->noiseData[noise][0],
             SI_SBR_START_NOISE_BITS_AMP_RES_3_0);
    for (band=1; band<sbrChan->numNoiseBands; band++) {
      delta = sbrChan->noiseData[noise][band] - sbrChan->noiseData[noise][band]
              + 31;
      BsPutBit(ptrBs, v_Huff_envelopeLevelC11F[delta],
               v_Huff_envelopeLevelL11F[delta]);
    }
  }
}

#ifdef SONY_PVC_ENC
static int countPvcEnvelope(SBR_CHAN *sbrChan)
{
  return sbrChan->hPVC.pvc_bit_count;
}

static void writePvcEnvelope(HANDLE_BSBITSTREAM ptrBs, SBR_CHAN *sbrChan
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
                            ,int const bUsacIndependenceFlag
#endif
							 )
{ 
    int             i;
    unsigned short  *p_pvcID_bs;
    PVC_PARAM       *p_pvcParam;
    PVC_BSINFO      *p_pvcBsinfo;

    p_pvcParam = &(sbrChan->hPVC.pvcParam);
    p_pvcBsinfo = &(sbrChan->hPVC.pvcBsinfo);
    p_pvcID_bs = p_pvcBsinfo->pvcID_bs;

    BsPutBit(ptrBs, p_pvcBsinfo->divMode, PVC_NBIT_DIVMODE);
    BsPutBit(ptrBs, p_pvcBsinfo->nsMode, PVC_NBIT_NSMODE);

    if (p_pvcBsinfo->divMode == 0) {

        if (p_pvcBsinfo->grid_info[0]) { 
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
			if(!bUsacIndependenceFlag){
				BsPutBit(ptrBs, 0, PVC_NBIT_REUSE_PVCID);
			}
#else
			BsPutBit(ptrBs, 0, PVC_NBIT_REUSE_PVCID);
#endif
            BsPutBit(ptrBs, *(p_pvcID_bs++), p_pvcParam->pvc_id_nbit);
        } else {
            BsPutBit(ptrBs, 1, PVC_NBIT_REUSE_PVCID);
        }

    } else {

        for (i=0; i<p_pvcBsinfo->num_grid_info; i++) {
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
			if(!(bUsacIndependenceFlag && (i ==0))){
				BsPutBit(ptrBs, p_pvcBsinfo->grid_info[i], PVC_NBIT_GRID_INFO);
			}
#else
            BsPutBit(ptrBs, p_pvcBsinfo->grid_info[i], PVC_NBIT_GRID_INFO);
#endif
            if (p_pvcBsinfo->grid_info[i]) {
                BsPutBit(ptrBs, *(p_pvcID_bs++), p_pvcParam->pvc_id_nbit);
            }
        }

    }
    
}
#endif
static int countSbrSCEdata(SBR_CHAN *sbrChan,
                           SBR_MODE sbrMode,
#ifdef SONY_PVC_ENC
                           int pvc_mode
#endif
                          ,const int bUsacIndependenceFlag
) {
  int bit_count = 0;
  int band;

  if ( sbrMode == SBR_TF )
    bit_count += 1; /* reserved bits present */

  if ( sbrMode == SBR_USAC_HARM ) {
    bit_count += SI_SBR_PATCHING_MODE;
    bit_count += SI_SBR_OVERSAMPLING_FLAG;
    if(0/*sbrChan->sbrPitchInBins > 0 set off for now */) {
      bit_count += SI_SBR_PITCH_FLAG;
      bit_count += SI_SBR_PITCH_ESTIMATE;
    } else {
      bit_count += SI_SBR_PITCH_FLAG;
    }
  }

#ifdef SONY_PVC_ENC
  bit_count += countSbrGrid(pvc_mode);
#else
  bit_count += countSbrGrid();
#endif
  bit_count += countSbrDtdf(
#ifdef SONY_PVC_ENC
                            pvc_mode
#endif
                           ,bUsacIndependenceFlag);

  for (band=0; band<sbrChan->numNoiseBands; band++) {
    bit_count += SI_SBR_INVF_MODE_BITS;
  }

#ifdef SONY_PVC_ENC
  if(pvc_mode == 0){
    bit_count += countSbrEnvelope(sbrChan);
    bit_count += countSbrNoise(sbrChan);
  }else{
    bit_count += countPvcEnvelope(sbrChan);
    bit_count += countSbrNoise(sbrChan);
  }
#else
  bit_count += countSbrEnvelope(sbrChan);

  bit_count += countSbrNoise(sbrChan);
#endif

  bit_count += SI_SBR_ADD_HARMONIC_ENABLE_BITS;

  if ( sbrMode == SBR_TF )
    bit_count += SI_SBR_EXTENDED_DATA_BITS;

#ifdef PARAMETRICSTEREO
  if (sbrChan->psAvailable) {
    bit_count += CountSbrPs(&sbrChan->psData);
  }
#endif

  return bit_count;
}


/*static void writeSbrSCEdata(BsBitStream *ptrBs, */
static void writeSbrSCEdata(HANDLE_BSBITSTREAM ptrBs,
                            SBR_CHAN *sbrChan,
                            SBR_MODE sbrMode
#ifdef SONY_PVC_ENC
                            ,int pvc_mode
#endif
                            ,int const bUsacIndependenceFlag
                            )
{
  int band;

  if ( sbrMode == SBR_TF )
    BsPutBit(ptrBs, 0, 1); /* no reserved bits */

  if ( sbrMode == SBR_USAC_HARM ) {
    BsPutBit(ptrBs, 0, SI_SBR_PATCHING_MODE);
    BsPutBit(ptrBs, sbrChan->sbrOversamplingFlag, SI_SBR_OVERSAMPLING_FLAG);
    if(sbrChan->sbrPitchInBins > 0) {
      BsPutBit(ptrBs, 1, SI_SBR_PITCH_FLAG);
      BsPutBit(ptrBs, sbrChan->sbrPitchInBins, SI_SBR_PITCH_ESTIMATE);
    } else {
      BsPutBit(ptrBs, 0, SI_SBR_PITCH_FLAG);
    }
  }

#ifdef SONY_PVC_ENC  
  writeSbrGrid(ptrBs, pvc_mode);
  writeSbrDtdf(ptrBs, pvc_mode, bUsacIndependenceFlag);
#else
  writeSbrGrid(ptrBs);
  writeSbrDtdf(ptrBs, bUsacIndependenceFlag);
#endif

  for (band=0; band<sbrChan->numNoiseBands; band++)
    BsPutBit(ptrBs, 0, SI_SBR_INVF_MODE_BITS);

#ifdef SONY_PVC_ENC
  
  if(pvc_mode == 0){
    writeSbrEnvelope(ptrBs, sbrChan);
    writeSbrNoise(ptrBs, sbrChan);
  }else{
	writePvcEnvelope(ptrBs, sbrChan
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
		, bUsacIndependenceFlag
#endif
		);
	writeSbrNoise(ptrBs, sbrChan);
  }

#else
  writeSbrEnvelope(ptrBs, sbrChan);

  writeSbrNoise(ptrBs, sbrChan);
#endif

  BsPutBit(ptrBs, 0, SI_SBR_ADD_HARMONIC_ENABLE_BITS);

#ifdef PARAMETRICSTEREO
  if (sbrChan->psAvailable) {
    BsPutBit(ptrBs, 1, SI_SBR_EXTENDED_DATA_BITS); /* PS in extended data */
    WriteSbrPs(&sbrChan->psData, ptrBs);
  }
  else
#endif
  {
    if ( sbrMode == SBR_TF )
      BsPutBit(ptrBs, 0, SI_SBR_EXTENDED_DATA_BITS); /* no extended data */
  }
}

static int countSbrCPEdata(SBR_CHAN *sbrChanL, SBR_CHAN *sbrChanR, SBR_MODE sbrMode, const int bUsacIndependenceFlag)
{
  int bit_count = 0;
  int band;

  if ( sbrMode == SBR_TF )
    bit_count += 1; /* flag reserved bits */

  bit_count += SI_SBR_COUPLING_BITS;
  
  if ( sbrMode == SBR_USAC_HARM ) {
    bit_count += SI_SBR_PATCHING_MODE;
    bit_count += SI_SBR_PATCHING_MODE;
    bit_count += SI_SBR_OVERSAMPLING_FLAG;
    bit_count += SI_SBR_OVERSAMPLING_FLAG;
  }

#ifdef SONY_PVC_ENC
  bit_count += countSbrGrid(0);
  bit_count += countSbrGrid(0);
#else
  bit_count += countSbrGrid();
  bit_count += countSbrGrid();
#endif
  bit_count += countSbrDtdf(
#ifdef SONY_PVC_ENC
                            0
#endif
                           ,bUsacIndependenceFlag);

  bit_count += countSbrDtdf(
#ifdef SONY_PVC_ENC
                            0
#endif
                           ,bUsacIndependenceFlag);


  for (band=0; band<sbrChanL->numNoiseBands; band++) {
    bit_count += SI_SBR_INVF_MODE_BITS;
  }
  for (band=0; band<sbrChanR->numNoiseBands; band++) {
    bit_count += SI_SBR_INVF_MODE_BITS;
  }

  bit_count += countSbrEnvelope(sbrChanL);
  bit_count += countSbrEnvelope(sbrChanR);

  bit_count += countSbrNoise(sbrChanL);
  bit_count += countSbrNoise(sbrChanR);

  bit_count += SI_SBR_ADD_HARMONIC_ENABLE_BITS;
  bit_count += SI_SBR_ADD_HARMONIC_ENABLE_BITS;

  if ( sbrMode == SBR_TF )
    bit_count += SI_SBR_EXTENDED_DATA_BITS;

  return bit_count;
}


/*static void writeSbrCPEdata(BsBitStream *ptrBs, */
static void writeSbrCPEdata(HANDLE_BSBITSTREAM ptrBs,
                            SBR_CHAN *sbrChanL,
                            SBR_CHAN *sbrChanR,
                            SBR_MODE sbrMode,
                            int const bUsacIndependenceFlag)
{
  int band;

  if ( sbrMode == SBR_TF )
    BsPutBit(ptrBs, 0, 1); /* no reserved bits */

  BsPutBit(ptrBs, 0, SI_SBR_COUPLING_BITS);

  if ( sbrMode == SBR_USAC_HARM ) {
    BsPutBit(ptrBs, 0, SI_SBR_PATCHING_MODE);
    BsPutBit(ptrBs, sbrChanL->sbrOversamplingFlag, SI_SBR_OVERSAMPLING_FLAG);
    if(sbrChanL->sbrPitchInBins > 0) {
      BsPutBit(ptrBs, 1, SI_SBR_PITCH_FLAG);
      BsPutBit(ptrBs, sbrChanL->sbrPitchInBins, SI_SBR_PITCH_ESTIMATE);
    } else {
      BsPutBit(ptrBs, 0, SI_SBR_PITCH_FLAG);
    }
    BsPutBit(ptrBs, 0, SI_SBR_PATCHING_MODE);
    BsPutBit(ptrBs, sbrChanR->sbrOversamplingFlag, SI_SBR_OVERSAMPLING_FLAG);
    if(sbrChanR->sbrPitchInBins > 0) {
      BsPutBit(ptrBs, 1, SI_SBR_PITCH_FLAG);
      BsPutBit(ptrBs, sbrChanR->sbrPitchInBins, SI_SBR_PITCH_ESTIMATE);
    } else {
      BsPutBit(ptrBs, 0, SI_SBR_PITCH_FLAG);
    }
  }

#ifdef SONY_PVC_ENC
  writeSbrGrid(ptrBs, 0);
  writeSbrGrid(ptrBs, 0);
  writeSbrDtdf(ptrBs, 0, bUsacIndependenceFlag);
  writeSbrDtdf(ptrBs, 0, bUsacIndependenceFlag);
#else
  writeSbrGrid(ptrBs);
  writeSbrGrid(ptrBs);
  writeSbrDtdf(ptrBs, bUsacIndependenceFlag);
  writeSbrDtdf(ptrBs, bUsacIndependenceFlag);
#endif

  for (band=0; band<sbrChanL->numNoiseBands; band++)
    BsPutBit(ptrBs, 0, SI_SBR_INVF_MODE_BITS);
  for (band=0; band<sbrChanR->numNoiseBands; band++)
    BsPutBit(ptrBs, 0, SI_SBR_INVF_MODE_BITS);

  writeSbrEnvelope(ptrBs, sbrChanL);
  writeSbrEnvelope(ptrBs, sbrChanR);

  writeSbrNoise(ptrBs, sbrChanL);
  writeSbrNoise(ptrBs, sbrChanR);

  BsPutBit(ptrBs, 0, SI_SBR_ADD_HARMONIC_ENABLE_BITS);
  BsPutBit(ptrBs, 0, SI_SBR_ADD_HARMONIC_ENABLE_BITS);

  if ( sbrMode == SBR_TF )
    BsPutBit(ptrBs, 0, SI_SBR_EXTENDED_DATA_BITS); /* no extended data */
}

static int countSbrData(int nChan, SBR_CHAN *sbrChanL, SBR_CHAN *sbrChanR, SBR_MODE sbrMode,
#ifdef SONY_PVC_ENC
                        int pvc_mode
#endif
                       ,const int bUsacIndependenceFlag
) {
  int bit_count=0;

  if (nChan==1)
    bit_count += countSbrSCEdata(sbrChanL, sbrMode,
#ifdef SONY_PVC_ENC
                                 pvc_mode
#endif
                                ,bUsacIndependenceFlag);
  else if (nChan==2)
    bit_count += countSbrCPEdata(sbrChanL, sbrChanR, sbrMode, bUsacIndependenceFlag);

  return bit_count;
}

/*static void writeSbrData(BsBitStream *ptrBs, */
static void writeSbrData(HANDLE_BSBITSTREAM ptrBs,
                         int nChan,
                         SBR_CHAN *sbrChanL,
                         SBR_CHAN *sbrChanR,
                         SBR_MODE sbrMode
#ifdef SONY_PVC_ENC
                         ,int pvc_mode
#endif
                         ,int const bUsacIndependenceFlag
                         )
{
  if (nChan==1)
    writeSbrSCEdata(ptrBs, sbrChanL, sbrMode
#ifdef SONY_PVC_ENC
                    ,pvc_mode
#endif
                    ,bUsacIndependenceFlag 
                    );
  else if (nChan==2)
    writeSbrCPEdata(ptrBs, sbrChanL, sbrChanR, sbrMode, bUsacIndependenceFlag);
}

static void SbrPitchEstimator( float *rBuffer,
                               float *iBuffer,
                               int   *sbrPitchInBins)
{

    static int i=0;
    if(!(i % 10)) {
      *sbrPitchInBins = i;
    } 
    i++;
    if(i>127) i=0;
    return;
}

static void SbrOversamplingDetector( float *rBuffer,
                                     float *iBuffer,
                                     int *sbrOversamplingFlag)
{

    static int i;
    if(i && !(i % 10)) {
        *sbrOversamplingFlag=1;
    } else {
        *sbrOversamplingFlag=0;
    }
    i++;
    return;
}

static void calcEnv(SBR_CHAN *sbrChan,
                    float *samples
#ifdef PARAMETRICSTEREO
                    , float *samplesR   /* NULL if not parametric stereo */
#endif
#ifdef SONY_PVC_ENC
                    , float *p_qmfl
                    , float *p_qmfh
#endif
                    )
{
  int i, j, k, l, li, ui, count;
  float rBuffer[64], iBuffer[64], nrgBuffer[64][64];
  int frameSize = (sbrChan->sbrRatioIndex == SBR_RATIO_INDEX_4_1)?4096:2048;
  int timeBufSize = (sbrChan->sbrRatioIndex == SBR_RATIO_INDEX_4_1)?TIME_BUF_SIZE_41:TIME_BUF_SIZE_21;
#ifdef PARAMETRICSTEREO
  SBR_CHAN *sbrChanR = sbrChan+1;
  float lrBuffer[32][64];
  float liBuffer[32][64];
  float rrBuffer[32][64];
  float riBuffer[32][64];
#endif

  /* update timeBuffer */
  for (i=0; i<timeBufSize-frameSize; i++)
    sbrChan->timeBuffer[i] = sbrChan->timeBuffer[i+frameSize];
  for (i=0; i<frameSize; i++)
    sbrChan->timeBuffer[timeBufSize-frameSize+i] = samples[i];

#ifdef PARAMETRICSTEREO
  if (samplesR) {
    sbrChan->psAvailable = 1;

    for (i=0; i<timeBufSize-2048; i++)
      sbrChanR->timeBuffer[i] = sbrChanR->timeBuffer[i+2048];
    for (i=0; i<2048; i++)
      sbrChanR->timeBuffer[timeBufSize-2048+i] = samplesR[i];

    /* filter bank */
    for (i=0; i<32; i++) {
      CalculateEncAnaFilterbank(&sbrChan->sbrFbank,
                                sbrChan->timeBuffer + i*64,
                                lrBuffer[i], liBuffer[i]);
      CalculateEncAnaFilterbank(&sbrChanR->sbrFbank,
                                sbrChanR->timeBuffer + i*64,
                                rrBuffer[i], riBuffer[i]);
      /* downmix, calc energy */
      for (j=0; j<64; j++)
        nrgBuffer[i][j] =
          (lrBuffer[i][j]+rrBuffer[i][j])*(lrBuffer[i][j]+rrBuffer[i][j])/4 +
          (liBuffer[i][j]+riBuffer[i][j])*(liBuffer[i][j]+riBuffer[i][j])/4;
    }

    /* estimate PS data */
    SbrPsEncode(&sbrChan->psData,
                lrBuffer,
                liBuffer,
                rrBuffer,
                riBuffer);
  }
  else
#endif
  {
    /* filter bank */
    for (i=0; i<(frameSize/64); i++) {
      CalculateEncAnaFilterbank(&sbrChan->sbrFbank,
                                sbrChan->timeBuffer + i*64,
                                rBuffer, iBuffer);
      /* calc energy */
      for (j=0; j<64; j++)
        nrgBuffer[i][j] = rBuffer[j] * rBuffer[j] + iBuffer[j] * iBuffer[j];
    }
#ifdef SONY_PVC_ENC
if ((p_qmfl != (float *)NULL) && (p_qmfh != (float *)NULL)) {
    if (sbrChan->hPVC.pvcParam.pvc_rate == 2) {
        for(i=0; i<16; i++){
          for (j=0; j<32; j++){
            p_qmfl[i*32+j] = (nrgBuffer[2*i][j] + nrgBuffer[2*i+1][j])/2.0f;
          }
        }
        for(i=0; i<16; i++){
          for (j=0; j<64; j++){
            p_qmfh[i*64+j] = (nrgBuffer[2*i][j] + nrgBuffer[2*i+1][j])/2.0f;
          }
        }
    } else if(sbrChan->hPVC.pvcParam.pvc_rate == 4) {
      for(i=0; i<16; i++){
        for (j=0; j<16; j++){
          p_qmfl[i*16+j] = (nrgBuffer[4*i][j] + nrgBuffer[4*i+1][j] + nrgBuffer[4*i+2][j] + nrgBuffer[4*i+3][j])/4.0f;
        }
      }
      for(i=0; i<16; i++){
        for (j=0; j<64; j++){
          p_qmfh[i*64+j] = (nrgBuffer[4*i][j] + nrgBuffer[4*i+1][j] + nrgBuffer[4*i+2][j] + nrgBuffer[4*i+3][j])/4.0f;
        }
      }
    }
 }
#endif
/* oversampling detector */
 SbrOversamplingDetector(rBuffer, iBuffer, &sbrChan->sbrOversamplingFlag);  
 
    /* pitch estimator */
    SbrPitchEstimator(rBuffer, iBuffer, &sbrChan->sbrPitchInBins);
   
  }

  /* group to bands in 2 envelopes */
  for (i=0; i<2; i++) {
    for (j=0; j<sbrChan->numBands; j++) {
      float nrg=0.0f;
      int   offset = frameSize/128;
      li = sbrChan->freqBandTable[j];
      ui = sbrChan->freqBandTable[j+1];
      count = offset * (ui-li);
      for (k=li; k<ui; k++)
        for (l=offset*i; l<offset*(i+1); l++)
          nrg += nrgBuffer[l][k];
      nrg = (float)(log(nrg/(count*64)+EPS) * ILOG2);
      nrg = (nrg < 0.0f ? 0.0f : nrg);
      sbrChan->envData[i][j] = (int)(nrg + 0.5);
    }
  }
}

/******************************************************************/

static int freqBandTableSF7[] = {
  15, 16, 17, 18, 19, 20, 22, 24, 26, 28, 30, 33, 37, 41, 45};
static int freqBandTableSF12[] = {
  22, 23, 25, 27, 29, 31, 33, 36, 39, 42, 45};

static int V_k_master[MAX_FREQ_COEFFS + 1];

void SbrInit(int bitRate, int samplingFreq, int nChan, SBR_RATIO_INDEX sbrRatioIndex, SBR_CHAN *sbrChan)
{
  int env, noise, band;
  int i;
  int lsbM, lsb, usb, Num_Master;

  int FreqBandTable[2][MAX_FREQ_COEFFS + 1];
  int NSfb[2];
  int NoNoiseBands;     
  int downSampleRatio = 2;
  int timeBufSize = 0;

  switch(sbrRatioIndex){
  case SBR_RATIO_INDEX_4_1:
    downSampleRatio = 4;
    timeBufSize = TIME_BUF_SIZE_41;
    break;
  case SBR_RATIO_INDEX_8_3:
  case SBR_RATIO_INDEX_2_1:
    downSampleRatio = 2;
    timeBufSize = TIME_BUF_SIZE_21;
    break;
  default:
    assert(0);
    break;
  }

  sbrChan->sbrRatioIndex = sbrRatioIndex;

  if (nChan==1) {
    if(bitRate >= 30000){
      sbrChan->startFreq = 7;
      sbrChan->stopFreq = 9;
    } else {
      sbrChan->startFreq = 5;
      sbrChan->stopFreq = 7;
    } 
    sbrChan->bufOffset = 214;
  }
  else {
    sbrChan->startFreq = 12;
    sbrChan->stopFreq = 9;
    
    sbrChan->bufOffset = 146;
  }

  sbrdecFindStartAndStopBand(samplingFreq,
                             sbrChan->startFreq,
                             sbrChan->stopFreq,
                             (float)downSampleRatio,
                             &lsbM, &usb);

  sbrdecUpdateFreqScale(V_k_master, &Num_Master,
                        lsbM, usb, 2,
                        1, 0,
                        (float)downSampleRatio);

  /*Derive Hiresolution from master frequency function*/
  sbrdecUpdateHiRes(FreqBandTable[1], &NSfb[1],
                    V_k_master, Num_Master,
                    0 );
  /*Derive  Loresolution from Hiresolution*/
  sbrdecUpdateLoRes(FreqBandTable[0], &NSfb[0],
                    FreqBandTable[1], NSfb[1]);

  lsb = FreqBandTable[0][0];
  usb = FreqBandTable[0][NSfb[0]];

  sbrChan->freqBandTable = V_k_master;
  sbrChan->numBands = Num_Master;
  
  NoNoiseBands = (int) ( 2 * log((double) usb / lsb) / log(2.0) + 0.5 );

  if( NoNoiseBands == 0) {
    NoNoiseBands = 1;
  }

  sbrChan->numNoiseBands = NoNoiseBands;

  InitEncAnaFilterbank(&sbrChan->sbrFbank);

  for (i=0; i<timeBufSize; i++)
    sbrChan->timeBuffer[i] = 0.0f;

  for (env=0; env<MAX_ENVELOPES; env++)
    for (band=0; band<MAX_FREQ_COEFFS; band++)
      sbrChan->envData[env][band] = 0;

  for (noise=0; noise<MAX_NOISE_ENVELOPES; noise++)
    for (band=0; band<MAX_NOISE_COEFFS; band++)
      sbrChan->noiseData[noise][band] = 0;

  sbrChan->sendHdrCnt = -1;
  sbrChan->sendHdrFrame = 10; /* send new header every 10th frame */
#ifdef SONY_PVC_ENC
  sbrChan->switchPvcFlg = 0;
#endif
  sbrChan->bs_interTes = 1;

#ifdef PARAMETRICSTEREO
  sbrChan->psAvailable = 0;
  SbrPsInit(&sbrChan->psData);
#endif
#ifdef SONY_PVC_ENC
  pvc_init_encode(&(sbrChan->hPVC));
  sbrChan->hPVC.pvcParam.pvc_rate = downSampleRatio;
#endif
}

int SbrEncodeSingleChannel_BSAC(SBR_CHAN *sbrChan,
                                SBR_MODE sbrMode,
                                float *samples,
#ifdef PARAMETRICSTEREO
                                float *samplesRight,
#endif
#ifdef SONY_PVC_ENC
                                int *pvc_mode,
#endif
                                const int bUsacIndependenceFlag
) {
  int noise, band;
  int hdrBits, dataBits, sbrBits;
  int cnt, maxCount;
#ifdef SONY_PVC_ENC
  float a_qmfl[16*32];
  float a_qmfh[16*64];
#endif

  /* calculate envelopes */
  calcEnv(sbrChan, samples
#ifdef PARAMETRICSTEREO
          , samplesRight
#endif
#ifdef SONY_PVC_ENC
          , a_qmfl
          , a_qmfh
#endif
          );

  for (noise=0; noise<2; noise++)
    for (band=0; band<sbrChan->numNoiseBands; band++)
       sbrChan->noiseData[noise][band] = 28;


  /* update counter for sending a header */
  sbrChan->sendHdrCnt++;
  if (sbrChan->sendHdrCnt==sbrChan->sendHdrFrame)
    sbrChan->sendHdrCnt=0;

#ifdef SONY_PVC_ENC
  if (!(*pvc_mode) != !sbrChan->hPVC.pvcParam.pvc_flg_prev)
  {
      sbrChan->sendHdrCnt=0;
  }
#endif

#ifdef SONY_PVC_ENC
    pvc_encode_frame(&(sbrChan->hPVC), (unsigned char)(*pvc_mode), 
                     sbrChan->hPVC.pvcParam.pvc_rate, 
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
                     (!sbrChan->sendHdrCnt || bUsacIndependenceFlag), 
                     bUsacIndependenceFlag,
#else
                     !sbrChan->sendHdrCnt,
#endif
                     a_qmfl, a_qmfh, sbrChan->freqBandTable[0], sbrChan->freqBandTable[sbrChan->numBands]-1,  &(sbrChan->hPVC.pvc_bit_count));
#endif
    /* count bits needed for fill element */
    if ( (sbrMode == SBR_USAC) || (sbrMode == SBR_USAC_HARM) ){
      int sbrInfoPresent = (sbrChan->sendHdrCnt==0)?1:0;
      int sbrHeaderPresent = (sbrChan->sendHdrCnt==0)?1:0;

      hdrBits = writeSbrHdrUsac(NULL, sbrChan, bUsacIndependenceFlag, sbrInfoPresent, sbrHeaderPresent, sbrChan->bs_pvc, *pvc_mode);
    } else {
      if (sbrChan->sendHdrCnt==0)
        hdrBits = SI_SBR_HDR_BIT + countSbrHdr(1, sbrMode);
      else
        hdrBits = SI_SBR_HDR_BIT;
    }


  dataBits = countSbrData(1, sbrChan, NULL, sbrMode,
#ifdef SONY_PVC_ENC
                          *pvc_mode
#endif
                         ,bUsacIndependenceFlag);

#ifdef BSACCONF
  hdrBits = countSbrHdr(2, sbrMode);
#endif

  /* 20060107 */
  sbrBits = /*LEN_EX_TYPE + */ hdrBits + dataBits;
  if ( sbrMode == SBR_TF ) {
    sbrChan->totalBits = sbrBits + LEN_F_CNT;
    /* byte align */
    if (sbrChan->totalBits%8 !=0)
      sbrChan->fillBits = 8-(sbrChan->totalBits%8);
    else
      sbrChan->fillBits = 0;
    sbrChan->totalBits += sbrChan->fillBits;

    /* 20060110 */
    cnt = (sbrChan->totalBits) / 8;
    maxCount = (1<<LEN_F_CNT) - 1;  /* Max count without escaping */

    if (cnt < maxCount) {
      sbrChan->count = cnt;
      sbrChan->esc_count = 0;
    }
    else {
      sbrChan->count = maxCount;
      /* 20060110 */
      cnt += 1;
      sbrChan->esc_count = cnt - sbrChan->count + 1;
    }

    if (sbrChan->esc_count) sbrChan->totalBits += LEN_BYTE;
  }
  else if ( (sbrMode == SBR_USAC) || (sbrMode == SBR_USAC_HARM) ) {
    sbrChan->totalBits = sbrBits;
  }
  else {
    
  }
  return sbrChan->totalBits;
}

int SbrEncodeChannelPair_BSAC(SBR_CHAN *sbrChanL,
                              SBR_CHAN *sbrChanR,
                              SBR_MODE sbrMode,
                              float *samplesL,
                              float *samplesR, 
                              int bUsacIndependenceFlag)
{
  int noise, band;
  int hdrBits, dataBits, sbrBits;
  int cnt, maxCount;

  /* calculate envelopes */
#ifndef PARAMETRICSTEREO
#ifdef SONY_PVC_ENC
  calcEnv(sbrChanL, samplesL, NULL, NULL);
  calcEnv(sbrChanR, samplesR, NULL, NULL);
#else
  calcEnv(sbrChanL, samplesL);
  calcEnv(sbrChanR, samplesR);
#endif
#else
#ifdef SONY_PVC_ENC
  calcEnv(sbrChanL, samplesL, NULL, NULL, NULL);
  calcEnv(sbrChanR, samplesR, NULL, NULL, NULL);
#else
  calcEnv(sbrChanL, samplesL, NULL);
  calcEnv(sbrChanR, samplesR, NULL);
#endif
#endif

  for (noise=0; noise<2; noise++)
    for (band=0; band<sbrChanL->numNoiseBands; band++) {
       sbrChanL->noiseData[noise][band] = 28;
       sbrChanR->noiseData[noise][band] = 28;
    }


  /* update counter for sending a header */
  sbrChanL->sendHdrCnt++;
  if (sbrChanL->sendHdrCnt==sbrChanL->sendHdrFrame)
    sbrChanL->sendHdrCnt=0;

  /* count bits needed for fill element */
  if ( (sbrMode == SBR_USAC) || (sbrMode == SBR_USAC_HARM) ){
    int sbrInfoPresent = (sbrChanL->sendHdrCnt==0)?1:0;
    int sbrHeaderPresent = (sbrChanL->sendHdrCnt==0)?1:0;
    
    hdrBits = writeSbrHdrUsac(NULL, sbrChanL, bUsacIndependenceFlag, sbrInfoPresent, sbrHeaderPresent, sbrChanL->bs_pvc, 0);
  } else {
    if (sbrChanL->sendHdrCnt==0)
      hdrBits = SI_SBR_HDR_BIT + countSbrHdr(2, sbrMode);
    else
      hdrBits = SI_SBR_HDR_BIT;
  }

  dataBits = countSbrData(2, sbrChanL, sbrChanR, sbrMode,
#ifdef SONY_PVC_ENC
                          0
#endif
                         ,bUsacIndependenceFlag);

#ifdef BSACCONF
  hdrBits = countSbrHdr(2, sbrMode);
#endif
/* 20060107 */
  sbrBits = /*LEN_EX_TYPE +*/ hdrBits + dataBits;
  if ( sbrMode == SBR_TF ) {
  sbrChanL->totalBits = sbrBits + LEN_F_CNT;

  /* byte align */
  if (sbrChanL->totalBits%8 !=0)
	sbrChanL->fillBits = 8-(sbrChanL->totalBits%8);
  else
	  sbrChanL->fillBits = 0;

  sbrChanL->totalBits += sbrChanL->fillBits;

  /* 20060110 */
  cnt = (sbrChanL->totalBits) / 8;
  maxCount = (1<<LEN_F_CNT) - 1;  /* Max count without escaping */

  if (cnt < maxCount) {
    sbrChanL->count = cnt;
    sbrChanL->esc_count = 0;
  }
  else {
    sbrChanL->count = maxCount;
	/* 20060110 */
	cnt += 1;
	sbrChanL->esc_count = cnt - sbrChanL->count + 1;
  }

  if (sbrChanL->esc_count) sbrChanL->totalBits += LEN_BYTE;
  }
  else if ( (sbrMode == SBR_USAC) || (sbrMode == SBR_USAC_HARM) ) {
    sbrChanL->totalBits = sbrBits;
  }

  return sbrChanL->totalBits;
}


int WriteSbrSCE(SBR_CHAN *sbrChan,
                /*BsBitStream *ptrBs)*/
                SBR_MODE sbrMode,
                HANDLE_BSBITSTREAM ptrBs
#ifdef SONY_PVC_ENC
                ,int pvc_mode
#endif
                ,int const bUsacIndependenceFlag
)
{
  if ( sbrMode == SBR_TF ) {
    BsPutBit(ptrBs, ID_FIL, LEN_SE_ID); /* Write fill_element ID */

    BsPutBit(ptrBs, sbrChan->count, LEN_F_CNT); /* count */
    if (sbrChan->esc_count) {
      BsPutBit(ptrBs, sbrChan->esc_count, LEN_BYTE); /* esc_count */
    }

    /* extension type */
    BsPutBit(ptrBs, SI_SBR_STD_EXTENSION, LEN_EX_TYPE); /* no crc */
  }

  if ( (sbrMode == SBR_USAC) || (sbrMode == SBR_USAC_HARM) ){
    int sbrInfoPresent  = (sbrChan->sendHdrCnt==0)?1:0;
    int sbrHeaderPresent = (sbrChan->sendHdrCnt==0)?1:0;

    writeSbrHdrUsac(ptrBs, sbrChan, bUsacIndependenceFlag, sbrInfoPresent, sbrHeaderPresent, sbrChan->bs_pvc
#ifdef SONY_PVC_ENC
                    ,pvc_mode
#endif        
                    );
  } else {
    if (sbrChan->sendHdrCnt==0)
      BsPutBit(ptrBs, 1, SI_SBR_HDR_BIT); /* header will follow */
    else
      BsPutBit(ptrBs, 0, SI_SBR_HDR_BIT); /* no header */

    /* header */
    if (sbrChan->sendHdrCnt==0)
      writeSbrHdr(ptrBs, sbrChan, 1, sbrMode
#ifdef SONY_PVC_ENC
                  ,pvc_mode
#endif
                  );
  }

  /* data */
  writeSbrData(ptrBs, 1, sbrChan, NULL, sbrMode
#ifdef SONY_PVC_ENC
               ,pvc_mode
#endif
               ,bUsacIndependenceFlag
               );

  /* fill bits */
  if ( sbrMode == SBR_TF)
    BsPutBit(ptrBs, 0, sbrChan->fillBits);

  return sbrChan->totalBits;
}

int WriteSbrCPE(SBR_CHAN *sbrChanL,
                SBR_CHAN *sbrChanR,
                SBR_MODE sbrMode,
                /*BsBitStream *ptrBs)*/
                HANDLE_BSBITSTREAM ptrBs,
                int const bUsacIndependenceFlag)
{
  if ( sbrMode == SBR_TF ) {
    BsPutBit(ptrBs, ID_FIL, LEN_SE_ID); /* Write fill_element ID */

    BsPutBit(ptrBs, sbrChanL->count, LEN_F_CNT); /* count */
    if (sbrChanL->esc_count) {
      BsPutBit(ptrBs, sbrChanL->esc_count, LEN_BYTE); /* esc_count */
    }

    BsPutBit(ptrBs, SI_SBR_STD_EXTENSION, LEN_EX_TYPE); /* no crc */
  }

  /* header */
  if ( (sbrMode == SBR_USAC) || (sbrMode == SBR_USAC_HARM) ){
    int sbrInfoPresent  = (sbrChanL->sendHdrCnt==0)?1:0;
    int sbrHeaderPresent = (sbrChanL->sendHdrCnt==0)?1:0;
    
    writeSbrHdrUsac(ptrBs, sbrChanL, bUsacIndependenceFlag, sbrInfoPresent, sbrHeaderPresent, sbrChanL->bs_pvc
#ifdef SONY_PVC_ENC
                    ,0
#endif        
                    );
  } else {
    if (sbrChanL->sendHdrCnt==0)
      BsPutBit(ptrBs, 1, SI_SBR_HDR_BIT); /* header will follow */
    else
      BsPutBit(ptrBs, 0, SI_SBR_HDR_BIT); /* no header */
    
    /* header */
    if (sbrChanL->sendHdrCnt==0)
      writeSbrHdr(ptrBs, sbrChanL, 2, sbrMode
#ifdef SONY_PVC_ENC
                  ,0
#endif
                  );
  }

  /* data */
  writeSbrData(ptrBs, 2, sbrChanL, sbrChanR, sbrMode
#ifdef SONY_PVC_ENC
		,0
#endif
               ,bUsacIndependenceFlag
               );

  /* fill bits */
  if ( sbrMode == SBR_TF )
    BsPutBit(ptrBs, 0, sbrChanL->fillBits);

  return sbrChanL->totalBits;
}

