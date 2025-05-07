/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:

and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
$Id: cnst.h,v 1.5 2011-03-07 22:28:10 gournape2 Exp $
*******************************************************************************/

/*--------------------------------------------------------------------------*
 *                         CNST.H                                           *
 *--------------------------------------------------------------------------*
 *       Codec constant parameters (coder and decoder)                      *
 *--------------------------------------------------------------------------*/
/* $Id: cnst.h,v 1.5 2011-03-07 22:28:10 gournape2 Exp $ */

#ifndef cnst_h
#define cnst_h

#include "options.h"  /* // do all setting in there */

#define NCOEF_F43 64

/* Codec constant assuming fs=12.8kHz (1024 frame length) */
#define L_FRAME_1024    1024
#define L_DIV_1024       256
#define NB_DIV             4          /* Number of division (20ms) per 80ms frame   */
#define NB_SUBFR_1024      4          /* Number of 5ms subframe per 20ms frame      */
#define L_SUBFR           64          /* Subframe size (5ms)                        */

/* Max size of spectral coefficients */
#define  N_MAX      L_FRAME_1024

#define LFAC_1024  (L_DIV_1024/2)           /* FAC frame length                           */

#define BPF_SFD    1                                       /* Bass postfilter delay (subframe) */

#define NB_SUBFR_SUPERFR_1024  (NB_DIV*NB_SUBFR_1024)      /* number of 5ms subframe per 80ms frame      */
#define LPD_SFD_1024           (NB_SUBFR_SUPERFR_1024/2)   /* LPD delay (subframe) */
#define SYN_SFD_1024           (LPD_SFD_1024-BPF_SFD)      /* Synthesis delay (subframe)       */  
#define BPF_DELAY_1024         (BPF_SFD*L_SUBFR)
#define SYN_DELAY_1024         (SYN_SFD_1024*L_SUBFR)

#define FDNS_NPTS_1024 64     /* FD noise shaping resolution (64=100Hz/point) */


#define L_NEXT_HIGH_RATE_1024     288
#define L_LPC0_1024               256     /* Past signal needed for LPC0 analysis       */
#define L_WINDOW_1024             448     /* 35ms window size in LP analysis            */
#define L_WINDOW_HIGH_RATE_1024   512     /* window size in LP analysis (50% overlap)   */

#define SR_MIN            6000     
#define SR_MAX            24000

#define FSCALE_DENOM      12800     /* filter into decim_split.h */
#define FAC_FSCALE_MAX    24000
#define FAC_FSCALE_MIN	   6000

#define USE_CASE_A       0
#define USE_CASE_B       1

#define L_FRAME48k   ((L_FRAME_1024/4)*15)
#define L_FRAME32k   ((L_FRAME_1024/2)*5)

#define L_FRAME_FSMAX 2*L_FRAME48k

#define L_TOTAL_HIGH_RATE	(M+L_FRAME_1024+L_NEXT_HIGH_RATE_1024)   /* Total size of speech buffer */

#define M            16      /* order of LP filter                         */

#define L_FILT       12      /* Delay of up-sampling filter                */

#define PIT_MIN_12k8	34      /* Minimum pitch lag with resolution 1/4      */
#define PIT_FR2_12k8	128     /* Minimum pitch lag with resolution 1/2      */
#define PIT_FR1_12k8	160     /* Minimum pitch lag with resolution 1        */
#define PIT_MAX_12k8	231     /* Maximum pitch lag                          */

/* For pitch predictor */
#define PIT_UP_SAMP      4
#define PIT_L_INTERPOL2  16
#define PIT_FIR_SIZE2    (PIT_UP_SAMP*PIT_L_INTERPOL2+1)

/* Maximum pitch lag for highest freq. scaling factor  */
#define PIT_MAX_MAX     (PIT_MAX_12k8 + (6*((((FAC_FSCALE_MAX*PIT_MIN_12k8)+(FSCALE_DENOM/2))/FSCALE_DENOM)-PIT_MIN_12k8)))

#define L_INTERPOL   (16+1)  /* Length of filter for interpolation         */

#define OPL_DECIM    2       /* Decimation in open-loop pitch analysis     */

#ifndef PI
#define PI           3.14159265358979323846264338327950288
#endif

#define PREEMPH_FAC  0.68f    /* preemphasis factor                         */
#define GAMMA1       0.92f    /* weighting factor (numerator)               */
#define TILT_FAC     0.68f    /* tilt factor (denominator)                  */

#define PIT_SHARP    0.85f    /* pitch sharpening factor                    */
#define TILT_CODE    0.3f     /* ACELP code preemphasis factor              */

#define L_MEANBUF    3        /* for isf recovery */

/* AMR_WB+ mode relative to AMR-WB core */
#define MODE_9k6     0
#define MODE_11k2    1
#define MODE_12k8    2
#define MODE_14k4    3
#define MODE_16k     4
#define MODE_18k4    5

#define MODE_8k0     6
#define MODE_8k8     7


/* number of bits (for core codec) per 80ms frame according to the mode */
extern const int NBITS_CORE_768[8];
extern const int NBITS_CORE_1024[8];
extern const int NBITS_CORE_AMR_WB[9];
extern const int NBITS_CORE_ACELP_PLUS[8];
extern const int NBITS_CORE_AMR_WB_ACELP_PLUS[9];
extern const int *NBITS_ACELP_PLUS[2];
#define NB_MODES 8  

/* maximum number of bits (to set buffer size of bitstream vector) */
#define  NBITS_MAX    (48*80+46)    /* define the buffer size at 32kbps */

/* number of packets per frame (4 packets of 20ms) */
#define  N_PACK_MAX   1

/* codec mode: 0=ACELP, 1=TCX20, 2=TCX40, 3=TCX80 */
#define  NBITS_MODE   (4*2)      /* 4 packets x 2 bits */
#define  NBITS_LPC    (46)       /* AMR-WB LPC quantizer */

#define  SYNC_WORD    (short)0x6b21    /* packet sync transmitted every 20ms */
#define  BIT_0        (short)0x007F
#define  BIT_1        (short)0x0081

/* AMRWB+ core parameters constants */
#define  L_TCX        L_FRAME_1024     /* Long TCX window size                     */
#define  NPRM_LPC     5                /* number of prm for absolute LPC quantizer */
#define  NPRM_RE8     (L_TCX+(L_TCX/8))

#define  NPRM_TCX80   (LFAC_1024+2+NPRM_RE8)           /* TCX 80ms */
#define  NPRM_TCX40   (LFAC_1024+2+(NPRM_RE8/2))       /* TCX 40ms */
#define  NPRM_TCX20   (LFAC_1024+2+(320 + 320/8))      /* mul: modified; TCX 20ms */

#define  NPRM_LPC_NEW 256 /* Classification + Absolute quantizer + 4 differential quantizers (inc. LPC0) */
#define  NPRM_DIV     (NPRM_TCX20)           /* buffer size = NB_DIV*NPRM_DIV */
/* number of parameters on the decoder side (AVQ use 4 bits per parameter) */
/*#define  DEC_NPRM_DIV (((24*80)/4)/NB_DIV)*/   /* set for max of 24kbps (TCX) */
#define  DEC_NPRM_DIV NPRM_DIV /*fcs: SQ in TCX (NPRM_DIV>DEC_NPRM_DIV)*/

#define L_OLD_SPEECH			L_TOTAL_PLUS-L_FRAME_1024
#define L_OLD_SPEECH_HIGH_RATE	L_TOTAL_HIGH_RATE-L_FRAME_1024

#endif /* cnst_h */
