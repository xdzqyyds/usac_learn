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
Copyright (c) ISO/IEC 2003.

 $Id: ct_psdec_ssc.h,v 1.1.1.1 2009-05-29 14:10:15 mul Exp $

*******************************************************************************/
/*!
  \file
  \brief  Sbr decoder $Revision: 1.1.1.1 $
*/
#ifndef __CT_PSDEC_H
#define __CT_PSDEC_H

#include "SSC_System.h"
#include "Bits.h"

#define DELAY         ( 1 ) 
#define INTERLEAVE    ( 2 )

#define HYBRID_FILTER_LENGTH  13
#define HYBRID_FILTER_DELAY    6
#define HANDLE_ERROR_INFO int
#define REAL 0
#define CPLX 1


#define EXTENSION_ID_PS_CODING       2
#define EXTENSION_ID_IPDOPD_CODING   0
#define PS_EXTENSION_SIZE_BITS       4
#define PS_EXTENSION_ESC_COUNT_BITS  8
#define PS_EXTENSION_ID_BITS         2


#define NO_SUB_QMF_CHANNELS         12
#define NO_SUB_QMF_CHANNELS_HI_RES  32
#define NO_QMF_CHANNELS             64
#define MAX_NUM_COL                 32
#define DECAY_CUTOFF                3
#define DECAY_CUTOFF_HI_RES         5
#define DECAY_SLOPE                 0.05
#define ALLPASS_SERIAL_TIME_CONST   7.0
#define INT_EPSILON                 1.0
#define NRG_INT_COEFF               0.75
#define NO_SERIAL_ALLPASS_LINKS     3
#define MAX_NO_PS_ENV               (4+1)   /* +1 needed for VAR_BORDER */

#define NO_HI_RES_BINS                  ( 34 )
#define NO_MID_RES_BINS                 ( 20 )
#define NO_LOW_RES_BINS                 ( 10 )

#define NO_HI_RES_IID_BINS              ( NO_HI_RES_BINS )
#define NO_HI_RES_ICC_BINS              ( NO_HI_RES_BINS )
#define NO_HI_RES_IPD_BINS              ( 17 )
#define NO_HI_RES_IPD_BINS_EST          ( NO_HI_RES_BINS )

#define NO_MID_RES_IID_BINS             ( NO_MID_RES_BINS )
#define NO_MID_RES_ICC_BINS             ( NO_MID_RES_BINS )
#define NO_MID_RES_IPD_BINS             ( 11 )
#define NO_MID_RES_IPD_BINS_EST         ( NO_MID_RES_BINS )

#define NO_LOW_RES_IID_BINS             ( NO_LOW_RES_BINS )
#define NO_LOW_RES_ICC_BINS             ( NO_LOW_RES_BINS )
#define NO_LOW_RES_IPD_BINS             ( 5 )
#define NO_LOW_RES_IPD_BINS_EST         ( NO_LOW_RES_BINS )

#define SUBQMF_GROUPS                   ( 10 )
#define QMF_GROUPS                      ( 12 )
#define IPD_QMF_GROUPS                  ( 3 )

#define SUBQMF_GROUPS_HI_RES            ( 32 )
#define QMF_GROUPS_HI_RES               ( 18 )
#define IPD_QMF_GROUPS_HI_RES           ( 1 )

#define NO_IID_GROUPS                   ( SUBQMF_GROUPS + QMF_GROUPS )
#define NO_IPD_GROUPS                   ( SUBQMF_GROUPS + IPD_QMF_GROUPS )

#define NO_IID_GROUPS_HI_RES            ( SUBQMF_GROUPS_HI_RES + QMF_GROUPS_HI_RES )
#define NO_IPD_GROUPS_HI_RES            ( SUBQMF_GROUPS_HI_RES + IPD_QMF_GROUPS_HI_RES )

#define NO_IID_STEPS                    ( 7 )  /* 1 .. +7 */
#define NO_IID_STEPS_FINE               ( 15 ) /* 1 .. +15 */
#define NO_ICC_STEPS                    ( 8 )  /* 0 .. +7 */
#define NO_IPD_STEPS                    ( 8 )  /* 1 .. +8 */
#define NO_OPD_STEPS                    ( 8 )  /* 1 .. +8 */

#define NO_IID_LEVELS                   ( 2 * NO_IID_STEPS + 1 ) /* -7 .. +7 */
#define NO_IID_LEVELS_FINE              ( 2 * NO_IID_STEPS_FINE + 1 ) /* -15 .. +15 */
#define NO_ICC_LEVELS                   ( NO_ICC_STEPS )         /*  0 .. +7 */

