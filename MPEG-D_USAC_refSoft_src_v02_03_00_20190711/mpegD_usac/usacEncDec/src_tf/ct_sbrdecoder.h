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

 $Id: ct_sbrdecoder.h,v 1.14.6.1 2012-04-19 09:15:33 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  SBR decoder frontend prototypes and definitions $Revision: 1.14.6.1 $
*/
#ifndef __SBRDECODER_H
#define __SBRDECODER_H

#ifdef SONY_PVC
#include "sony_pvcprepro.h"
#endif  /* SONY_PVC */


#define MAX_FRAME_SIZE  1024

#define MAXNRELEMENTS 1
#define MAXNRSBRCHANNELS (MAXNRELEMENTS*2)

#define MAXSBRBYTES 1024

#define EH_ERROR(x,y) 0
#define handBack(e) 0
#define WARN(x) 0
#define HANDLE_ERROR_INFO int
#define NOERROR 0

typedef enum
{
  SBRDEC_OK = 0,
  SBRDEC_NOSYNCH,
  SBRDEC_ILLEGAL_PROGRAM,
  SBRDEC_ILLEGAL_TAG,
  SBRDEC_ILLEGAL_CHN_CONFIG,
  SBRDEC_ILLEGAL_SECTION,
  SBRDEC_ILLEGAL_SCFACTORS,
  SBRDEC_ILLEGAL_PULSE_DATA,
  SBRDEC_MAIN_PROFILE_NOT_IMPLEMENTED,
  SBRDEC_GC_NOT_IMPLEMENTED,
  SBRDEC_ILLEGAL_PLUS_ELE_ID,
  SBRDEC_CREATE_ERROR,
  SBRDEC_NOT_INITIALIZED
}
SBR_ERROR;

typedef enum
{
  SBR_ID_SCE = 0,
  SBR_ID_CPE
}
SBR_ELEMENT_ID;

typedef struct
{
  int ElementID;
  int ExtensionType;
  int Payload;
  unsigned char Data[MAXSBRBYTES];
}
SBR_ELEMENT_STREAM;

typedef struct
{
  int NrElements;
  int NrElementsCore;
  SBR_ELEMENT_STREAM sbrElement[MAXNRELEMENTS];
}
SBRBITSTREAM;

typedef struct {

  unsigned int start_freq;
  unsigned int stop_freq;
  unsigned int header_extra1;
  unsigned int header_extra2;
  unsigned int freq_scale;
  unsigned int alter_scale;
  unsigned int noise_bands;
  unsigned int limiter_bands;
  unsigned int limiter_gains;
  unsigned int interpol_freq;
  unsigned int smoothing_mode;

} SBRDEC_USAC_SBR_HEADER;

typedef enum
{
  UPSAMPLING,
  SBR_ACTIVE
}
SBR_SYNC_STATE;

#ifdef SONY_PVC_DEC
typedef enum
{
  USE_UNKNOWN_SBR = 0,
  USE_ORIG_SBR,
  USE_PVC_SBR
}
SBR_TYPE_ID;
#endif /* SONY_PVC_DEC */

typedef enum
{
  SBR_RATIO_INDEX_NO_SBR = 0,
  SBR_RATIO_INDEX_4_1 = 1,
  SBR_RATIO_INDEX_8_3 = 2,
  SBR_RATIO_INDEX_2_1 = 3
}
SBR_RATIO_INDEX;

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct sbrDecoderInstance *SBRDECODER ;

  SBRDECODER openSBR (int sampleRate,
                      int bDownSampledSbr,
                      int coreCodecFrameSize,
                      SBR_RATIO_INDEX sbrRatioIndex
#ifdef AAC_ELD
                      ,int ldsbr
                      ,unsigned char *sbrHeader
#endif
                      ,int * bUseHBE
                      ,int * bs_interTes
                      ,int * bs_pvc
                      ,int   bUseHQtransposer
#ifdef HUAWEI_TFPP_DEC
                      ,int actATFPP
#endif
                      );

  SBR_ERROR
  applySBR (SBRDECODER self,
            SBRBITSTREAM * stream,
            float** timeData,
            float** timeDataOut,
            float **qmfBufferReal, /* qmfBufferReal[64][128] */
            float **qmfBufferImag, /* qmfBufferImag[64][128] */
#ifdef SBR_SCALABLE
            int maxSfbFreqLine,
#endif
            int bDownSampledSbr,
#ifdef PARAMETRICSTEREO
            int sbrEnablePS,
#endif
            int numChannels
            /* 20060107 */
            ,int	core_bandwidth
            ,int	bUsedBSAC
            );

  void closeSBR (SBRDECODER self) ;

  /* - - -  - - -  - - - */

  int   getSBRnOutputChannels  (SBRDECODER self, int nChannels);
  void  adjustGainLevels       (SBRDECODER self, int numLevels,
		                            const int bandLimits [], float gainLevels []);


  /* provide SBR library version info */
  typedef struct {
    char date[20];
    char versionNo[20];
  } SbrDecLibInfo;

  void SbrDecGetLibInfo (SbrDecLibInfo* libInfo);


  SBR_ERROR
  usac_applySBR (SBRDECODER self,
                 SBRBITSTREAM * stream,
                 float** timeData,
                 float** timeDataOut,
                 float ***qmfBufferReal, /* qmfBufferReal[2][MAX_TIME_SLOTS][64] */
                 float ***qmfBufferImag, /* qmfBufferImag[2][MAX_TIME_SLOTS][64] */
#ifdef SBR_SCALABLE
                 int maxSfbFreqLine,
#endif
                 int bDownSampledSbr,
#ifdef PARAMETRICSTEREO
                 int sbrEnablePS,
#endif
                 int stereoConfigIndex,
                 int numChannels, int el,
                 int channelOffset
                 /* 20060107 */
                 ,int  core_bandwidth
                 ,int bUsedBSAC
                 ,int *pnBitsRead
#ifdef SONY_PVC_DEC
                 ,int core_mode
                 ,int *sbr_mode
#endif /* SONY_PVC_DEC */
                 ,int const bUsacIndependenceFlag
                 );

  void initUsacDfltHeader(SBRDECODER self, 
                          SBRDEC_USAC_SBR_HEADER *pUsacDfltHeader,
                          int elIdx);


#ifdef __cplusplus
}
#endif

#endif

