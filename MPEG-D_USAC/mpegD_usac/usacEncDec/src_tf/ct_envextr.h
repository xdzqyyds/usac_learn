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

 $Id: ct_envextr.h,v 1.15 2011-08-17 12:51:52 mul Exp $

*******************************************************************************/
/*!
  \file
  \brief  Envelope extraction prototypes $Revision: 1.15 $
*/

#ifndef __CT_ENVEXTR_H
#define __CT_ENVEXTR_H

#include "ct_sbrconst.h"
#include "ct_sbrbitb.h"
#include "ct_sbrdecoder.h"
#ifdef PARAMETRICSTEREO
#include "ct_psdec.h"
#endif


#ifdef SONY_PVC_DEC
#include "sony_pvcdec.h"
#endif /* SONY_PVC_DEC */

#ifdef SONY_PVC_DEC
#define ESC_SIN_POS	31
#endif /* SONY_PVC_DEC */

typedef enum
{
  HEADER_OK,
  HEADER_RESET
}
SBR_HEADER_STATUS;

typedef enum
{
  INVF_OFF,
  INVF_LOW_LEVEL,
  INVF_MID_LEVEL,
  INVF_HIGH_LEVEL,

  INVF_NO_OVERRIDE
}
INVF_MODE;



typedef enum
{
  COUPLING_OFF,
  COUPLING_LEVEL,
  COUPLING_BAL
}
COUPLING_MODE;


typedef struct
{
  SBR_HEADER_STATUS status;      /*!< the current status of the header     */

  /* Changes in these variables indicates an error */
  int crcEnable;
  int ampResolution;

  /* Changes in these variables causes a reset of the decoder */
  int startFreq;
  int stopFreq;
  int xover_band;
  int freqScale;
  int alterScale;
  int noise_bands;               /*!< noise bands per octave, read from bitstream */

  /* Helper variable*/
  int noNoiseBands;              /*!< actual number of noise bands to read from the bitstream */

  int limiterBands;
  int limiterGains;
  int interpolFreq;
  int smoothingLength;
  int bPreFlattening;

#ifdef SONY_PVC_DEC
  int pvc_brmode;
#endif /* SONY_PVC_DEC */

}
SBR_HEADER_DATA;

typedef SBR_HEADER_DATA *HANDLE_SBR_HEADER_DATA;


typedef struct
{
  int nScaleFactors;            /* total number of scalefactors in frame */
  int nNoiseFactors;
  int crcCheckSum;
  int frameClass;
  int frameInfo[LENGTH_FRAME_INFO];
  int pvc_frameInfo[LENGTH_FRAME_INFO];
  int nSfb[2];
  int nNfb;
  int offset;
  int ampRes;
  int nNoiseFloorEnvelopes;
  int p;
  int prevEnvIsShort;

  SBR_HEADER_DATA sbr_header;


  /* dynamic control signals */
  int domain_vec1[MAX_ENVELOPES];
  int domain_vec2[MAX_ENVELOPES];


  INVF_MODE sbr_invf_mode[MAX_NUM_NOISE_VALUES];
  INVF_MODE sbr_invf_mode_prev[MAX_NUM_NOISE_VALUES];

  COUPLING_MODE coupling;               /*!< 3 possibilities: off, level, pan */

  /* envelope data */
  float iEnvelope[MAX_NUM_ENVELOPE_VALUES];
  float sbrNoiseFloorLevel[MAX_NUM_NOISE_VALUES];
  int addHarmonics[MAX_NUM_ENVELOPE_VALUES];

  float sfb_nrg_prev[MAX_FREQ_COEFFS];
  float prevNoiseLevel[MAX_NUM_NOISE_VALUES];

#ifdef SBR_SCALABLE
  int bStereoEnhancementLayerDecodablePrevE;
  int bStereoEnhancementLayerDecodablePrevQ;
#endif

  int interTesGamma[MAX_ENVELOPES];

  int numTimeSlots;

#ifdef AAC_ELD
  int ldsbr;
#endif
  int rate;
  int sbrPatchingMode;
  int sbrOversamplingFlag;
  int sbrPitchInBins;

#ifdef SONY_PVC_DEC
  int bs_sin_pos_present;
  int bs_sin_pos;

  int sin_start_for_next_top;
  int sin_len_for_next_top;

  int sin_start_for_cur_top;
  int sin_len_for_cur_top;

  int frameInfoPrev[LENGTH_FRAME_INFO];

  int PrevVarlenEnvId;
#endif /* SONY_PVC_DEC */

  int bUsacIndependenceFlag;
}
SBR_FRAME_DATA;