#define PSC_SQRT2                       ( 1.41421356237309504880 )
#define PSC_PIF                         ( ( double )SSC_PI )
#define PSC_2PIF                        ( ( double )( 2 * SSC_PI ) )
#define PSC_PI2F                        ( ( double )( SSC_PI / 2 ) )
#define PSC_SQRT2F                      ( ( double )PSC_SQRT2 )

#define MODE_BANDS20                    (0)
#define MODE_BANDS34                    (1)

/* ipd/opd range is [0, 2*pi[ with 8 representation levels [0, 8[ */
#define IPD_HALF_RANGE                  ( PSC_PIF )
#define IPD_SCALE_FACTOR                ( IPD_HALF_RANGE / NO_IPD_STEPS )
#define OPD_HALF_RANGE                  ( PSC_PIF )
#define OPD_SCALE_FACTOR                ( OPD_HALF_RANGE / NO_OPD_STEPS )
#define NEGATE_IPD_MASK                 ( 0x00001000 )

#define PHASE_SMOOTH_HIST1              ( 0.5 )
#define PHASE_SMOOTH_HIST2              ( 0.25 )
#define PHASE_HISTORY_LENGTH            ( 2 )

typedef enum {

  HYBRID_2_REAL     = 2,
  HYBRID_4_CPLX     = 4,
  HYBRID_8_CPLX     = 8,
  HYBRID_12_CPLX    = 12

} HYBRID_RES;

typedef struct
{
  int   nQmfBands;
  int   frameSizeInit;
  int   frameSize;
  int   *pResolution;
  int   qmfBufferMove;

  double *pWorkReal;         /**< Working arrays for Qmf samples. */
  double *pWorkImag;

  double **mQmfBufferReal;   /**< Stores old Qmf samples. */
  double **mQmfBufferImag;
  double **mTempReal;        /**< Temporary matrices for filter bank output. */
  double **mTempImag;

} HYBRID;

typedef HYBRID *HANDLE_HYBRID;


typedef struct
{
  int bValidFrame;
  int bPsDataAvail;            /*!< set if new data available from bitstream */

  int bEnableIid;
  int bEnableIcc;
  int bEnableExt;
  int bEnableIpdOpd;


  int freqResIid;           /*!< 0=low, 1=mid or 2=high frequency resolution for iid */
  int freqResIcc;           /*!< 0=low, 1=mid or 2=high frequency resolution for icc */
  int freqResIpd;           /*!< 0=low, 1=mid or 2=high frequency resolution for ipd */

  int usedStereoBands;      /*!< 0 = 10/20 bands, 1 = 34 bands */

  int bFineIidQ;            /*!< Use fine Iid quantisation. */
  int bUsePcaRot;           /*!< Use PCA Rotation, i.e., Rb. */

  int bFrameClass;

  int noEnv;
  int aEnvStartStop[MAX_NO_PS_ENV+1];

  int abIidDtFlag[MAX_NO_PS_ENV];         /*!< Deltacoding time/freq flag for IID, 0 => freq */
  int abIccDtFlag[MAX_NO_PS_ENV];         /*!< Deltacoding time/freq flag for ICC, 0 => freq */
  int abIpdDtFlag[MAX_NO_PS_ENV];         /*!< Deltacoding time/freq flag for IPD, 0 => freq */
  int abOpdDtFlag[MAX_NO_PS_ENV];         /*!< Deltacoding time/freq flag for OPD, 0 => freq */

  int aaIidIndex[MAX_NO_PS_ENV][NO_HI_RES_IID_BINS];    /*!< The IID index for all envelopes and all IID bins */
  int aaIccIndex[MAX_NO_PS_ENV][NO_HI_RES_ICC_BINS];    /*!< The ICC index for all envelopes and all ICC bins */

  int aaIpdIndex[MAX_NO_PS_ENV][NO_HI_RES_IPD_BINS];    /*!< The IPD index for all envelopes and all IPD bins */
  int aaOpdIndex[MAX_NO_PS_ENV][NO_HI_RES_IPD_BINS];    /*!< The OPD index for all envelopes and all IPD bins */

  int aaIidIndexMapped[MAX_NO_PS_ENV][NO_HI_RES_IID_BINS];    /*!< The mapped IID index for all envelopes and all IID bins */
  int aaIccIndexMapped[MAX_NO_PS_ENV][NO_HI_RES_ICC_BINS];    /*!< The mapped ICC index for all envelopes and all ICC bins */

  int aaIpdIndexMapped[MAX_NO_PS_ENV][NO_HI_RES_IPD_BINS];    /*!< The mapped IPD index for all envelopes and all IPD bins */
  int aaOpdIndexMapped[MAX_NO_PS_ENV][NO_HI_RES_IPD_BINS];    /*!< The mapped OPD index for all envelopes and all IPD bins */

} OCS_PARAMETERS;

typedef OCS_PARAMETERS *HANDLE_OCS_PARAMETERS;

struct PS_DEC {
  unsigned int noSubSamplesInit;
  unsigned int noSubSamples;
  unsigned int noChannels;

  int bUse34StereoBands;    /*!< current frame:  0 = 10/20 bands, 1 = 34 bands */
  int bUse34StereoBandsPrev;/*!< previous frame: 0 = 10/20 bands, 1 = 34 bands */

  int aIidPrevFrameIndex[NO_HI_RES_IID_BINS];/*!< The IID index for previous frame */
  int aIccPrevFrameIndex[NO_HI_RES_ICC_BINS]; /*!< The ICC index for previous frame */
  int aIpdPrevFrameIndex[NO_HI_RES_IPD_BINS]; /*!< The IPD index for previous frame */
  int aOpdPrevFrameIndex[NO_HI_RES_IPD_BINS]; /*!< The OPD index for previous frame */
  double aIidPrevFrameData[NO_HI_RES_IID_BINS];/*!< Previous frame IID gain for all IID bins */
  double aIccPrevFrameData[NO_HI_RES_ICC_BINS]; /*!< Previous frame ICC gain for all ICC bins */
  double aIpdPrevFrameData[NO_HI_RES_IPD_BINS]; /*!< Previous frame IPD gain for all ICC bins */

  int psMode;

  const int *pGroupBorders;          /*!< points to the 20 or 34 bands vector */
  const int *pBins2groupMap;         /*!< points to the 20 or 34 bands vector */

/*---------------------------------------*/

  unsigned int parsertarget; 
  unsigned int pipe_length;
  OCS_PARAMETERS aOcsParams[DELAY+INTERLEAVE];
  OCS_PARAMETERS prevOcsParams;
/*---------------------------------------*/

  int noBins;               /*!< hybrid filterbank configuration: 20 or 34 bins */
  int noGroups;             /*!< hybrid filterbank configuration: 22 or 50 groups */
  int noSubQmfGroups;       /*!< hybrid filterbank configuration: 10 or 32 SubQmfGroups */

  int aIpdIndexMapped_1[NO_HI_RES_IPD_BINS];    /*!< The mapped IPD index for last envelope and all IPD bins */
  int aOpdIndexMapped_1[NO_HI_RES_IPD_BINS];    /*!< The mapped OPD index for last envelope and all IPD bins */
  int aIpdIndexMapped_2[NO_HI_RES_IPD_BINS];    /*!< The mapped IPD index for second last envelope and all IPD bins */
  int aOpdIndexMapped_2[NO_HI_RES_IPD_BINS];    /*!< The mapped OPD index for second last envelope and all IPD bins */

  int   delayBufIndex;        /*!< Pointer to where the latest sample is in buffer */
  int   noSampleDelayAllpass; /*!< How many QMF samples delay is used. (allpass) */
  int   noSampleDelay;        /*!< How many QMF samples delay is used. */
  double **aaRealDelayBufferQmf;  /*!< Real part delay buffer */
  double **aaImagDelayBufferQmf;  /*!< Imaginary part delay buffer */

  int   aDelayRBufIndexSer[NO_SERIAL_ALLPASS_LINKS];
  int   aNoSampleDelayRSer[NO_SERIAL_ALLPASS_LINKS];  /*!< How many QMF samples delay is used. */

  double **aaaRealDelayRBufferSerQmf[NO_SERIAL_ALLPASS_LINKS];  /*!< Real part delay buffer */
  double **aaaImagDelayRBufferSerQmf[NO_SERIAL_ALLPASS_LINKS];  /*!< Imaginary part delay buffer */

  double *aaFractDelayPhaseFactorReQmf;
  double *aaFractDelayPhaseFactorImQmf;
  double **aaFractDelayPhaseFactorSerReQmf;
  double **aaFractDelayPhaseFactorSerImQmf;

  double **aaaRealDelayRBufferSerSubQmf[NO_SERIAL_ALLPASS_LINKS];  /*!< Real part delay buffer */
  double **aaaImagDelayRBufferSerSubQmf[NO_SERIAL_ALLPASS_LINKS];  /*!< Imaginary part delay buffer */