typedef SBR_FRAME_DATA *HANDLE_SBR_FRAME_DATA;








void sbrGetSingleChannelElement (HANDLE_SBR_FRAME_DATA hFrameData,
                                 HANDLE_BIT_BUFFER hBitBuf
#ifdef PARAMETRICSTEREO
                                 ,HANDLE_PS_DEC hParametricStereoDec
#endif
                                 );

void sbrGetSingleChannelElement_BSAC (HANDLE_SBR_FRAME_DATA hFrameData,
                                      HANDLE_BIT_BUFFER hBitBuf);

void
usac_sbrGetSingleChannelElement (HANDLE_SBR_FRAME_DATA hFrameData,
                                 HANDLE_BIT_BUFFER hBitBuf, 
                                 int bHarmonicSBR,
                                 int bs_interTes,
#ifdef SONY_PVC_DEC
                                 HANDLE_PVC_DATA hPvcData,
#endif /* SONY_PVC_DEC */										 
                                 int const bUsacIndependenceFlag
                                 );

#ifdef SONY_PVC_DEC
#endif /* SONY_PVC_DEC */

#ifdef SONY_PVC_DEC
void
usac_sbrGetSingleChannelElement_for_pvc (HANDLE_SBR_FRAME_DATA hFrameData,
                                         HANDLE_BIT_BUFFER hBitBuf, 
                                         int bHarmonicSBR,
                                         int sbr_mode,
                                         HANDLE_PVC_DATA hPvcData
                                         ,int *varlen
                                         ,const int bUsacIndependenceFlag
                                         );

void usac_pvcGetData(HANDLE_PVC_DATA hPvcData
					 ,HANDLE_BIT_BUFFER hBitBuf
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
                     ,int    indepFlag
#endif /* SONY_PVC_SUPPORT_INDEPFLAG */
					 );
#endif /* SONY_PVC_DEC */

#ifdef SBR_SCALABLE
void sbrGetChannelPairElement (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                              HANDLE_SBR_FRAME_DATA hFrameDataRight,
                              HANDLE_BIT_BUFFER hBitBufBase,
                              HANDLE_BIT_BUFFER hBitBufEnh,
                              int bStereoLayer);

void sbrGetChannelPairElement_BSAC (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                              HANDLE_SBR_FRAME_DATA hFrameDataRight,
                              HANDLE_BIT_BUFFER hBitBufBase,
                              HANDLE_BIT_BUFFER hBitBufEnh,
                              int bStereoLayer);
#else
void sbrGetChannelPairElement (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                              HANDLE_SBR_FRAME_DATA hFrameDataRight,
                              HANDLE_BIT_BUFFER hBitBuf);

void sbrGetChannelPairElement_BSAC (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                              HANDLE_SBR_FRAME_DATA hFrameDataRight,
                              HANDLE_BIT_BUFFER hBitBuf);

void
usac_sbrGetChannelPairElement (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                               HANDLE_SBR_FRAME_DATA hFrameDataRight,
                               HANDLE_BIT_BUFFER hBitBuf, 
                               int bHarmonicSBR,
                               int bs_interTes,
                               int const bUsacIndependenceFlag);

#endif

SBR_HEADER_STATUS
sbrGetHeaderData (SBR_HEADER_DATA *h_sbr_header,
                  HANDLE_BIT_BUFFER hBitBuf,
                  SBR_ELEMENT_ID id_sbr,
                  int isUsacBitstream,
                  SBR_SYNC_STATE syncState
				  );

SBR_HEADER_STATUS
sbrGetHeaderDataUsac (SBR_HEADER_DATA   *h_sbr_header,
                      SBR_HEADER_DATA   *h_sbr_dflt_header,
                      HANDLE_BIT_BUFFER  hBitBuf,
                      int                bUsacIndependencyFlag, 
                      int                bs_pvc,
                      SBR_SYNC_STATE     syncState
                      );

#endif