  double *aaFractDelayPhaseFactorReSubQmf20;
  double *aaFractDelayPhaseFactorImSubQmf20;
  double **aaFractDelayPhaseFactorSerReSubQmf20;
  double **aaFractDelayPhaseFactorSerImSubQmf20;

  double *aaFractDelayPhaseFactorReSubQmf34;
  double *aaFractDelayPhaseFactorImSubQmf34;
  double **aaFractDelayPhaseFactorSerReSubQmf34;
  double **aaFractDelayPhaseFactorSerImSubQmf34;

  double **aaRealDelayBufferSubQmf;  /*!< Real part delay buffer */
  double **aaImagDelayBufferSubQmf;  /*!< Imaginary part delay buffer */

  double **mHybridRealLeft;     /*!< Buffers holding hybrid subband samples. */
  double **mHybridImagLeft;
  double **mHybridRealRight;     
  double **mHybridImagRight;

  HANDLE_HYBRID hHybrid20;   /*!< Handle to hybrid filter bank struct 1. */
  HANDLE_HYBRID hHybrid34;   /*!< Handle to hybrid filter bank struct 2. */

  HANDLE_HYBRID pHybrid;     /*!< Handle to hybrid filter bank struct 1 or 2. */

  double PeakDecayFactorFast;
  double intFilterCoeff;

  double aPeakDecayFastBin[NO_HI_RES_BINS];
  double aPrevNrgBin[NO_HI_RES_BINS];
  double aPrevPeakDiffBin[NO_HI_RES_BINS];

  int aDelayBufIndexDelayQmf[NO_QMF_CHANNELS];
  int aNoSampleDelayDelayQmf[NO_QMF_CHANNELS];  /*!< How many QMF samples delay is used. */

  int firstDelayGr20;
  int firstDelayGr34;
  int firstDelayGr;
  int firstDelaySb;

  double h11rCurr[NO_HI_RES_BINS];
  double h12rCurr[NO_HI_RES_BINS];
  double h21rCurr[NO_HI_RES_BINS];
  double h22rCurr[NO_HI_RES_BINS];

  double h11iCurr[NO_HI_RES_BINS];
  double h12iCurr[NO_HI_RES_BINS];
  double h21iCurr[NO_HI_RES_BINS];
  double h22iCurr[NO_HI_RES_BINS];

  double h11rPrev[NO_HI_RES_BINS];
  double h12rPrev[NO_HI_RES_BINS];
  double h21rPrev[NO_HI_RES_BINS];
  double h22rPrev[NO_HI_RES_BINS];

  double h11iPrev[NO_HI_RES_BINS];
  double h12iPrev[NO_HI_RES_BINS];
  double h21iPrev[NO_HI_RES_BINS];
  double h22iPrev[NO_HI_RES_BINS];
};

typedef struct PS_DEC *HANDLE_PS_DEC;


void
HybridAnalysis ( const double **mQmfReal,
                 const double **mQmfImag,
                 double **mHybridReal,
                 double **mHybridImag,
                 HANDLE_HYBRID hHybrid,
                 int usedStereoBands);

void
HybridSynthesis ( const double **mHybridReal,  
                  const double **mHybridImag,  
                  double **mQmfReal,            
                  double **mQmfImag,           
                  HANDLE_HYBRID hHybrid );

HANDLE_ERROR_INFO
CreateHybridFilterBank ( HANDLE_HYBRID *phHybrid,
                         int frameSize,
                         int noBands,
                         const int *pResolution );

void
DeleteHybridFilterBank ( HANDLE_HYBRID *phHybrid );


unsigned int
ReadPsData (struct PS_DEC  *h_ps_d,
            SSC_BITS *hBitBuf);

void
DecodePs(struct PS_DEC *h_ps_d);


int
CreatePsDec(HANDLE_PS_DEC *h_PS_DEC,
            int fs,
            unsigned int noQmfChans,
            unsigned int noSubSamples,
            int psMode);

void ResetPsDec( HANDLE_PS_DEC pms );
void ResetPsDeCor( HANDLE_PS_DEC pms );

void
DeletePsDec(HANDLE_PS_DEC * h_ps_d_p);

void
ApplyPsFrame(HANDLE_PS_DEC h_ps_d,
             double **iBufferLeft,
             double **rBufferLeft,
             double **iIntBufferRight,
             double **rIntBufferRight);

int
initPsDec(HANDLE_PS_DEC *hParametricStereoDec, int noSubSamples);

void SetNumberOfSlots(HANDLE_PS_DEC h, unsigned int NumberOfSlots);

#endif
