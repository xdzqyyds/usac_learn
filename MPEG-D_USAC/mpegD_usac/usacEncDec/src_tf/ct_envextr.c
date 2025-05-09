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

 $Id: ct_envextr.c,v 1.17 2012-04-16 07:19:09 frd Exp $

*******************************************************************************/
/*!
  \file
  \brief  Envelope extraction $Revision: 1.17 $
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define V2_2_WARNINGS_ERRORS_FIX

#ifdef SONY_PVC
#include "sony_pvcprepro.h"
#endif  /* SONY_PVC */

#include "ct_envextr.h"


extern	int		debug[256];

#ifdef SONY_PVC_DEC
static	int	next_varlen = 0;
#endif /* SONY_PVC_DEC */



static void extractFrameInfo_LD (HANDLE_BIT_BUFFER hBitBuf,
                                HANDLE_SBR_FRAME_DATA h_frame_data);
static void extractFrameInfo (HANDLE_BIT_BUFFER hBitBuf,
                             HANDLE_SBR_FRAME_DATA h_frame_data);
static void sbrGetEnvelope (HANDLE_SBR_FRAME_DATA h_frame_data,
                            HANDLE_BIT_BUFFER hBitBuf, int channel, int bs_interTes);
static void sbrGetDirectionControlData (HANDLE_SBR_FRAME_DATA h_frame_data,
                                        HANDLE_BIT_BUFFER hBitBuf);
static void sbrGetNoiseFloorData (HANDLE_SBR_FRAME_DATA h_frame_data,
                                  HANDLE_BIT_BUFFER hBitBuf);
static void sbrCopyFrameControlData (HANDLE_SBR_FRAME_DATA hTargetFrameData,
                                     HANDLE_SBR_FRAME_DATA hSourceFrameData);

#ifdef SONY_PVC_DEC
static void extractFrameInfo_for_pvc (HANDLE_BIT_BUFFER hBitBuf,
                                      HANDLE_SBR_FRAME_DATA h_frame_data,
                                      int bs_noise_pos,
                                      int	prev_sbr_mode,
                                      int	next_varlen
                                      );

static void sbrGetDirectionControlData_for_pvc (HANDLE_SBR_FRAME_DATA h_frame_data,
                                                HANDLE_BIT_BUFFER hBitBuf,
                                                int bs_num_noise
                                                );

#endif /* SONY_PVC_DEC */


typedef int FRAME_INFO[LENGTH_FRAME_INFO];


typedef const char (*Huffman)[2];


/* Envelope Lookup Table */
/* [bs_transient_pos] = { num_envelopes, border[1], border[2], transientIdx } */
static const int LD_EnvelopeTable512[16][4] = {
  { 2, 4, -1, 0},
  { 2, 5, -1, 0},
  { 3, 2, 6, 1},
  { 3, 3, 7, 1},
  { 3, 4, 8, 1},
  { 3, 5, 9, 1},
  { 3, 6, 10, 1},
  { 3, 7, 11, 1},
  { 3, 8, 12, 1},
  { 3, 9, 13, 1},
  { 3, 10, 14, 1},
  { 2, 11, -1, 1},
  { 2, 12, -1, 1},
  { 2, 13, -1, 1},
  { 2, 14, -1, 1},
  { 2, 15, -1, 1},
};

static const int LD_EnvelopeTable480[15][4] = {
  { 2, 4, -1, 0},
  { 2, 5, -1, 0},
  { 3, 2, 6, 1},
  { 3, 3, 7, 1},
  { 3, 4, 8, 1},
  { 3, 5, 9, 1},
  { 3, 6, 10, 1},
  { 3, 7, 11, 1},
  { 3, 8, 12, 1},
  { 3, 9, 13, 1},
  { 2, 10, -1, 1},
  { 2, 11, -1, 1},
  { 2, 12, -1, 1},
  { 2, 13, -1, 1},
  { 2, 14, -1, 1},
};

/*******************************************************************************/
/* table       : envelope level, 1.5 dB                                        */
/* theor range : [-58,58], CODE_BOOK_SCF_LAV   = 58                            */
/* implem range: [-60,60], CODE_BOOK_SCF_LAV10 = 60                            */
/* raw stats   : envelopeLevel_00 (yes, wrong suffix in name)  KK 01-03-09     */
/*******************************************************************************/

/* direction: time
   raw table: HuffCode3C2FIX.m/envelopeLevel_00T_cF.mat/m_hALC_cF
   built by : FH 01-07-05 */

static const char bookSbrEnvLevel10T[120][2] = {
  {   1,   2 },    { -64, -65 },    {   3,   4 },    { -63, -66 },
  {   5,   6 },    { -62, -67 },    {   7,   8 },    { -61, -68 },
  {   9,  10 },    { -60, -69 },    {  11,  12 },    { -59, -70 },
  {  13,  14 },    { -58, -71 },    {  15,  16 },    { -57, -72 },
  {  17,  18 },    { -73, -56 },    {  19,  21 },    { -74,  20 },
  { -55, -75 },    {  22,  26 },    {  23,  24 },    { -54, -76 },
  { -77,  25 },    { -53, -78 },    {  27,  34 },    {  28,  29 },
  { -52, -79 },    {  30,  31 },    { -80, -51 },    {  32,  33 },
  { -83, -82 },    { -81, -50 },    {  35,  57 },    {  36,  40 },
  {  37,  38 },    { -88, -84 },    { -48,  39 },    { -90, -85 },
  {  41,  46 },    {  42,  43 },    { -49, -87 },    {  44,  45 },
  { -89, -86 },    {-124,-123 },    {  47,  50 },    {  48,  49 },
  {-122,-121 },    {-120,-119 },    {  51,  54 },    {  52,  53 },
  {-118,-117 },    {-116,-115 },    {  55,  56 },    {-114,-113 },
  {-112,-111 },    {  58,  89 },    {  59,  74 },    {  60,  67 },
  {  61,  64 },    {  62,  63 },    {-110,-109 },    {-108,-107 },
  {  65,  66 },    {-106,-105 },    {-104,-103 },    {  68,  71 },
  {  69,  70 },    {-102,-101 },    {-100, -99 },    {  72,  73 },
  { -98, -97 },    { -96, -95 },    {  75,  82 },    {  76,  79 },
  {  77,  78 },    { -94, -93 },    { -92, -91 },    {  80,  81 },
  { -47, -46 },    { -45, -44 },    {  83,  86 },    {  84,  85 },
  { -43, -42 },    { -41, -40 },    {  87,  88 },    { -39, -38 },
  { -37, -36 },    {  90, 105 },    {  91,  98 },    {  92,  95 },
  {  93,  94 },    { -35, -34 },    { -33, -32 },    {  96,  97 },
  { -31, -30 },    { -29, -28 },    {  99, 102 },    { 100, 101 },
  { -27, -26 },    { -25, -24 },    { 103, 104 },    { -23, -22 },
  { -21, -20 },    { 106, 113 },    { 107, 110 },    { 108, 109 },
  { -19, -18 },    { -17, -16 },    { 111, 112 },    { -15, -14 },
  { -13, -12 },    { 114, 117 },    { 115, 116 },    { -11, -10 },
  {  -9,  -8 },    { 118, 119 },    {  -7,  -6 },    {  -5,  -4 }
};

/* direction: freq
   raw table: HuffCode3C2FIX.m/envelopeLevel_00F_cF.mat/m_hALC_cF
   built by : FH 01-07-05 */

static const char bookSbrEnvLevel10F[120][2] = {
  {   1,   2 },    { -64, -65 },    {   3,   4 },    { -63, -66 },
  {   5,   6 },    { -67, -62 },    {   7,   8 },    { -68, -61 },
  {   9,  10 },    { -69, -60 },    {  11,  13 },    { -70,  12 },
  { -59, -71 },    {  14,  16 },    { -58,  15 },    { -72, -57 },
  {  17,  19 },    { -73,  18 },    { -56, -74 },    {  20,  23 },
  {  21,  22 },    { -55, -75 },    { -54, -53 },    {  24,  27 },
  {  25,  26 },    { -76, -52 },    { -77, -51 },    {  28,  31 },
  {  29,  30 },    { -50, -78 },    { -79, -49 },    {  32,  36 },
  {  33,  34 },    { -48, -47 },    { -80,  35 },    { -81, -82 },
  {  37,  47 },    {  38,  41 },    {  39,  40 },    { -83, -46 },
  { -45, -84 },    {  42,  44 },    { -85,  43 },    { -44, -43 },
  {  45,  46 },    { -88, -87 },    { -86, -90 },    {  48,  66 },
  {  49,  56 },    {  50,  53 },    {  51,  52 },    { -92, -42 },
  { -41, -39 },    {  54,  55 },    {-105, -89 },    { -38, -37 },
  {  57,  60 },    {  58,  59 },    { -94, -91 },    { -40, -36 },
  {  61,  63 },    { -20,  62 },    {-115,-110 },    {  64,  65 },
  {-108,-107 },    {-101, -97 },    {  67,  89 },    {  68,  75 },
  {  69,  72 },    {  70,  71 },    { -95, -93 },    { -34, -27 },
  {  73,  74 },    { -22, -17 },    { -16,-124 },    {  76,  82 },
  {  77,  79 },    {-123,  78 },    {-122,-121 },    {  80,  81 },
  {-120,-119 },    {-118,-117 },    {  83,  86 },    {  84,  85 },
  {-116,-114 },    {-113,-112 },    {  87,  88 },    {-111,-109 },
  {-106,-104 },    {  90, 105 },    {  91,  98 },    {  92,  95 },
  {  93,  94 },    {-103,-102 },    {-100, -99 },    {  96,  97 },
  { -98, -96 },    { -35, -33 },    {  99, 102 },    { 100, 101 },
  { -32, -31 },    { -30, -29 },    { 103, 104 },    { -28, -26 },
  { -25, -24 },    { 106, 113 },    { 107, 110 },    { 108, 109 },
  { -23, -21 },    { -19, -18 },    { 111, 112 },    { -15, -14 },
  { -13, -12 },    { 114, 117 },    { 115, 116 },    { -11, -10 },
  {  -9,  -8 },    { 118, 119 },    {  -7,  -6 },    {  -5,  -4 }
};

/*******************************************************************************/
/* table       : envelope balance, 1.5 dB                                      */
/* theor range : [-48,48], CODE_BOOK_SCF_LAV = 48                              */
/* implem range: same but mapped to [-24,24], CODE_BOOK_SCF_LAV_BALANCE10 = 24 */
/* raw stats   : envelopePan_00 (yes, wrong suffix in name)  KK 01-03-09       */
/*******************************************************************************/

/* direction: time
   raw table: HuffCode3C.m/envelopePan_00T.mat/v_hALB
   built by : FH 01-05-15 */

static const char bookSbrEnvBalance10T[48][2] = {
  { -64,   1 },    { -63,   2 },    { -65,   3 },    { -62,   4 },
  { -66,   5 },    { -61,   6 },    { -67,   7 },    { -60,   8 },
  { -68,   9 },    {  10,  11 },    { -69, -59 },    {  12,  13 },
  { -70, -58 },    {  14,  28 },    {  15,  21 },    {  16,  18 },
  { -57,  17 },    { -71, -56 },    {  19,  20 },    { -88, -87 },
  { -86, -85 },    {  22,  25 },    {  23,  24 },    { -84, -83 },
  { -82, -81 },    {  26,  27 },    { -80, -79 },    { -78, -77 },
  {  29,  36 },    {  30,  33 },    {  31,  32 },    { -76, -75 },
  { -74, -73 },    {  34,  35 },    { -72, -55 },    { -54, -53 },
  {  37,  41 },    {  38,  39 },    { -52, -51 },    { -50,  40 },
  { -49, -48 },    {  42,  45 },    {  43,  44 },    { -47, -46 },
  { -45, -44 },    {  46,  47 },    { -43, -42 },    { -41, -40 }
};

/* direction: freq
   raw table: HuffCode3C.m/envelopePan_00T.mat/v_hALB
   built by : FH 01-05-15 */

static const char bookSbrEnvBalance10F[48][2] = {
  { -64,   1 },    { -65,   2 },    { -63,   3 },    { -66,   4 },
  { -62,   5 },    { -61,   6 },    { -67,   7 },    { -68,   8 },
  { -60,   9 },    {  10,  11 },    { -69, -59 },    { -70,  12 },
  { -58,  13 },    {  14,  17 },    { -71,  15 },    { -57,  16 },
  { -56, -73 },    {  18,  32 },    {  19,  25 },    {  20,  22 },
  { -72,  21 },    { -88, -87 },    {  23,  24 },    { -86, -85 },
  { -84, -83 },    {  26,  29 },    {  27,  28 },    { -82, -81 },
  { -80, -79 },    {  30,  31 },    { -78, -77 },    { -76, -75 },
  {  33,  40 },    {  34,  37 },    {  35,  36 },    { -74, -55 },
  { -54, -53 },    {  38,  39 },    { -52, -51 },    { -50, -49 },
  {  41,  44 },    {  42,  43 },    { -48, -47 },    { -46, -45 },
  {  45,  46 },    { -44, -43 },    { -42,  47 },    { -41, -40 }
};

/*******************************************************************************/
/* table       : envelope level, 3.0 dB                                        */
/* theor range : [-29,29], CODE_BOOK_SCF_LAV   = 29                            */
/* implem range: [-31,31], CODE_BOOK_SCF_LAV11 = 31                            */
/* raw stats   : envelopeLevel_11  KK 00-02-03                                 */
/*******************************************************************************/

/* direction: time
   raw table: HuffCode2.m
   built by : FH 00-02-04 */

static const char bookSbrEnvLevel11T[62][2] = {
  { -64,   1 },    { -65,   2 },    { -63,   3 },    { -66,   4 },
  { -62,   5 },    { -67,   6 },    { -61,   7 },    { -68,   8 },
  { -60,   9 },    {  10,  11 },    { -69, -59 },    {  12,  14 },
  { -70,  13 },    { -71, -58 },    {  15,  18 },    {  16,  17 },
  { -72, -57 },    { -73, -74 },    {  19,  22 },    { -56,  20 },
  { -55,  21 },    { -54, -77 },    {  23,  31 },    {  24,  25 },
  { -75, -76 },    {  26,  27 },    { -78, -53 },    {  28,  29 },
  { -52, -95 },    { -94,  30 },    { -93, -92 },    {  32,  47 },
  {  33,  40 },    {  34,  37 },    {  35,  36 },    { -91, -90 },
  { -89, -88 },    {  38,  39 },    { -87, -86 },    { -85, -84 },
  {  41,  44 },    {  42,  43 },    { -83, -82 },    { -81, -80 },
  {  45,  46 },    { -79, -51 },    { -50, -49 },    {  48,  55 },
  {  49,  52 },    {  50,  51 },    { -48, -47 },    { -46, -45 },
  {  53,  54 },    { -44, -43 },    { -42, -41 },    {  56,  59 },
  {  57,  58 },    { -40, -39 },    { -38, -37 },    {  60,  61 },
  { -36, -35 },    { -34, -33 }
};

/* direction: freq
   raw table: HuffCode2.m
   built by : FH 00-02-04 */

static const char bookSbrEnvLevel11F[62][2] = {
  { -64,   1 },    { -65,   2 },    { -63,   3 },    { -66,   4 },
  { -62,   5 },    { -67,   6 },    {   7,   8 },    { -61, -68 },
  {   9,  10 },    { -60, -69 },    {  11,  12 },    { -59, -70 },
  {  13,  14 },    { -58, -71 },    {  15,  16 },    { -57, -72 },
  {  17,  19 },    { -56,  18 },    { -55, -73 },    {  20,  24 },
  {  21,  22 },    { -74, -54 },    { -53,  23 },    { -75, -76 },
  {  25,  30 },    {  26,  27 },    { -52, -51 },    {  28,  29 },
  { -77, -79 },    { -50, -49 },    {  31,  39 },    {  32,  35 },
  {  33,  34 },    { -78, -46 },    { -82, -88 },    {  36,  37 },
  { -83, -48 },    { -47,  38 },    { -86, -85 },    {  40,  47 },
  {  41,  44 },    {  42,  43 },    { -80, -44 },    { -43, -42 },
  {  45,  46 },    { -39, -87 },    { -84, -40 },    {  48,  55 },
  {  49,  52 },    {  50,  51 },    { -95, -94 },    { -93, -92 },
  {  53,  54 },    { -91, -90 },    { -89, -81 },    {  56,  59 },
  {  57,  58 },    { -45, -41 },    { -38, -37 },    {  60,  61 },
  { -36, -35 },    { -34, -33 }
};

/*******************************************************************************/
/* table       : envelope balance, 3.0 dB                                      */
/* theor range : [-24,24], CODE_BOOK_SCF_LAV = 24                              */
/* implem range: same but mapped to [-12,12], CODE_BOOK_SCF_LAV_BALANCE11 = 12 */
/* raw stats   : envelopeBalance_11  KK 00-02-03                               */
/*******************************************************************************/

/* direction: time
   raw table: HuffCode3C.m/envelopeBalance_11T.mat/v_hALB
   built by : FH 01-05-15 */

static const char bookSbrEnvBalance11T[24][2] = {
  { -64,   1 },    { -63,   2 },    { -65,   3 },    { -66,   4 },
  { -62,   5 },    { -61,   6 },    { -67,   7 },    { -68,   8 },
  { -60,   9 },    {  10,  16 },    {  11,  13 },    { -69,  12 },
  { -76, -75 },    {  14,  15 },    { -74, -73 },    { -72, -71 },
  {  17,  20 },    {  18,  19 },    { -70, -59 },    { -58, -57 },
  {  21,  22 },    { -56, -55 },    { -54,  23 },    { -53, -52 }
};

/* direction: time (?)
   raw table: HuffCode3C.m/envelopeBalance_11T.mat/v_hALB
   built by : FH 01-05-15 */

static const char bookSbrEnvBalance11F[24][2] = {
  { -64,   1 },    { -65,   2 },    { -63,   3 },    { -66,   4 },
  { -62,   5 },    { -61,   6 },    { -67,   7 },    { -68,   8 },
  { -60,   9 },    {  10,  13 },    { -69,  11 },    { -59,  12 },
  { -58, -76 },    {  14,  17 },    {  15,  16 },    { -75, -74 },
  { -73, -72 },    {  18,  21 },    {  19,  20 },    { -71, -70 },
  { -57, -56 },    {  22,  23 },    { -55, -54 },    { -53, -52 }
};

/*******************************************************************************/
/* table       : noise level, 3.0 dB                                           */
/* theor range : [-29,29], CODE_BOOK_SCF_LAV   = 29                            */
/* implem range: [-31,31], CODE_BOOK_SCF_LAV11 = 31                            */
/* raw stats   : noiseLevel_11  KK 00-02-03                                    */
/*******************************************************************************/

/* direction: time
   raw table: HuffCode2.m
   built by : FH 00-02-04 */

static const char bookSbrNoiseLevel11T[62][2] = {
  { -64,   1 },    { -63,   2 },    { -65,   3 },    { -66,   4 },
  { -62,   5 },    { -67,   6 },    {   7,   8 },    { -61, -68 },
  {   9,  30 },    {  10,  15 },    { -60,  11 },    { -69,  12 },
  {  13,  14 },    { -59, -53 },    { -95, -94 },    {  16,  23 },
  {  17,  20 },    {  18,  19 },    { -93, -92 },    { -91, -90 },
  {  21,  22 },    { -89, -88 },    { -87, -86 },    {  24,  27 },
  {  25,  26 },    { -85, -84 },    { -83, -82 },    {  28,  29 },
  { -81, -80 },    { -79, -78 },    {  31,  46 },    {  32,  39 },
  {  33,  36 },    {  34,  35 },    { -77, -76 },    { -75, -74 },
  {  37,  38 },    { -73, -72 },    { -71, -70 },    {  40,  43 },
  {  41,  42 },    { -58, -57 },    { -56, -55 },    {  44,  45 },
  { -54, -52 },    { -51, -50 },    {  47,  54 },    {  48,  51 },
  {  49,  50 },    { -49, -48 },    { -47, -46 },    {  52,  53 },
  { -45, -44 },    { -43, -42 },    {  55,  58 },    {  56,  57 },
  { -41, -40 },    { -39, -38 },    {  59,  60 },    { -37, -36 },
  { -35,  61 },    { -34, -33 }
};

/*******************************************************************************/
/* table       : noise balance, 3.0 dB                                         */
/* theor range : [-24,24], CODE_BOOK_SCF_LAV = 24                              */
/* implem range: same but mapped to [-12,12], CODE_BOOK_SCF_LAV_BALANCE11 = 12 */
/* raw stats   : noiseBalance_11  KK 00-02-03                                  */
/*******************************************************************************/

/* direction: time
   raw table: HuffCode3C.m/noiseBalance_11.mat/v_hALB
   built by : FH 01-05-15 */

static const char bookSbrNoiseBalance11T[24][2] = {
  { -64,   1 },    { -65,   2 },    { -63,   3 },    {   4,   9 },
  { -66,   5 },    { -62,   6 },    {   7,   8 },    { -76, -75 },
  { -74, -73 },    {  10,  17 },    {  11,  14 },    {  12,  13 },
  { -72, -71 },    { -70, -69 },    {  15,  16 },    { -68, -67 },
  { -61, -60 },    {  18,  21 },    {  19,  20 },    { -59, -58 },
  { -57, -56 },    {  22,  23 },    { -55, -54 },    { -53, -52 }
};


/*!
  \brief     Decodes one huffman code word

  Reads bits from the bitstream until a valid codeword is found.
  The table entries are interpreted either as index to the next entry
  or - if negative - as the codeword.

  \return    decoded value

*/
static int
decode_huff_cw (Huffman h,                   /*!< pointer to huffman codebook table */
                HANDLE_BIT_BUFFER hBitBuf)   /*!< Handle to Bitbuffer */
{
  int bit, index = 0;

  while (index >= 0) {
    bit = BufGetBits (hBitBuf, 1);
    index = h[index][bit];
  }
  return( index+64 ); /* Add offset */
}







#ifdef SBR_SCALABLE

static const float SBR_ENERGY_PAN_OFFSET = 12.0;

/***************************************************************************/
/*!

  \brief  Sets control of enhancement layer

  \return none

****************************************************************************/
static void
sbr_dtdf_no_enhancement_layerE(HANDLE_SBR_FRAME_DATA hFrameDataRight)
{

  memset(hFrameDataRight->domain_vec1,FREQ,hFrameDataRight->frameInfo[0]*sizeof(int));
}


/***************************************************************************/
/*!

  \brief  Sets control of enhancement layer

  \return none

****************************************************************************/
static void
sbr_dtdf_no_enhancement_layerQ(HANDLE_SBR_FRAME_DATA hFrameDataRight)
{

  hFrameDataRight->nNoiseFloorEnvelopes = hFrameDataRight->frameInfo[0]>1 ? 2 : 1;
  memset(hFrameDataRight->domain_vec2,FREQ,hFrameDataRight->nNoiseFloorEnvelopes*sizeof(int));

}





/***************************************************************************/
/*!

  \brief  Copies control data from core layer to enhancement layer,
          and sets useful deafult values.

  \return none

****************************************************************************/
static void
sbr_envelope_no_enhancement_layer(const HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                                  HANDLE_SBR_FRAME_DATA hFrameDataRight)
{
  int offset, env, band;

  hFrameDataRight->ampRes        = hFrameDataLeft->ampRes;
  hFrameDataRight->nScaleFactors = hFrameDataLeft->nScaleFactors;
  hFrameDataRight->nSfb[0]       = hFrameDataLeft->nSfb[0];
  hFrameDataRight->nSfb[1]       = hFrameDataLeft->nSfb[1];
  offset = 0;

  for(env = 0; env < hFrameDataRight->frameInfo[0]; env++){
    int freqRes = hFrameDataRight->frameInfo[hFrameDataRight->frameInfo[0] + 2 + env];

    hFrameDataRight->iEnvelope[0 + offset] = (hFrameDataRight->ampRes) ? SBR_ENERGY_PAN_OFFSET : 2*SBR_ENERGY_PAN_OFFSET;

    for(band = 1; band < hFrameDataRight->nSfb[freqRes]; band++){
      hFrameDataRight->iEnvelope[band + offset] = 0;
    }
    offset += band;
  }
}



/***************************************************************************/
/*!

  \brief  Copies control data from core layer to enhancement layer,
          and sets useful deafult values.

  \return none

****************************************************************************/
static void
sbr_noise_no_enhancement_layer(const HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                               HANDLE_SBR_FRAME_DATA hFrameDataRight)
{
  int offset, env, band;

  hFrameDataRight->nNfb = hFrameDataLeft->nNfb;
  hFrameDataRight->nNoiseFactors = hFrameDataLeft->nNfb*hFrameDataRight->nNoiseFloorEnvelopes;

  offset = 0;

  for(env = 0; env < hFrameDataRight->nNoiseFloorEnvelopes; env++){

    hFrameDataRight->sbrNoiseFloorLevel[0 + offset] = SBR_ENERGY_PAN_OFFSET;

    for(band = 1; band < hFrameDataRight->nNfb; band++){
      hFrameDataRight->sbrNoiseFloorLevel[band + offset] = 0;
    }
    offset += band;
  }

}


/***************************************************************************/
/*!

  \brief  Copies control data from core layer to enhancement layer,
          and sets useful deafult values.

  \return none

****************************************************************************/
static void
sbr_additional_data_no_enhancement_layer(const HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                                         HANDLE_SBR_FRAME_DATA hFrameDataRight)
{
  int band;

  for(band = 0; band < hFrameDataRight->nSfb[1]; band++){
    hFrameDataRight->addHarmonics[band] = hFrameDataLeft->addHarmonics[band];
  }
}

#endif





static void 
sbrParseInfoUsac (SBR_HEADER_DATA   *h_sbr_header,
                  HANDLE_BIT_BUFFER  hBitBuf, 
                  int bs_pvc){

  h_sbr_header->ampResolution   = BufGetBits (hBitBuf, SI_SBR_AMP_RES_BITS);
  
  h_sbr_header->xover_band      = BufGetBits (hBitBuf, SI_SBR_XOVER_BAND_BITS_USAC);

  h_sbr_header->bPreFlattening  = BufGetBits (hBitBuf, SI_SBR_PRE_FLAT_BITS);

  if(bs_pvc){
    h_sbr_header->pvc_brmode =  BufGetBits (hBitBuf, SI_PVC_MODE_BITS);
  } else {
    h_sbr_header->pvc_brmode = 0;
  }

  return;
}


static void 
sbrParseHeaderUsac(SBR_HEADER_DATA   *h_sbr_header,
                   HANDLE_BIT_BUFFER  hBitBuf){

  int headerExtra1, headerExtra2;
  
  /* Read new header from bitstream */
  h_sbr_header->startFreq       = BufGetBits (hBitBuf, SI_SBR_START_FREQ_BITS);
  h_sbr_header->stopFreq        = BufGetBits (hBitBuf, SI_SBR_STOP_FREQ_BITS);

  headerExtra1    = BufGetBits (hBitBuf, SI_SBR_HEADER_EXTRA_1_BITS);
  headerExtra2    = BufGetBits (hBitBuf, SI_SBR_HEADER_EXTRA_2_BITS);

  /* handle extra header information */
  if (headerExtra1) {
    h_sbr_header->freqScale   = BufGetBits (hBitBuf, SI_SBR_FREQ_SCALE_BITS);
    h_sbr_header->alterScale  = BufGetBits (hBitBuf, SI_SBR_ALTER_SCALE_BITS);
    h_sbr_header->noise_bands = BufGetBits (hBitBuf, SI_SBR_NOISE_BANDS_BITS);
  }
  else{ /* Set default values.*/
    h_sbr_header->freqScale   = SBR_FREQ_SCALE_DEFAULT;
    h_sbr_header->alterScale  = SBR_ALTER_SCALE_DEFAULT;
    h_sbr_header->noise_bands = SBR_NOISE_BANDS_DEFAULT;
  }

  if (headerExtra2) {
    h_sbr_header->limiterBands    = BufGetBits (hBitBuf, SI_SBR_LIMITER_BANDS_BITS);
    h_sbr_header->limiterGains    = BufGetBits (hBitBuf, SI_SBR_LIMITER_GAINS_BITS);
    h_sbr_header->interpolFreq    = BufGetBits (hBitBuf, SI_SBR_INTERPOL_FREQ_BITS);
    h_sbr_header->smoothingLength = BufGetBits (hBitBuf, SI_SBR_SMOOTHING_LENGTH_BITS);
  }
  else{ /* Set default values.*/
    h_sbr_header->limiterBands    = SBR_LIMITER_BANDS_DEFAULT;
    h_sbr_header->limiterGains    = SBR_LIMITER_GAINS_DEFAULT;
    h_sbr_header->interpolFreq    = SBR_INTERPOL_FREQ_DEFAULT;
    h_sbr_header->smoothingLength = SBR_SMOOTHING_LENGTH_DEFAULT;
  }
  
  return;
}

static void 
sbrAssignDfltHeaderUsac (SBR_HEADER_DATA   *h_sbr_header,
                         SBR_HEADER_DATA   *h_sbr_dflt_header
) {
  h_sbr_header->startFreq     = h_sbr_dflt_header->startFreq;
  h_sbr_header->stopFreq      = h_sbr_dflt_header->stopFreq;

  /* header_extra_1 */
  h_sbr_header->freqScale   = h_sbr_dflt_header->freqScale;
  h_sbr_header->alterScale  = h_sbr_dflt_header->alterScale;
  h_sbr_header->noise_bands = h_sbr_dflt_header->noise_bands;

  /* header_extra_2 */
  h_sbr_header->limiterBands    = h_sbr_dflt_header->limiterBands;
  h_sbr_header->limiterGains    = h_sbr_dflt_header->limiterGains;
  h_sbr_header->interpolFreq    = h_sbr_dflt_header->interpolFreq;
  h_sbr_header->smoothingLength = h_sbr_dflt_header->smoothingLength;

  return;
}

/*******************************************************************************
 Functionname:  sbrGetHeaderDataUsac
 *******************************************************************************

 Description:   Reads header data from bitstream

 Arguments:     h_sbr_header - handle to struct SBR_HEADER_DATA
                hBitBuf      - handle to struct BIT_BUFFER
                id_sbr       - SBR_ELEMENT_ID

 Return:        error status - 0 if ok

*******************************************************************************/
SBR_HEADER_STATUS
sbrGetHeaderDataUsac (SBR_HEADER_DATA   *h_sbr_header,
                      SBR_HEADER_DATA   *h_sbr_dflt_header,
                      HANDLE_BIT_BUFFER  hBitBuf,
                      int                bUsacIndependencyFlag, 
                      int                bs_pvc,
                      SBR_SYNC_STATE     syncState
                      )
{
  SBR_HEADER_DATA lastHeader;
  int sbrInfoPresent = 0, sbrHeaderPresent = 0;
  
  /* Copy header to temporary header */
  if(syncState == SBR_ACTIVE){
    memcpy (&lastHeader, h_sbr_header, sizeof(SBR_HEADER_DATA));
  }

  if(bUsacIndependencyFlag){
    sbrInfoPresent   = 1;
    sbrHeaderPresent = 1;
  } else {
    sbrInfoPresent = BufGetBits (hBitBuf, 1);
    if(sbrInfoPresent){
      sbrHeaderPresent = BufGetBits(hBitBuf, 1);
    } else {
      sbrHeaderPresent = 0;
    }
  }

  if(sbrInfoPresent){
    sbrParseInfoUsac (h_sbr_header,
                      hBitBuf, 
                      bs_pvc);
  }

  if(sbrHeaderPresent){
    int sbrUseDfltHeader = BufGetBits(hBitBuf, 1);
    if(sbrUseDfltHeader){
      sbrAssignDfltHeaderUsac (h_sbr_header, h_sbr_dflt_header);
    } else {
      sbrParseHeaderUsac(h_sbr_header, 
                         hBitBuf);
    }
  }

  if(syncState == SBR_ACTIVE){
    h_sbr_header->status = HEADER_OK;

    /* look for new settings */
    if(lastHeader.startFreq   != h_sbr_header->startFreq   ||
       lastHeader.stopFreq    != h_sbr_header->stopFreq    ||
       lastHeader.xover_band  != h_sbr_header->xover_band  ||
       lastHeader.freqScale   != h_sbr_header->freqScale   ||
       lastHeader.alterScale  != h_sbr_header->alterScale  ||
       lastHeader.noise_bands != h_sbr_header->noise_bands) {
      h_sbr_header->status = HEADER_RESET;
    }
  }
  else{
    h_sbr_header->status = HEADER_RESET;
  }

  return h_sbr_header->status;
}


/*******************************************************************************
 Functionname:  sbrGetHeaderData
 *******************************************************************************

 Description:   Reads header data from bitstream

 Arguments:     h_sbr_header - handle to struct SBR_HEADER_DATA
                hBitBuf      - handle to struct BIT_BUFFER
                id_sbr       - SBR_ELEMENT_ID

 Return:        error status - 0 if ok

*******************************************************************************/
SBR_HEADER_STATUS
sbrGetHeaderData (SBR_HEADER_DATA   *h_sbr_header,
                  HANDLE_BIT_BUFFER  hBitBuf,
                  SBR_ELEMENT_ID     id_sbr,
                  int                isUsacBitstream,
                  SBR_SYNC_STATE     syncState
				  )
{
  SBR_HEADER_DATA lastHeader;
  int headerExtra1, headerExtra2;

#ifdef SONY_PVC_DEC
  int headerExtra3;
#endif /* SONY_PVC_DEC */

  /* Copy header to temporary header */
  if(syncState == SBR_ACTIVE){
    memcpy (&lastHeader, h_sbr_header, sizeof(SBR_HEADER_DATA));
  }


  /* Read new header from bitstream */
  h_sbr_header->ampResolution   = BufGetBits (hBitBuf, SI_SBR_AMP_RES_BITS);
  h_sbr_header->startFreq       = BufGetBits (hBitBuf, SI_SBR_START_FREQ_BITS);
  h_sbr_header->stopFreq        = BufGetBits (hBitBuf, SI_SBR_STOP_FREQ_BITS);
  if (isUsacBitstream!=0) {
    h_sbr_header->xover_band      = BufGetBits (hBitBuf, SI_SBR_XOVER_BAND_BITS_USAC);
  } else {
    h_sbr_header->xover_band      = BufGetBits (hBitBuf, SI_SBR_XOVER_BAND_BITS);
  }
  h_sbr_header->bPreFlattening = BufGetBits (hBitBuf, SI_SBR_PRE_FLAT_BITS);

  BufGetBits (hBitBuf, SI_SBR_RESERVED_BITS_HDR);
  headerExtra1    = BufGetBits (hBitBuf, SI_SBR_HEADER_EXTRA_1_BITS);
  headerExtra2    = BufGetBits (hBitBuf, SI_SBR_HEADER_EXTRA_2_BITS);

#ifdef SONY_PVC_DEC
  headerExtra3    = BufGetBits (hBitBuf,  SI_SBR_HEADER_EXTRA_3_BITS);
#endif /* SONY_PVC_DEC */

  /* handle extra header information */
  if (headerExtra1) {
    h_sbr_header->freqScale   = BufGetBits (hBitBuf, SI_SBR_FREQ_SCALE_BITS);
    h_sbr_header->alterScale  = BufGetBits (hBitBuf, SI_SBR_ALTER_SCALE_BITS);
    h_sbr_header->noise_bands = BufGetBits (hBitBuf, SI_SBR_NOISE_BANDS_BITS);
  }
  else{ /* Set default values.*/
    h_sbr_header->freqScale   = SBR_FREQ_SCALE_DEFAULT;
    h_sbr_header->alterScale  = SBR_ALTER_SCALE_DEFAULT;
    h_sbr_header->noise_bands = SBR_NOISE_BANDS_DEFAULT;
  }

  if (headerExtra2) {
    h_sbr_header->limiterBands    = BufGetBits (hBitBuf, SI_SBR_LIMITER_BANDS_BITS);
    h_sbr_header->limiterGains    = BufGetBits (hBitBuf, SI_SBR_LIMITER_GAINS_BITS);
    h_sbr_header->interpolFreq    = BufGetBits (hBitBuf, SI_SBR_INTERPOL_FREQ_BITS);
    h_sbr_header->smoothingLength = BufGetBits (hBitBuf, SI_SBR_SMOOTHING_LENGTH_BITS);
  }
  else{ /* Set default values.*/
    h_sbr_header->limiterBands    = SBR_LIMITER_BANDS_DEFAULT;
    h_sbr_header->limiterGains    = SBR_LIMITER_GAINS_DEFAULT;
    h_sbr_header->interpolFreq    = SBR_INTERPOL_FREQ_DEFAULT;
    h_sbr_header->smoothingLength = SBR_SMOOTHING_LENGTH_DEFAULT;
  }

#ifdef SONY_PVC_DEC
   if(headerExtra3){
     h_sbr_header->pvc_brmode =  BufGetBits (hBitBuf, SI_PVC_MODE_BITS);
   }else{
	 h_sbr_header->pvc_brmode = 0;
   }
#endif /* SONY_PVC_DEC */



  if(syncState == SBR_ACTIVE){
    h_sbr_header->status = HEADER_OK;

    /* look for new settings */
    if(lastHeader.startFreq   != h_sbr_header->startFreq   ||
       lastHeader.stopFreq    != h_sbr_header->stopFreq    ||
       lastHeader.xover_band  != h_sbr_header->xover_band  ||
       lastHeader.freqScale   != h_sbr_header->freqScale   ||
       lastHeader.alterScale  != h_sbr_header->alterScale  ||
       lastHeader.noise_bands != h_sbr_header->noise_bands) {
      h_sbr_header->status = HEADER_RESET;
    }
  }
  else{
    h_sbr_header->status = HEADER_RESET;
  }

  return h_sbr_header->status;
}



/*******************************************************************************
 Functionname:  sbrGetAdditionalData
 *******************************************************************************

 Description:

 Arguments:     hFrameData - handle to struct SBR_FRAME_DATA
                hBitBuf    - handle to struct BIT_BUF

 Return:

*******************************************************************************/
static int
sbrGetAdditionalData(HANDLE_SBR_FRAME_DATA hFrameData,
                     HANDLE_BIT_BUFFER hBitBuf)
{
  int i, bitsRead = 0;

  int flag = BufGetBits(hBitBuf,1);
  bitsRead++;

#ifdef SONY_PVC_DEC
	hFrameData->sin_start_for_cur_top = hFrameData->sin_start_for_next_top;
	hFrameData->sin_len_for_cur_top = hFrameData->sin_len_for_next_top;
	hFrameData->sin_start_for_next_top = 0;
	hFrameData->sin_len_for_next_top = 0;
#endif /* SONY_PVC_DEC */

  if(flag){
#ifdef SONY_PVC_DEC
		if(hFrameData->sbr_header.pvc_brmode != 0){
			for(i=0;i<hFrameData->nSfb[HI];i++){
      			hFrameData->addHarmonics[i] = BufGetBits(hBitBuf,1);
				bitsRead++;
			}

			hFrameData->bs_sin_pos = ESC_SIN_POS;

			hFrameData->bs_sin_pos_present = BufGetBits(hBitBuf,1);
			bitsRead += 1;

			if(hFrameData->bs_sin_pos_present == 1){
				hFrameData->bs_sin_pos = BufGetBits(hBitBuf,5);
				bitsRead += 5;
			}
			if(next_varlen > 0){
				if(hFrameData->bs_sin_pos > 16){
					if(hFrameData->bs_sin_pos == 31){
						hFrameData->sin_start_for_next_top = 0;
						hFrameData->sin_len_for_next_top = next_varlen;
					}else{
						if((next_varlen+16) == hFrameData->bs_sin_pos){
							hFrameData->sin_start_for_next_top = 0;
							hFrameData->sin_len_for_next_top = next_varlen;
						}else{
							hFrameData->sin_start_for_next_top = hFrameData->bs_sin_pos - 16;
							hFrameData->sin_len_for_next_top = next_varlen;
						}
					}
				}else{
					hFrameData->sin_start_for_next_top = 0;
					hFrameData->sin_len_for_next_top = next_varlen;
				}
			}else{
				hFrameData->sin_start_for_next_top = 0;
				hFrameData->sin_len_for_next_top = 0;
			}
		}else{
			for(i=0;i<hFrameData->nSfb[HI];i++){
      			hFrameData->addHarmonics[i] = BufGetBits(hBitBuf,1);
				bitsRead++;
			}
		}
#else /* SONY_PVC_DEC */
		for(i=0;i<hFrameData->nSfb[HI];i++){
			hFrameData->addHarmonics[i] = BufGetBits(hBitBuf,1);
			bitsRead++;
		}
#endif /* SONY_PVC_DEC */

  }

  return(bitsRead);
}




/*!
  \brief      Reads extension data from the bitstream

  The bitstream format allows up to 4 kinds of extended data element.
  Extended data may contain several elements, each identified by a 2-bit-ID.
*/
static void extractExtendedData(HANDLE_SBR_FRAME_DATA hFrameData,     /*!< Destination for extracted data of left channel */
                                HANDLE_SBR_FRAME_DATA hFrameDataRight,/*!< Destination for extracted data of right channel */
                                HANDLE_BIT_BUFFER hBitBuf              /*!< Handle to the bit buffer */
#ifdef PARAMETRICSTEREO
                                ,HANDLE_PS_DEC hParametricStereoDec    /*!< Parametric Stereo Decoder */
#endif
                                )
{
  int extended_data;
  int i,nBitsLeft,extension_id;

  extended_data = BufGetBits(hBitBuf, SI_SBR_EXTENDED_DATA_BITS);

  if (extended_data) {
    int cnt;

    cnt = BufGetBits(hBitBuf, SI_SBR_EXTENSION_SIZE_BITS);
    if (cnt == (1<<SI_SBR_EXTENSION_SIZE_BITS)-1)
      cnt += BufGetBits(hBitBuf, SI_SBR_EXTENSION_ESC_COUNT_BITS);

    nBitsLeft = 8 * cnt;
    while (nBitsLeft > 7) {
      extension_id = BufGetBits(hBitBuf, SI_SBR_EXTENSION_ID_BITS);
      nBitsLeft -= SI_SBR_EXTENSION_ID_BITS;

      switch(extension_id) {

#ifdef PARAMETRICSTEREO
      case EXTENSION_ID_PS_CODING:
        {
          static int psDetected = 0;
          if (!psDetected) {
            if (debug['n'])
              fprintf(stderr, "Implicit signalling of PS for SBR detected (-c enables decoding).\n");
            psDetected = 1;
          }
        }
        nBitsLeft -= tfReadPsData(hParametricStereoDec, hBitBuf, nBitsLeft);
	/* parametric stereo detected, could set channelMode accordingly here */
        break;
#endif

      /* Future extensions shall be extracted here!! */

      default:
        /* An unknown extension id causes the remaining extension data
           to be skipped */
        cnt = nBitsLeft >> 3; /* number of remaining bytes */
        for (i=0; i<cnt; i++)
          BufGetBits(hBitBuf, 8);
        nBitsLeft -= cnt * 8;
      }
    }
    /* read fill bits for byte alignment */
    BufGetBits(hBitBuf, nBitsLeft);

  }
}


/*******************************************************************************
 Functionname:  sbrGetSingleChannelElement
 *******************************************************************************

 Description:

 Arguments:     hFrameData - handle to struct SBR_FRAME_DATA
                hBitBuf    - handle to struct BIT_BUF

 Return:        SbrFrameOK

*******************************************************************************/
void
sbrGetSingleChannelElement (HANDLE_SBR_FRAME_DATA hFrameData,
                            HANDLE_BIT_BUFFER hBitBuf
#ifdef PARAMETRICSTEREO
                            ,HANDLE_PS_DEC hParametricStereoDec
#endif
                            )
{
  int i, bit;

  /* reserved bits */
  bit = BufGetBits (hBitBuf, SI_SBR_RESERVED_PRESENT);
  if (bit)
    BufGetBits(hBitBuf, SI_SBR_RESERVED_BITS_DATA);

  /* side info */
#ifdef AAC_ELD
  if(hFrameData->ldsbr == 1) {
    /* use ld sbr frame structure if available */
    extractFrameInfo_LD (hBitBuf, hFrameData);
  } else {
#endif
    extractFrameInfo (hBitBuf, hFrameData);
#ifdef AAC_ELD
  }
#endif


  sbrGetDirectionControlData (hFrameData, hBitBuf);
  for(i = 0; i < hFrameData->nNfb; i++){
    hFrameData->sbr_invf_mode_prev[i] = hFrameData->sbr_invf_mode[i];
    hFrameData->sbr_invf_mode[i] =
      (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
  }


  /* raw data */
  sbrGetEnvelope (hFrameData, hBitBuf, 0, 0);

  sbrGetNoiseFloorData (hFrameData, hBitBuf);

  memset(hFrameData->addHarmonics,0,hFrameData->nSfb[HI]*sizeof(int));

  sbrGetAdditionalData(hFrameData, hBitBuf);

  extractExtendedData(hFrameData,NULL,hBitBuf
#ifdef PARAMETRICSTEREO
                      ,hParametricStereoDec
#endif
                      );

  hFrameData->coupling = COUPLING_OFF;

}

/* 20060110 */

void
sbrGetSingleChannelElement_BSAC (HANDLE_SBR_FRAME_DATA hFrameData,
                                 HANDLE_BIT_BUFFER hBitBuf
                                 )
{
  int i, bit;

  /* reserved bits */
  bit = BufGetBits (hBitBuf, SI_SBR_RESERVED_PRESENT);
  if (bit)
    BufGetBits(hBitBuf, SI_SBR_RESERVED_BITS_DATA);

  /* side info */
  extractFrameInfo (hBitBuf, hFrameData);


  sbrGetDirectionControlData (hFrameData, hBitBuf);
  for(i = 0; i < hFrameData->nNfb; i++){
    hFrameData->sbr_invf_mode_prev[i] = hFrameData->sbr_invf_mode[i];
    hFrameData->sbr_invf_mode[i] =
      (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
  }


  /* raw data */
  sbrGetEnvelope (hFrameData, hBitBuf, 0, 0);

  sbrGetNoiseFloorData (hFrameData, hBitBuf);

  memset(hFrameData->addHarmonics,0,hFrameData->nSfb[HI]*sizeof(int));

  sbrGetAdditionalData(hFrameData, hBitBuf);

  extractExtendedData(hFrameData,NULL,hBitBuf
#ifdef PARAMETRICSTEREO
                      ,NULL
#endif
                      );

  hFrameData->coupling = COUPLING_OFF;

}

#ifdef SONY_PVC_DEC
void usac_pvcGetData( HANDLE_PVC_DATA hPvcData 
                     ,HANDLE_BIT_BUFFER hBitBuf
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
                     ,int    indepFlag
#endif /* SONY_PVC_SUPPORT_INDEPFLAG */
                     ){

	int             i, j, k;
    int             fixed_length, num_grid_info, grid_info;
    unsigned char   divMode, nsMode;
    unsigned short  pvcID[PVC_NTIMESLOT];
    unsigned char   num_length;
    unsigned char   length;
    unsigned char   reuse_pvcID;
    int             sum_length = 0;
    int             nbit_length = 4;
    PVC_PARAM       *p_pvcParam;

    p_pvcParam = &(hPvcData->pvcParam);

    /* unpack and parsing */
    divMode = (unsigned char)BufGetBits(hBitBuf, PVC_NBIT_DIVMODE);
    nsMode = (unsigned char)BufGetBits(hBitBuf, PVC_NBIT_NSMODE);


    switch(p_pvcParam->brMode) {
        case 1:
            p_pvcParam->pvc_id_nbit = PVC_ID_NBIT;
            break;

        case 2:
            p_pvcParam->pvc_id_nbit = PVC_ID_NBIT;
            break;

		case 3:
            p_pvcParam->pvc_id_nbit = PVC_ID_NBIT_RESERVED;
            break;

        default :
            ;
    }

    if(divMode <= 3){
        num_length = divMode;

#ifdef SONY_PVC_SUPPORT_INDEPFLAG
        if (indepFlag) {
            reuse_pvcID = 0;
        } else {
            reuse_pvcID = (unsigned char)BufGetBits(hBitBuf, PVC_NBIT_REUSE_PVCID);
        }
#else /* SONY_PVC_SUPPORT_INDEPFLAG */
        reuse_pvcID = (unsigned char)BufGetBits(hBitBuf, PVC_NBIT_REUSE_PVCID);
#endif /* SONY_PVC_SUPPORT_INDEPFLAG */
        if (reuse_pvcID == 1) {
            pvcID[0] = p_pvcParam->pvcID_prev;
        } else {
            pvcID[0] = (unsigned short)BufGetBits(hBitBuf, p_pvcParam->pvc_id_nbit);
        }

        k = 1;
        if (num_length) {
            sum_length = 0;            
            for (i=0; i<num_length; i++) {   
                if (sum_length>=13) {
                    nbit_length = 1;
                } else if (sum_length>=11) {
                    nbit_length = 2;
                } else if (sum_length>=7) {
                    nbit_length = 3;
                } else {
                    nbit_length = 4;
                }
                length = (unsigned char)BufGetBits(hBitBuf, nbit_length); 
                length += 1;
                sum_length += length;
                for (j=1; j<length; j++, k++) {
                    pvcID[k] = pvcID[k-1];
                } 
                pvcID[k++] = (unsigned short)BufGetBits(hBitBuf, p_pvcParam->pvc_id_nbit);
            }
        }
        
        for (; k<16; k++){
            pvcID[k] = pvcID[k-1];
        }

    } else {
      switch(divMode){
      case 4:
        num_grid_info=2;
        fixed_length=8;
        break;
      case 5:
        num_grid_info=4;
        fixed_length=4;
        break;
      case 6:
        num_grid_info=8;
        fixed_length=2;
        break;
      case 7:
        num_grid_info=16;
        fixed_length=1;
        break;
      default:
        ;
      }
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
        if (indepFlag) {
            grid_info = 1;
        } else {
            grid_info = BufGetBits(hBitBuf, PVC_NBIT_GRID_INFO);
        }
#else /* SONY_PVC_SUPPORT_INDEPFLAG */
        grid_info = BufGetBits(hBitBuf, PVC_NBIT_GRID_INFO);
#endif /* SONY_PVC_SUPPORT_INDEPFLAG */
        if (grid_info) {
            pvcID[0] = (unsigned short)BufGetBits(hBitBuf, p_pvcParam->pvc_id_nbit);
        } else {
            pvcID[0] = p_pvcParam->pvcID_prev;
        }
        for(j=1, k=1; j<fixed_length; j++, k++){
			pvcID[k] = pvcID[k-1];
		}

        for(i=1; i<num_grid_info; i++){
          grid_info = BufGetBits(hBitBuf, PVC_NBIT_GRID_INFO);
          if(grid_info == 1){
            pvcID[k++] = (unsigned short)BufGetBits(hBitBuf, p_pvcParam->pvc_id_nbit);
          }else{
            pvcID[k] = pvcID[k-1];
            k++;
          }
          for(j=1; j<fixed_length; j++, k++){
            pvcID[k] = pvcID[k-1];
          }
        }
    }
    /* copy params to handle data */
    p_pvcParam->divMode = divMode;
    p_pvcParam->nsMode = nsMode;
    for (i=0; i<PVC_NTIMESLOT; i++) {
        p_pvcParam->pvcID[i] = pvcID[i];
    }

}


void
usac_sbrGetSingleChannelElement_for_pvc (HANDLE_SBR_FRAME_DATA hFrameData,
                                         HANDLE_BIT_BUFFER hBitBuf, 
                                         int bHarmonicSBR,
                                         int sbr_mode,
                                         HANDLE_PVC_DATA hPvcData
                                         ,int *cur_varlen
                                         ,const int bUsacIndependenceFlag
                                         )
{
  int i;
  int bs_noise_pos;
  int tmp;

  hFrameData->bUsacIndependenceFlag = bUsacIndependenceFlag;

  /* side info */
  if(bHarmonicSBR){
    hFrameData->sbrPatchingMode = BufGetBits (hBitBuf, SI_SBR_PATCHING_MODE);

    if(hFrameData->sbrPatchingMode == 0)
      {
        hFrameData->sbrOversamplingFlag = BufGetBits (hBitBuf, SI_SBR_OVERSAMPLING_FLAG);
        if(BufGetBits (hBitBuf, SI_SBR_PITCH_FLAG))
          hFrameData->sbrPitchInBins = BufGetBits (hBitBuf, SI_SBR_PITCH_ESTIMATE);
        else
          hFrameData->sbrPitchInBins = 0;
      } else {
      hFrameData->sbrOversamplingFlag = hFrameData->sbrPitchInBins = 0;
    }
  }


  /* noise position data */
  bs_noise_pos = BufGetBits (hBitBuf, 4);

  *cur_varlen = next_varlen;
  /* Read the first bit */
  tmp = BufGetBits (hBitBuf, 1);

  if(tmp == 0){
    next_varlen = 0;
  }else{
    tmp = BufGetBits (hBitBuf, 2);
    if(tmp == 0){
      next_varlen = 1;
    }else if(tmp == 1){
      next_varlen = 2;
    }else if(tmp == 2){
      next_varlen = 3;
    }else if(tmp == 3){
      next_varlen = 4;
    }
  }
  /* Build frameInfo for PVC decode */
  extractFrameInfo_for_pvc (hBitBuf, hFrameData, bs_noise_pos, hPvcData->pvcParam.prev_sbr_mode,next_varlen);

#ifndef SONY_PVC_NOFIX
  hPvcData->pvcParam.prev_sbr_mode = USE_PVC_SBR;
#endif

  /* dtdf data */
  sbrGetDirectionControlData_for_pvc (hFrameData, hBitBuf, hFrameData->frameInfo[1+2*hFrameData->frameInfo[0]+1+1]);
  
  /* inverse filetering data */
  for(i = 0; i < hFrameData->nNfb; i++){
    hFrameData->sbr_invf_mode_prev[i] = hFrameData->sbr_invf_mode[i];
    hFrameData->sbr_invf_mode[i] = (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);

  }
  
  hPvcData->pvcParam.brMode = hFrameData->sbr_header.pvc_brmode;
  /* Read PVC data */
#ifdef SONY_PVC_SUPPORT_INDEPFLAG
  usac_pvcGetData(hPvcData, hBitBuf, bUsacIndependenceFlag);
#else /* SONY_PVC_SUPPORT_INDEPFLAG */
  usac_pvcGetData(hPvcData, hBitBuf);
#endif /* SONY_PVC_SUPPORT_INDEPFLAG */

  /* noise envelope raw data */
  sbrGetNoiseFloorData (hFrameData, hBitBuf);

  /* add_harmonics */
  memset(hFrameData->addHarmonics,0,hFrameData->nSfb[HI]*sizeof(int));
  sbrGetAdditionalData(hFrameData, hBitBuf); 

  hFrameData->coupling = COUPLING_OFF;
}

#endif /* SONY_PVC_DEC */



void
usac_sbrGetSingleChannelElement (HANDLE_SBR_FRAME_DATA hFrameData,
                                 HANDLE_BIT_BUFFER hBitBuf, 
                                 int bHarmonicSBR,
                                 int bs_interTes,
#ifdef SONY_PVC_DEC
                                 HANDLE_PVC_DATA hPvcData,
#endif /* SONY_PVC_DEC */
                                 int const bUsacIndependenceFlag)
{
  int i;

  hFrameData->bUsacIndependenceFlag = bUsacIndependenceFlag;
  /* side info */
  if(bHarmonicSBR){
    hFrameData->sbrPatchingMode = BufGetBits (hBitBuf, SI_SBR_PATCHING_MODE);
    if(hFrameData->sbrPatchingMode == 0){
      hFrameData->sbrOversamplingFlag = BufGetBits (hBitBuf, SI_SBR_OVERSAMPLING_FLAG);
      if(BufGetBits (hBitBuf, SI_SBR_PITCH_FLAG))
        hFrameData->sbrPitchInBins = BufGetBits (hBitBuf, SI_SBR_PITCH_ESTIMATE);
      else
        hFrameData->sbrPitchInBins = 0;
    } else {
      hFrameData->sbrOversamplingFlag = hFrameData->sbrPitchInBins = 0;
    }
  }


  extractFrameInfo (hBitBuf, hFrameData);

#ifndef SONY_PVC_NOFIX
  hPvcData->pvcParam.prev_sbr_mode = USE_ORIG_SBR;
#endif

  sbrGetDirectionControlData (hFrameData, hBitBuf);
  for(i = 0; i < hFrameData->nNfb; i++){
    hFrameData->sbr_invf_mode_prev[i] = hFrameData->sbr_invf_mode[i];
    hFrameData->sbr_invf_mode[i] =
      (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
  }

  /* raw data */
  sbrGetEnvelope (hFrameData, hBitBuf, 0, bs_interTes);
  sbrGetNoiseFloorData (hFrameData, hBitBuf);

  memset(hFrameData->addHarmonics,0,hFrameData->nSfb[HI]*sizeof(int));
  sbrGetAdditionalData(hFrameData, hBitBuf); /* add_harmonics */

  hFrameData->coupling = COUPLING_OFF;
}



#ifdef SBR_SCALABLE
/*******************************************************************************
 Functionname:  sbrGetChannelPairElement
 *******************************************************************************

 Description:

 Arguments:     hFrameDataLeft  - handle to struct SBR_FRAME_DATA for first channel
                hFrameDataRight - handle to struct SBR_FRAME_DATA for first channel
                hBitBuf         - handle to struct BIT_BUF

 Return:        SbrFrameOK

*******************************************************************************/
void
sbrGetChannelPairElement (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                          HANDLE_SBR_FRAME_DATA hFrameDataRight,
                          HANDLE_BIT_BUFFER hBitBufBase,
                          HANDLE_BIT_BUFFER hBitBufEnh,
                          int bStereoLayer)
{
  int i, bit;

  int bStereoEnhancementLayerDecodableE = 0;
  int bStereoEnhancementLayerDecodableQ = 0;

  /* reserved bits */
  bit = BufGetBits (hBitBufBase, SI_SBR_RESERVED_PRESENT);
  if (bit) {
    BufGetBits(hBitBufBase, SI_SBR_RESERVED_BITS_DATA);
    BufGetBits(hBitBufBase, SI_SBR_RESERVED_BITS_DATA);
  }

  /* Read coupling flag */
  bit = BufGetBits (hBitBufBase, SI_SBR_COUPLING_BITS);
  if (bit) {
    hFrameDataLeft->coupling = COUPLING_LEVEL;
    hFrameDataRight->coupling = COUPLING_BAL;
  }
  else {
    hFrameDataLeft->coupling = COUPLING_OFF;
    hFrameDataRight->coupling = COUPLING_OFF;
  }

#ifdef AAC_ELD
  if(hFrameDataLeft->ldsbr == 1) {
    /* use ld sbr frame structure if available */
    extractFrameInfo_LD (hBitBufBase, hFrameDataLeft);
  } else {
#endif
    extractFrameInfo (hBitBufBase, hFrameDataLeft);
#ifdef AAC_ELD
  }
#endif



  if (hFrameDataLeft->coupling) {

    sbrCopyFrameControlData (hFrameDataRight, hFrameDataLeft);

    sbrGetDirectionControlData (hFrameDataLeft, hBitBufBase);


    bStereoEnhancementLayerDecodableE = 0;
    bStereoEnhancementLayerDecodableQ = 0;

    if(bStereoLayer){
      sbrGetDirectionControlData (hFrameDataRight, hBitBufEnh);
      bStereoEnhancementLayerDecodableE = 1;
      bStereoEnhancementLayerDecodableQ = 1;

      if(hFrameDataLeft->bStereoEnhancementLayerDecodablePrevE == 0){
        if(hFrameDataLeft->domain_vec1[0] == 1){
          bStereoEnhancementLayerDecodableE = 0;
        }
      }

      if(hFrameDataLeft->bStereoEnhancementLayerDecodablePrevQ == 0){
        if(hFrameDataLeft->domain_vec2[0] == 1){
          bStereoEnhancementLayerDecodableQ = 0;
        }
      }
    }

    if(bStereoEnhancementLayerDecodableE == 0){
      sbr_dtdf_no_enhancement_layerE(hFrameDataRight);
    }

    if(bStereoEnhancementLayerDecodableQ == 0){
      sbr_dtdf_no_enhancement_layerQ(hFrameDataRight);
    }


    for(i = 0; i < hFrameDataLeft->nNfb; i++){
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataLeft->sbr_invf_mode[i]  = (INVF_MODE) BufGetBits (hBitBufBase, SI_SBR_INVF_MODE_BITS);
      hFrameDataRight->sbr_invf_mode[i] = hFrameDataLeft->sbr_invf_mode[i];
    }

    sbrGetEnvelope (hFrameDataLeft, hBitBufBase, 0, 0);
    sbrGetNoiseFloorData (hFrameDataLeft, hBitBufBase);


    if(bStereoEnhancementLayerDecodableE){
      sbrGetEnvelope (hFrameDataRight, hBitBufEnh, 1, 0);
    }
    else{
      if(bStereoLayer){
        sbrGetEnvelope (hFrameDataRight, hBitBufEnh, 1, 0);
      }
      sbr_envelope_no_enhancement_layer(hFrameDataLeft,hFrameDataRight);
    }

    if(bStereoEnhancementLayerDecodableQ){
      sbrGetNoiseFloorData (hFrameDataRight, hBitBufEnh);
    }
    else{
      if(bStereoLayer){
        sbrGetNoiseFloorData (hFrameDataRight, hBitBufEnh);
      }
      sbr_noise_no_enhancement_layer(hFrameDataLeft,hFrameDataRight);
    }


    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));

    sbrGetAdditionalData(hFrameDataLeft, hBitBufBase);

    if(bStereoLayer){
      sbrGetAdditionalData(hFrameDataRight, hBitBufEnh);
    }
    else{
      sbr_additional_data_no_enhancement_layer(hFrameDataLeft,hFrameDataRight);
    }

  }
  else {
#ifdef AAC_ELD
    if(hFrameDataRight->ldsbr == 1) {
      /* use ld sbr frame structure if available */
      extractFrameInfo_LD (hBitBufBase, hFrameDataRight);
    } else {
#endif
      extractFrameInfo (hBitBufBase, hFrameDataRight);
#ifdef AAC_ELD
    }
#endif

    sbrGetDirectionControlData (hFrameDataLeft,  hBitBufBase);
    sbrGetDirectionControlData (hFrameDataRight, hBitBufBase);

    for(i = 0; i <  hFrameDataLeft->nNfb; i++) {
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataLeft->sbr_invf_mode[i]  =
        (INVF_MODE) BufGetBits (hBitBufBase, SI_SBR_INVF_MODE_BITS);
    }

    for(i = 0; i <  hFrameDataRight->nNfb; i++) {
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataRight->sbr_invf_mode[i] =
        (INVF_MODE) BufGetBits (hBitBufBase, SI_SBR_INVF_MODE_BITS);
    }
    sbrGetEnvelope (hFrameDataLeft,  hBitBufBase, 0, 0);
    sbrGetEnvelope (hFrameDataRight, hBitBufBase, 1, 0);
    sbrGetNoiseFloorData (hFrameDataLeft,  hBitBufBase);
    sbrGetNoiseFloorData (hFrameDataRight, hBitBufBase);

    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));

    sbrGetAdditionalData(hFrameDataLeft, hBitBufBase);
    sbrGetAdditionalData(hFrameDataRight, hBitBufBase);
  }

  extractExtendedData(hFrameDataLeft,hFrameDataRight,hBitBufBase);

  hFrameDataLeft->bStereoEnhancementLayerDecodablePrevE  = bStereoEnhancementLayerDecodableE;
  hFrameDataRight->bStereoEnhancementLayerDecodablePrevQ = bStereoEnhancementLayerDecodableQ;
  hFrameDataLeft->bStereoEnhancementLayerDecodablePrevE  = bStereoEnhancementLayerDecodableE;
  hFrameDataRight->bStereoEnhancementLayerDecodablePrevQ = bStereoEnhancementLayerDecodableQ;

}


void
sbrGetChannelPairElement_BSAC (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                          HANDLE_SBR_FRAME_DATA hFrameDataRight,
                          HANDLE_BIT_BUFFER hBitBuf)
{
  int i, bit;

  /* reserved bits */
  bit = BufGetBits (hBitBuf, SI_SBR_RESERVED_PRESENT);
  if (bit) {
    BufGetBits(hBitBuf, SI_SBR_RESERVED_BITS_DATA);
    BufGetBits(hBitBuf, SI_SBR_RESERVED_BITS_DATA);
  }

  /* Read coupling flag */
  bit = BufGetBits (hBitBuf, SI_SBR_COUPLING_BITS);

  if (bit) {
    hFrameDataLeft->coupling = COUPLING_LEVEL;
    hFrameDataRight->coupling = COUPLING_BAL;
  }
  else {
    hFrameDataLeft->coupling = COUPLING_OFF;
    hFrameDataRight->coupling = COUPLING_OFF;
  }

  extractFrameInfo (hBitBuf, hFrameDataLeft);




  if (hFrameDataLeft->coupling) {

    sbrCopyFrameControlData (hFrameDataRight, hFrameDataLeft);

    sbrGetDirectionControlData (hFrameDataLeft, hBitBuf);

    sbrGetDirectionControlData (hFrameDataRight, hBitBuf);



    for(i = 0; i < hFrameDataLeft->nNfb; i++){
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataLeft->sbr_invf_mode[i]  = (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
      hFrameDataRight->sbr_invf_mode[i] = hFrameDataLeft->sbr_invf_mode[i];
    }

    sbrGetEnvelope (hFrameDataLeft, hBitBuf, 0, 0);
    sbrGetNoiseFloorData (hFrameDataLeft, hBitBuf);

    sbrGetEnvelope (hFrameDataRight, hBitBuf, 1, 0);
    sbrGetNoiseFloorData (hFrameDataRight, hBitBuf);


    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));

    sbrGetAdditionalData(hFrameDataLeft, hBitBuf);
    sbrGetAdditionalData(hFrameDataRight, hBitBuf);

  }
  else {
    extractFrameInfo (hBitBuf, hFrameDataRight);

    sbrGetDirectionControlData (hFrameDataLeft,  hBitBuf);
    sbrGetDirectionControlData (hFrameDataRight, hBitBuf);

    for(i = 0; i <  hFrameDataLeft->nNfb; i++) {
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataLeft->sbr_invf_mode[i]  =
        (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
    }

    for(i = 0; i <  hFrameDataRight->nNfb; i++) {
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataRight->sbr_invf_mode[i] =
        (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
    }
    sbrGetEnvelope (hFrameDataLeft,  hBitBuf, 0, 0);
    sbrGetEnvelope (hFrameDataRight, hBitBuf, 1, 0);
    sbrGetNoiseFloorData (hFrameDataLeft,  hBitBuf);
    sbrGetNoiseFloorData (hFrameDataRight, hBitBuf);

    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));

    sbrGetAdditionalData(hFrameDataLeft, hBitBuf);
    sbrGetAdditionalData(hFrameDataRight, hBitBuf);
  }
}

#else

/*******************************************************************************
 Functionname:  sbrGetChannelPairElement
 *******************************************************************************

 Description:

 Arguments:     hFrameDataLeft  - handle to struct SBR_FRAME_DATA for first channel
                hFrameDataRight - handle to struct SBR_FRAME_DATA for first channel
                hBitBuf         - handle to struct BIT_BUF

 Return:        SbrFrameOK

*******************************************************************************/
void
sbrGetChannelPairElement (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                          HANDLE_SBR_FRAME_DATA hFrameDataRight,
                          HANDLE_BIT_BUFFER hBitBuf)
{
  int i, bit;

  /* reserved bits */
  bit = BufGetBits (hBitBuf, SI_SBR_RESERVED_PRESENT);
  if (bit) {
    BufGetBits(hBitBuf, SI_SBR_RESERVED_BITS_DATA);
    BufGetBits(hBitBuf, SI_SBR_RESERVED_BITS_DATA);
  }

  /* Read coupling flag */
  bit = BufGetBits (hBitBuf, SI_SBR_COUPLING_BITS);

  if (bit) {
    hFrameDataLeft->coupling = COUPLING_LEVEL;
    hFrameDataRight->coupling = COUPLING_BAL;
  }
  else {
    hFrameDataLeft->coupling = COUPLING_OFF;
    hFrameDataRight->coupling = COUPLING_OFF;
  }

#ifdef AAC_ELD
  if(hFrameDataLeft->ldsbr == 1) {
    /* use ld sbr frame structure if available */
    extractFrameInfo_LD (hBitBuf, hFrameDataLeft);
  } else {
#endif
    extractFrameInfo (hBitBuf, hFrameDataLeft);
#ifdef AAC_ELD
  }
#endif




  if (hFrameDataLeft->coupling) {

    sbrCopyFrameControlData (hFrameDataRight, hFrameDataLeft);

    sbrGetDirectionControlData (hFrameDataLeft, hBitBuf);

    sbrGetDirectionControlData (hFrameDataRight, hBitBuf);



    for(i = 0; i < hFrameDataLeft->nNfb; i++){
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataLeft->sbr_invf_mode[i]  = (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
      hFrameDataRight->sbr_invf_mode[i] = hFrameDataLeft->sbr_invf_mode[i];
    }

    sbrGetEnvelope (hFrameDataLeft, hBitBuf, 0, 0);
    sbrGetNoiseFloorData (hFrameDataLeft, hBitBuf);

    sbrGetEnvelope (hFrameDataRight, hBitBuf, 1, 0);
    sbrGetNoiseFloorData (hFrameDataRight, hBitBuf);


    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));

    sbrGetAdditionalData(hFrameDataLeft, hBitBuf);
    sbrGetAdditionalData(hFrameDataRight, hBitBuf);

  }
  else {
#ifdef AAC_ELD
    if(hFrameDataRight->ldsbr == 1) {
      /* use ld sbr frame structure if available */
      extractFrameInfo_LD (hBitBuf, hFrameDataRight);
    } else {
#endif
      extractFrameInfo (hBitBuf, hFrameDataRight);
#ifdef AAC_ELD
    }
#endif

    sbrGetDirectionControlData (hFrameDataLeft,  hBitBuf);
    sbrGetDirectionControlData (hFrameDataRight, hBitBuf);

    for(i = 0; i <  hFrameDataLeft->nNfb; i++) {
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataLeft->sbr_invf_mode[i]  =
        (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
    }

    for(i = 0; i <  hFrameDataRight->nNfb; i++) {
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataRight->sbr_invf_mode[i] =
        (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
    }
    sbrGetEnvelope (hFrameDataLeft,  hBitBuf, 0, 0);
    sbrGetEnvelope (hFrameDataRight, hBitBuf, 1, 0);
    sbrGetNoiseFloorData (hFrameDataLeft,  hBitBuf);
    sbrGetNoiseFloorData (hFrameDataRight, hBitBuf);

    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));

    sbrGetAdditionalData(hFrameDataLeft, hBitBuf);
    sbrGetAdditionalData(hFrameDataRight, hBitBuf);
  }

  extractExtendedData(hFrameDataLeft,hFrameDataRight,hBitBuf
#ifdef PARAMETRICSTEREO
                      ,NULL
#endif
                      );
}

void
sbrGetChannelPairElement_BSAC (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                          HANDLE_SBR_FRAME_DATA hFrameDataRight,
                          HANDLE_BIT_BUFFER hBitBuf)
{
  int i, bit;

  /* reserved bits */
  bit = BufGetBits (hBitBuf, SI_SBR_RESERVED_PRESENT);
  if (bit) {
    BufGetBits(hBitBuf, SI_SBR_RESERVED_BITS_DATA);
    BufGetBits(hBitBuf, SI_SBR_RESERVED_BITS_DATA);
  }

  /* Read coupling flag */
  bit = BufGetBits (hBitBuf, SI_SBR_COUPLING_BITS);

  if (bit) {
    hFrameDataLeft->coupling = COUPLING_LEVEL;
    hFrameDataRight->coupling = COUPLING_BAL;
  }
  else {
    hFrameDataLeft->coupling = COUPLING_OFF;
    hFrameDataRight->coupling = COUPLING_OFF;
  }

  extractFrameInfo (hBitBuf, hFrameDataLeft);




  if (hFrameDataLeft->coupling) {

    sbrCopyFrameControlData (hFrameDataRight, hFrameDataLeft);

    sbrGetDirectionControlData (hFrameDataLeft, hBitBuf);

    sbrGetDirectionControlData (hFrameDataRight, hBitBuf);



    for(i = 0; i < hFrameDataLeft->nNfb; i++){
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataLeft->sbr_invf_mode[i]  = (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
      hFrameDataRight->sbr_invf_mode[i] = hFrameDataLeft->sbr_invf_mode[i];
    }

    sbrGetEnvelope (hFrameDataLeft, hBitBuf, 0, 0);
    sbrGetNoiseFloorData (hFrameDataLeft, hBitBuf);

    sbrGetEnvelope (hFrameDataRight, hBitBuf, 1, 0);
    sbrGetNoiseFloorData (hFrameDataRight, hBitBuf);


    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));

    sbrGetAdditionalData(hFrameDataLeft, hBitBuf);
    sbrGetAdditionalData(hFrameDataRight, hBitBuf);

  }
  else {
    extractFrameInfo (hBitBuf, hFrameDataRight);

    sbrGetDirectionControlData (hFrameDataLeft,  hBitBuf);
    sbrGetDirectionControlData (hFrameDataRight, hBitBuf);

    for(i = 0; i <  hFrameDataLeft->nNfb; i++) {
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataLeft->sbr_invf_mode[i]  =
        (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
    }

    for(i = 0; i <  hFrameDataRight->nNfb; i++) {
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataRight->sbr_invf_mode[i] =
        (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
    }
    sbrGetEnvelope (hFrameDataLeft,  hBitBuf, 0, 0);
    sbrGetEnvelope (hFrameDataRight, hBitBuf, 1, 0);
    sbrGetNoiseFloorData (hFrameDataLeft,  hBitBuf);
    sbrGetNoiseFloorData (hFrameDataRight, hBitBuf);

    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));
    /*memset(hFrameDataRight->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));*/

    sbrGetAdditionalData(hFrameDataLeft, hBitBuf);
    sbrGetAdditionalData(hFrameDataRight, hBitBuf);
  }

  extractExtendedData(hFrameDataLeft,hFrameDataRight,hBitBuf
#ifdef PARAMETRICSTEREO
                      ,NULL
#endif
                      );
}
#endif


void
usac_sbrGetChannelPairElement (HANDLE_SBR_FRAME_DATA hFrameDataLeft,
                               HANDLE_SBR_FRAME_DATA hFrameDataRight,
                               HANDLE_BIT_BUFFER hBitBuf, 
                               int bHarmonicSBR,
                               int bs_interTes,
                               int const bUsacIndependenceFlag)
{
  int i, bit;

  hFrameDataLeft->bUsacIndependenceFlag = bUsacIndependenceFlag;
  hFrameDataRight->bUsacIndependenceFlag = bUsacIndependenceFlag;

  /* Read coupling flag */
  bit = BufGetBits (hBitBuf, SI_SBR_COUPLING_BITS);

  if (bit) {
    if(bHarmonicSBR){
      hFrameDataLeft->sbrPatchingMode = hFrameDataRight->sbrPatchingMode = BufGetBits (hBitBuf, SI_SBR_PATCHING_MODE);
      if(hFrameDataLeft->sbrPatchingMode == 0)
      {
        hFrameDataLeft->sbrOversamplingFlag = hFrameDataRight->sbrOversamplingFlag =
          BufGetBits (hBitBuf, SI_SBR_OVERSAMPLING_FLAG);
        if(BufGetBits (hBitBuf, SI_SBR_PITCH_FLAG))
          hFrameDataLeft->sbrPitchInBins = hFrameDataRight->sbrPitchInBins =
            BufGetBits (hBitBuf, SI_SBR_PITCH_ESTIMATE);
        else
          hFrameDataLeft->sbrPitchInBins = hFrameDataRight->sbrPitchInBins = 0;
      } else {
        hFrameDataLeft->sbrOversamplingFlag = hFrameDataRight->sbrOversamplingFlag = 
          hFrameDataLeft->sbrPitchInBins = hFrameDataRight->sbrPitchInBins      = 0;
      }
    }

    hFrameDataLeft->coupling = COUPLING_LEVEL;
    hFrameDataRight->coupling = COUPLING_BAL;
  }
  else {
    if(bHarmonicSBR){
      hFrameDataLeft->sbrPatchingMode  = BufGetBits (hBitBuf, SI_SBR_PATCHING_MODE); 
      if(hFrameDataLeft->sbrPatchingMode == 0)
      {
        hFrameDataLeft->sbrOversamplingFlag = BufGetBits (hBitBuf, SI_SBR_OVERSAMPLING_FLAG);
        if(BufGetBits (hBitBuf, SI_SBR_PITCH_FLAG))
          hFrameDataLeft->sbrPitchInBins = BufGetBits (hBitBuf, SI_SBR_PITCH_ESTIMATE);
        else
          hFrameDataLeft->sbrPitchInBins = 0;
      } else {
        hFrameDataLeft->sbrOversamplingFlag = hFrameDataLeft->sbrPitchInBins = 0;
      }
      hFrameDataRight->sbrPatchingMode = BufGetBits (hBitBuf, SI_SBR_PATCHING_MODE);
      if(hFrameDataRight->sbrPatchingMode == 0)
      {
        hFrameDataRight->sbrOversamplingFlag = BufGetBits (hBitBuf, SI_SBR_OVERSAMPLING_FLAG);
        if(BufGetBits (hBitBuf, SI_SBR_PITCH_FLAG))
          hFrameDataRight->sbrPitchInBins = BufGetBits (hBitBuf, SI_SBR_PITCH_ESTIMATE);
        else
          hFrameDataRight->sbrPitchInBins = 0;
      } else {
        hFrameDataRight->sbrOversamplingFlag = hFrameDataRight->sbrPitchInBins = 0;
      }
    }

    hFrameDataLeft->coupling = COUPLING_OFF;
    hFrameDataRight->coupling = COUPLING_OFF;
  }


    extractFrameInfo (hBitBuf, hFrameDataLeft);





  if (hFrameDataLeft->coupling) {

    sbrCopyFrameControlData (hFrameDataRight, hFrameDataLeft);

    sbrGetDirectionControlData (hFrameDataLeft, hBitBuf);

    sbrGetDirectionControlData (hFrameDataRight, hBitBuf);



    for(i = 0; i < hFrameDataLeft->nNfb; i++){
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataLeft->sbr_invf_mode[i]  = (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
      hFrameDataRight->sbr_invf_mode[i] = hFrameDataLeft->sbr_invf_mode[i];
    }

    sbrGetEnvelope (hFrameDataLeft, hBitBuf, 0, bs_interTes);
    sbrGetNoiseFloorData (hFrameDataLeft, hBitBuf);

    sbrGetEnvelope (hFrameDataRight, hBitBuf, 1, bs_interTes);
    sbrGetNoiseFloorData (hFrameDataRight, hBitBuf);


    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));

    sbrGetAdditionalData(hFrameDataLeft, hBitBuf);
    sbrGetAdditionalData(hFrameDataRight, hBitBuf);

  }
  else {

    extractFrameInfo (hBitBuf, hFrameDataRight);

    sbrGetDirectionControlData (hFrameDataLeft,  hBitBuf);
    sbrGetDirectionControlData (hFrameDataRight, hBitBuf);

    for(i = 0; i <  hFrameDataLeft->nNfb; i++) {
      hFrameDataLeft->sbr_invf_mode_prev[i]  = hFrameDataLeft->sbr_invf_mode[i];
      hFrameDataLeft->sbr_invf_mode[i]  =
        (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
    }

    for(i = 0; i <  hFrameDataRight->nNfb; i++) {
      hFrameDataRight->sbr_invf_mode_prev[i] = hFrameDataRight->sbr_invf_mode[i];

      hFrameDataRight->sbr_invf_mode[i] =
        (INVF_MODE) BufGetBits (hBitBuf, SI_SBR_INVF_MODE_BITS);
    }
    sbrGetEnvelope (hFrameDataLeft,  hBitBuf, 0, bs_interTes);
    sbrGetEnvelope (hFrameDataRight, hBitBuf, 1, bs_interTes);
    sbrGetNoiseFloorData (hFrameDataLeft,  hBitBuf);
    sbrGetNoiseFloorData (hFrameDataRight, hBitBuf);

    memset(hFrameDataLeft->addHarmonics,0,hFrameDataLeft->nSfb[HI]*sizeof(int));
    memset(hFrameDataRight->addHarmonics,0,hFrameDataRight->nSfb[HI]*sizeof(int));

    sbrGetAdditionalData(hFrameDataLeft, hBitBuf);
    sbrGetAdditionalData(hFrameDataRight, hBitBuf);
  }


}

/*******************************************************************************
 Functionname:  sbrGetDirectionControlData
 *******************************************************************************

 Description:   Reads direction control data from bitstream

 Arguments:     h_frame_data - handle to struct SBR_FRAME_DATA
                hBitBuf      - handle to struct BIT_BUF

 Return:        void

*******************************************************************************/
void
sbrGetDirectionControlData (HANDLE_SBR_FRAME_DATA h_frame_data,
                            HANDLE_BIT_BUFFER hBitBuf)
{
  int i;
  int const bUsacIndependenceFlag = h_frame_data->bUsacIndependenceFlag;

  h_frame_data->nNoiseFloorEnvelopes = h_frame_data->frameInfo[0]>1 ? 2 : 1;

  if(bUsacIndependenceFlag){
    h_frame_data->domain_vec1[0] = 0;
  } else {
    h_frame_data->domain_vec1[0] = BufGetBits (hBitBuf, SI_SBR_DOMAIN_BITS);
  }

  for (i = 1; i < h_frame_data->frameInfo[0]; i++) {
    h_frame_data->domain_vec1[i] = BufGetBits (hBitBuf, SI_SBR_DOMAIN_BITS);
  }

  if(bUsacIndependenceFlag){
    h_frame_data->domain_vec2[0] = 0;
  } else {
    h_frame_data->domain_vec2[0] = BufGetBits (hBitBuf, SI_SBR_DOMAIN_BITS);
  }

  for (i = 1; i < h_frame_data->nNoiseFloorEnvelopes; i++) {
    h_frame_data->domain_vec2[i] = BufGetBits (hBitBuf, SI_SBR_DOMAIN_BITS);
  }
}



#ifdef SONY_PVC_DEC
void sbrGetDirectionControlData_for_pvc (HANDLE_SBR_FRAME_DATA h_frame_data,
                                         HANDLE_BIT_BUFFER hBitBuf,
                                         int bs_num_noise
                                         )
{
  int i;
  int const bUsacIndependenceFlag = h_frame_data->bUsacIndependenceFlag;

  h_frame_data->nNoiseFloorEnvelopes = bs_num_noise;

  if(bUsacIndependenceFlag){
    h_frame_data->domain_vec2[0] = 0;
  } else {
    h_frame_data->domain_vec2[0] = BufGetBits (hBitBuf, SI_SBR_DOMAIN_BITS);
  }

  for (i = 1; i < h_frame_data->nNoiseFloorEnvelopes; i++) {
    h_frame_data->domain_vec2[i] = BufGetBits (hBitBuf, SI_SBR_DOMAIN_BITS);
  }
}

#endif /* SONY_PVC_DEC */


/*******************************************************************************
 Functionname:  sbrCopyFrameControlData
 *******************************************************************************

 Description:   Reads noise-floor-level data from bitstream

 Arguments:     hTargetFrameData - handle to struct SBR_FRAME_DATA
                hSourceFrameData - handle to struct SBR_FRAME_DATA

 Return:        void

*******************************************************************************/
void
sbrCopyFrameControlData (HANDLE_SBR_FRAME_DATA hTargetFrameData,
                         HANDLE_SBR_FRAME_DATA hSourceFrameData)
{
  memcpy (hTargetFrameData->frameInfo, hSourceFrameData->frameInfo,
          LENGTH_FRAME_INFO * sizeof (int));

  hTargetFrameData->nNoiseFloorEnvelopes = hSourceFrameData->nNoiseFloorEnvelopes;
  hTargetFrameData->frameClass = hSourceFrameData->frameClass;
  hTargetFrameData->ampRes = hSourceFrameData->ampRes;

}



/*******************************************************************************
 Functionname:  sbrGetNoiseFloorData
 *******************************************************************************

 Description:   Reads noise-floor-level data from bitstream

 Arguments:     h_frame_data - handle to struct SBR_FRAME_DATA
                hBitBuf      - handle to struct BIT_BUF

 Return:        void

*******************************************************************************/
void
sbrGetNoiseFloorData (HANDLE_SBR_FRAME_DATA h_frame_data,
                      HANDLE_BIT_BUFFER hBitBuf)
{
  int i, j;
  int delta;
  COUPLING_MODE coupling = h_frame_data->coupling;
  int noNoiseBands = h_frame_data->nNfb;

  Huffman hcb_noiseF;
  Huffman hcb_noise;
  int envDataTableCompFactor;


  if (coupling == COUPLING_BAL) {
    hcb_noise = (Huffman)&bookSbrNoiseBalance11T;
    hcb_noiseF = (Huffman)&bookSbrEnvBalance11F;  /* "bookSbrNoiseBalance11F" */
    envDataTableCompFactor = 1;
  }
  else {
    hcb_noise = (Huffman)&bookSbrNoiseLevel11T;
    hcb_noiseF = (Huffman)&bookSbrEnvLevel11F;    /* "bookSbrNoiseLevel11F" */
    envDataTableCompFactor = 0;
  }

  /*
    Calculate number of values alltogether
  */
  h_frame_data->nNoiseFactors = h_frame_data->frameInfo[h_frame_data->frameInfo[0]*2+3]*noNoiseBands;


  for (i=0; i<h_frame_data->nNoiseFloorEnvelopes; i++) {
    if (h_frame_data->domain_vec2[i] == FREQ) {
      if (coupling == COUPLING_BAL) {
        h_frame_data->sbrNoiseFloorLevel[i*noNoiseBands] =
          (float) ( BufGetBits (hBitBuf, SI_SBR_START_NOISE_BITS_BALANCE_AMP_RES_3_0) << envDataTableCompFactor );
      }
      else {
        h_frame_data->sbrNoiseFloorLevel[i*noNoiseBands] =
          (float) BufGetBits (hBitBuf, SI_SBR_START_NOISE_BITS_AMP_RES_3_0);
      }

      for (j = 1; j < noNoiseBands; j++) {
        delta = decode_huff_cw(hcb_noiseF, hBitBuf);
        h_frame_data->sbrNoiseFloorLevel[i*noNoiseBands+j] = (float) (delta << envDataTableCompFactor);
      }
    }
    else {
      for (j = 0; j < noNoiseBands; j++) {
        delta = decode_huff_cw(hcb_noise, hBitBuf);
        h_frame_data->sbrNoiseFloorLevel[i*noNoiseBands+j] = (float) (delta << envDataTableCompFactor);
      }
    }
  }
}



/*******************************************************************************
 Functionname:  sbrGetEnvelope
 *******************************************************************************

 Description:   Reads envelope data from bitstream

 Arguments:     h_frame_data - handle to struct SBR_FRAME_DATA
                hBitBuf      - handle to struct BIT_BUF
                channel      - channel number

 Return:        void

*******************************************************************************/
void
sbrGetEnvelope (HANDLE_SBR_FRAME_DATA h_frame_data,
                HANDLE_BIT_BUFFER hBitBuf, int channel, int bs_interTes)
{
  int i, j;
  int no_band[MAX_ENVELOPES];
  int delta = 0;
  int offset = 0;
  COUPLING_MODE coupling = h_frame_data->coupling;
  int ampRes;
  int envDataTableCompFactor;
  int start_bits, start_bits_balance;
  Huffman hcb_t, hcb_f;
  int flag;

  h_frame_data->nScaleFactors = 0;

  if ( ( h_frame_data->frameClass == FIXFIX ) && ( h_frame_data->frameInfo[0] == 1 ) ) {
#ifdef AAC_ELD
    if ( h_frame_data->ldsbr )
        ampRes = h_frame_data->ampRes;
    else
#endif
        ampRes = h_frame_data->ampRes = SBR_AMP_RES_1_5;
  }
  else {
    ampRes = h_frame_data->ampRes = h_frame_data->sbr_header.ampResolution;
  }


  /*
    Set number of bits for first value depending on amplitude resolution
  */
  if( ampRes == SBR_AMP_RES_3_0)
  {
    start_bits = SI_SBR_START_ENV_BITS_AMP_RES_3_0;
    start_bits_balance = SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_3_0;
  }
  else
  {
    start_bits = SI_SBR_START_ENV_BITS_AMP_RES_1_5;
    start_bits_balance = SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_1_5;
  }

  /*
    Calculate number of values for each envelope and alltogether
  */
  for (i = 0; i < h_frame_data->frameInfo[0]; i++) {
    no_band[i] =
      h_frame_data->nSfb[h_frame_data->frameInfo[h_frame_data->frameInfo[0] + 2 + i]];
    h_frame_data->nScaleFactors += no_band[i];

  }


  /*
    Select huffman codebook depending on coupling mode and amplitude resolution
  */
  if (coupling == COUPLING_BAL) {
    envDataTableCompFactor = 1;
    if ( ampRes == SBR_AMP_RES_1_5) {
      hcb_t = (Huffman)&bookSbrEnvBalance10T;
      hcb_f = (Huffman)&bookSbrEnvBalance10F;
    }
    else {
      hcb_t = (Huffman)&bookSbrEnvBalance11T;
      hcb_f = (Huffman)&bookSbrEnvBalance11F;
    }
  }
  else {
    envDataTableCompFactor = 0;
    if ( ampRes == SBR_AMP_RES_1_5) {
      hcb_t = (Huffman)&bookSbrEnvLevel10T;
      hcb_f = (Huffman)&bookSbrEnvLevel10F;
    }
    else {
      hcb_t = (Huffman)&bookSbrEnvLevel11T;
      hcb_f = (Huffman)&bookSbrEnvLevel11F;
    }
  }

  /*
    Now read raw envelope data
  */
  for (j = 0, offset = 0; j < h_frame_data->frameInfo[0]; j++) {
    if (h_frame_data->domain_vec1[j] == FREQ) {
      if (coupling == COUPLING_BAL) {
        h_frame_data->iEnvelope[offset] =
          (float) ( BufGetBits(hBitBuf, start_bits_balance) << envDataTableCompFactor );
      }
      else {
        h_frame_data->iEnvelope[offset] =
          (float) BufGetBits (hBitBuf, start_bits);
      }
    }

    for (i = (1 - h_frame_data->domain_vec1[j]); i < no_band[j]; i++) {

      if (h_frame_data->domain_vec1[j] == FREQ) {
        delta = decode_huff_cw(hcb_f, hBitBuf);
      }
      else {
        delta = decode_huff_cw(hcb_t, hBitBuf);
      }

      h_frame_data->iEnvelope[offset + i] = (float) (delta << envDataTableCompFactor);

    }
    offset += no_band[j];


    h_frame_data->interTesGamma[j] = 0; 
    if(bs_interTes){
      flag = (int) BufGetBits (hBitBuf, 1);
      if(flag) {
        h_frame_data->interTesGamma[j] = (int) BufGetBits (hBitBuf, 2);
      }
    }
    
  }

}



#ifdef AAC_ELD
/*******************************************************************************
 Functionname:   extractFrameInfo_LD
 *******************************************************************************

 Description: Extracts a low delay frame_info vector from control data read
              from the bitstream.

 Arguments:   hBitBuf      - bitbuffer handle
              v_frame_info - pointer to memorylocation where the frame-info will
                             be stored.

 Return:     none.

*******************************************************************************/
void
extractFrameInfo_LD (HANDLE_BIT_BUFFER hBitBuf,
                     HANDLE_SBR_FRAME_DATA h_frame_data)
{
  int absBordLead = 0, nRelLead = 0, nRelTrail = 0, bs_num_env = 0,
    frameClass, temp, env, k, absBordTrail = 0, middleBorder = 0,
    bs_num_noise, lA = 0, bs_transient_position = 0;

  int tE[MAX_ENVELOPES + 1];
  int tQ[2 + 1];
  int f[MAX_ENVELOPES + 1];
  int relBordLead[3];
  int relBordTrail[3];

  int *v_frame_info = h_frame_data->frameInfo;

  int numTimeSlots = h_frame_data->numTimeSlots;

  /*
   * First read from the bitstream.
   ***************************************/

  /* Read frame class */
  h_frame_data->frameClass = frameClass = BufGetBits (hBitBuf, SBRLD_CLA_BITS);

  switch (frameClass) {
    case FIXFIX:
      temp = BufGetBits (hBitBuf, SBR_ENV_BITS);
      bs_num_env = (int) (pow(2,temp));
      if (bs_num_env==1)
        h_frame_data->ampRes = BufGetBits (hBitBuf, SI_SBR_AMP_RES_BITS);
      f[0] = BufGetBits (hBitBuf, SBR_RES_BITS);
      for(env = 1; env < bs_num_env; env++)
        f[env] = f[0];
    break;
    case LD_TRAN:
      bs_transient_position = BufGetBits (hBitBuf, SBR_TRAN_BITS);
      bs_num_env = (numTimeSlots==16) ?
        LD_EnvelopeTable512[bs_transient_position][SBR_ENVT_NUMENV]
        : LD_EnvelopeTable480[bs_transient_position][SBR_ENVT_NUMENV];
      for(env = 0; env < bs_num_env; env++)
        f[env] = BufGetBits (hBitBuf, SBR_RES_BITS);
    break;
  }

  /*
   * Calculate the framing.
   *************************/


  switch (frameClass) {
    case FIXFIX:
      absBordLead  = 0;
      absBordTrail = numTimeSlots;
      nRelLead     = bs_num_env - 1;
      nRelTrail    = 0;

      for(k = 0; k < nRelLead; k++){
        relBordLead[k] = (int)((float)numTimeSlots/bs_num_env + 0.5f);
      }

      tE[0]          = absBordLead;
      tE[bs_num_env] = absBordTrail;
      for(env = 1; env <= nRelLead; env++){
        tE[env] = absBordLead;
        for(k = 0; k <= env - 1; k++)
          tE[env] += relBordLead[k];
      }
      for(env = nRelLead + 1; env < bs_num_env; env++){
        tE[env] = absBordTrail;
        for(k = 0; k <= bs_num_env - env - 1; k++)
          tE[env] -= relBordTrail[k];
      }
    break;

    case LD_TRAN:
      tE[0]          = 0;
      tE[bs_num_env] = numTimeSlots;
      for(k = 1; k < bs_num_env; k++)
        tE[k] = (numTimeSlots==16) ?
          LD_EnvelopeTable512[bs_transient_position][k]
          : LD_EnvelopeTable480[bs_transient_position][k];
    break;
  };


  switch(frameClass){
    case  FIXFIX:
      middleBorder = bs_num_env/2;
    break;
    case LD_TRAN:
      middleBorder = 1;
    break;
  };


  tQ[0] = tE[0];
  if(bs_num_env > 1){
    tQ[1] = tE[middleBorder];
    tQ[2] = tE[bs_num_env];
    bs_num_noise = 2;
  }
  else{
    tQ[1] = tE[bs_num_env];
    bs_num_noise = 1;
  }


 switch(frameClass){
  case  FIXFIX:
    lA = -1;
  break;
  case LD_TRAN:
    lA = (numTimeSlots==16) ?
      LD_EnvelopeTable512[bs_transient_position][SBR_ENVT_TRANIDX]
      : LD_EnvelopeTable480[bs_transient_position][SBR_ENVT_TRANIDX];
  break;
  };



  /*
   * Build the framInfo vector...
   *************************************/

  v_frame_info[0] = bs_num_env;   /* Number of envelopes*/
  memcpy(v_frame_info + 1, tE,(bs_num_env + 1)*sizeof(int));    /* time borders*/
  memcpy(v_frame_info + 1 + bs_num_env + 1, f,bs_num_env*sizeof(int));    /* frequency resolution */
  v_frame_info[1 + bs_num_env + 1 + bs_num_env] = lA;                     /* transient envelope*/
  v_frame_info[1 + bs_num_env + 1 + bs_num_env + 1] = bs_num_noise;       /* Number of noise envelopes */
  memcpy(v_frame_info + 1 + bs_num_env + 1 + bs_num_env + 1 + 1,tQ,(bs_num_noise + 1)*sizeof(int));    /* noise borders */
}
#endif


/*******************************************************************************
 Functionname:   extractFrameInfo
 *******************************************************************************

 Description: Extracts a frame_info vector from control data read from the
              bitstream.

 Arguments:   hBitBuf      - bitbuffer handle
              v_frame_info - pointer to memorylocation where the frame-info will
                             be stored.

 Return:     none.

*******************************************************************************/
void
extractFrameInfo (HANDLE_BIT_BUFFER hBitBuf,
                  HANDLE_SBR_FRAME_DATA h_frame_data)
{

  int absBordLead = 0, nRelLead = 0, nRelTrail = 0, bs_num_env = 0,
    bs_num_rel = 0, bs_var_bord = 0, bs_var_bord_0 = 0, bs_var_bord_1 = 0, bs_pointer = 0, bs_pointer_bits,
    frameClass, temp, env, k, bs_num_rel_0 = 0, bs_num_rel_1 = 0, absBordTrail = 0, middleBorder = 0,
    bs_num_noise, lA = 0;

  int tE[MAX_ENVELOPES + 1];
  int tQ[2 + 1];
  int f[MAX_ENVELOPES + 1];
  int bs_rel_bord[MAX_ENVELOPES - 1];
  int bs_rel_bord_0[MAX_ENVELOPES - 1];
  int bs_rel_bord_1[MAX_ENVELOPES - 1];
  int relBordLead[MAX_ENVELOPES - 1];
  int relBordTrail[MAX_ENVELOPES - 1];

  int *v_frame_info = h_frame_data->frameInfo;

  int numTimeSlots = h_frame_data->numTimeSlots;

  /*
   * First read from the bitstream.
   ***************************************/

  /* Read frame class */
  h_frame_data->frameClass = frameClass = BufGetBits (hBitBuf, SBR_CLA_BITS);


  switch (frameClass) {
  case FIXFIX:
    temp = BufGetBits (hBitBuf, SBR_ENV_BITS);
    bs_num_env = (int) (pow(2.0,temp));
    f[0] = BufGetBits (hBitBuf, SBR_RES_BITS);

    for(env = 1; env < bs_num_env; env++)
      f[env] = f[0];
  break;
  case FIXVAR:
    bs_var_bord = BufGetBits (hBitBuf, SBR_ABS_BITS);
    bs_num_rel  = BufGetBits (hBitBuf, SBR_NUM_BITS);
    bs_num_env  = bs_num_rel + 1;
    for (k = 0; k < bs_num_env - 1; k++) {
      bs_rel_bord[k] = 2*BufGetBits (hBitBuf, SBR_REL_BITS) + 2;
    }
    bs_pointer_bits = (int) ceil (log (bs_num_env + 1.0) / log (2.0));
    bs_pointer = BufGetBits (hBitBuf, bs_pointer_bits);

    for(env = 0; env < bs_num_env; env++)
      f[bs_num_env - 1 - env] = BufGetBits (hBitBuf, SBR_RES_BITS);

  break;
  case VARFIX:
    bs_var_bord = BufGetBits (hBitBuf, SBR_ABS_BITS);
    bs_num_rel  = BufGetBits (hBitBuf, SBR_NUM_BITS);
    bs_num_env  = bs_num_rel + 1;
    for (k = 0; k < bs_num_env - 1; k++) {
      bs_rel_bord[k] = 2*BufGetBits (hBitBuf, SBR_REL_BITS) + 2;
    }
    bs_pointer_bits = (int) ceil (log (bs_num_env + 1.0) / log (2.0));
    bs_pointer = BufGetBits (hBitBuf, bs_pointer_bits);

    for(env = 0; env < bs_num_env; env++)
      f[env] = BufGetBits (hBitBuf, SBR_RES_BITS);
  break;
  case VARVAR:
    bs_var_bord_0 = BufGetBits (hBitBuf, SBR_ABS_BITS);
    bs_var_bord_1 = BufGetBits (hBitBuf, SBR_ABS_BITS);
    bs_num_rel_0  = BufGetBits (hBitBuf, SBR_NUM_BITS);
    bs_num_rel_1  = BufGetBits (hBitBuf, SBR_NUM_BITS);

    bs_num_env = bs_num_rel_0 + bs_num_rel_1 + 1;

    for (k = 0; k < bs_num_rel_0; k++) {
      bs_rel_bord_0[k] = 2*BufGetBits (hBitBuf, SBR_REL_BITS) + 2;
    }
    for (k = 0; k < bs_num_rel_1; k++) {
      bs_rel_bord_1[k] = 2*BufGetBits (hBitBuf, SBR_REL_BITS) + 2;
    }
    bs_pointer_bits = (int) ceil (log (bs_num_env + 1.0) / log (2.0));
    bs_pointer = BufGetBits (hBitBuf, bs_pointer_bits);

    for(env = 0; env < bs_num_env; env++)
      f[env] = BufGetBits (hBitBuf, SBR_RES_BITS);
  break;
  };



  /*
   * Calculate the framing.
   *************************/


  switch (frameClass) {
  case FIXFIX:
    absBordLead  = 0;
    absBordTrail = numTimeSlots;
    nRelLead     = bs_num_env - 1;
    nRelTrail    = 0;
    break;
  case FIXVAR:
    absBordLead  = 0;
    absBordTrail = bs_var_bord + numTimeSlots;
    nRelLead     = 0;
    nRelTrail    = bs_num_rel;
    break;
  case VARFIX:
    absBordLead  = bs_var_bord;
    absBordTrail = numTimeSlots;
    nRelLead     = bs_num_rel;
    nRelTrail    = 0;
    break;
  case VARVAR:
    absBordLead  = bs_var_bord_0;
    absBordTrail = bs_var_bord_1 + numTimeSlots;
    nRelLead     = bs_num_rel_0;
    nRelTrail    = bs_num_rel_1;
    break;
  };



  for(k = 0; k < nRelLead; k++){
    switch(frameClass){
    case FIXFIX:
      relBordLead[k] = (int)((float)numTimeSlots/bs_num_env + 0.5f);
      break;
    case VARFIX:
      relBordLead[k] = bs_rel_bord[k];
      break;
    case VARVAR:
      relBordLead[k] = bs_rel_bord_0[k];
      break;
    };
  }


  for(k = 0; k < nRelTrail; k++){
    switch(frameClass){
    case FIXVAR:
      relBordTrail[k] = bs_rel_bord[k];
      break;
    case VARVAR:
      relBordTrail[k] = bs_rel_bord_1[k];
      break;
    };
  }


  tE[0]          = absBordLead;
  tE[bs_num_env] = absBordTrail;
  for(env = 1; env <= nRelLead; env++){
    tE[env] = absBordLead;
    for(k = 0; k <= env - 1; k++)
      tE[env] += relBordLead[k];
  }
  for(env = nRelLead + 1; env < bs_num_env; env++){
    tE[env] = absBordTrail;
    for(k = 0; k <= bs_num_env - env - 1; k++)
      tE[env] -= relBordTrail[k];
  }


  switch(frameClass){
  case  FIXFIX:
    middleBorder = bs_num_env/2;
    break;
  case VARFIX:
    switch(bs_pointer){
    case 0:
      middleBorder = 1;
      break;
    case 1:
      middleBorder = bs_num_env - 1;
      break;
    default:
      middleBorder = bs_pointer - 1;
      break;
    };
    break;
  case FIXVAR:
  case VARVAR:
    switch(bs_pointer){
    case 0:
      middleBorder = bs_num_env - 1;
      break;
    case 1:
      middleBorder = bs_num_env - 1;
      break;
    default:
      middleBorder = bs_num_env + 1 - bs_pointer;
      break;
    };
    break;
  };


  tQ[0] = tE[0];
  if(bs_num_env > 1){
    tQ[1] = tE[middleBorder];
    tQ[2] = tE[bs_num_env];
    bs_num_noise = 2;
  }
  else{
    tQ[1] = tE[bs_num_env];
    bs_num_noise = 1;
  }


 switch(frameClass){
  case  FIXFIX:
    lA = -1;
    break;
  case VARFIX:
    switch(bs_pointer){
    case 0:
    case 1:
      lA = -1;
      break;
    default:
      lA = bs_pointer - 1;
      break;
    };
    break;
  case FIXVAR:
  case VARVAR:
    switch(bs_pointer){
    case 0:
      lA = - 1;
      break;
    default:
      lA = bs_num_env + 1 - bs_pointer;
      break;
    };
    break;
  };



  /*
   * Build the framInfo vector...
   *************************************/

  v_frame_info[0] = bs_num_env;   /* Number of envelopes*/
  memcpy(v_frame_info + 1, tE,(bs_num_env + 1)*sizeof(int));    /* time borders*/
  memcpy(v_frame_info + 1 + bs_num_env + 1, f,bs_num_env*sizeof(int));    /* frequency resolution */
  v_frame_info[1 + bs_num_env + 1 + bs_num_env] = lA;                     /* transient envelope*/
  v_frame_info[1 + bs_num_env + 1 + bs_num_env + 1] = bs_num_noise;       /* Number of noise envelopes */
  memcpy(v_frame_info + 1 + bs_num_env + 1 + bs_num_env + 1 + 1,tQ,(bs_num_noise + 1)*sizeof(int));    /* noise borders */
}




#ifdef SONY_PVC_DEC
void
extractFrameInfo_for_pvc (HANDLE_BIT_BUFFER hBitBuf,
                          HANDLE_SBR_FRAME_DATA h_frame_data,
                          int bs_noise_pos,
                          int	prev_sbr_mode,
                          int	varlen
                          )
{
  int bs_num_env = 0,bs_num_noise = 0,lA = 0;
  int tE[MAX_ENVELOPES + 1];
  int tQ[2 + 1];
  int pvc_tE[MAX_ENVELOPES + 1];
  int pvc_tQ[2 + 1];

  int f[MAX_ENVELOPES + 1];
  int *v_frame_info = h_frame_data->frameInfo;
  int *pvc_frame_info = h_frame_data->pvc_frameInfo;
  int i;

  /* ndr: if the very first frame is already a PVC frame, nEnv == v_frame_info[0] == 0 */
  if(v_frame_info[0]>0) {
    tE[0] =  v_frame_info[1+v_frame_info[0]]- 16;
  }else {
    tE[0] = 0;
  }

  pvc_tE[0] = 0;
  f[0] = 0;
  if(bs_noise_pos == 0) {
    tE[1] = 16 + varlen;
    pvc_tE[1] = 16;
    bs_num_noise = 1;
    bs_num_env = 1;
  } else {
    tE[1] = bs_noise_pos;
    pvc_tE[1] = bs_noise_pos;
    tE[2] = 16 + varlen;
    pvc_tE[2] = 16;
    f[1] = 0;
    bs_num_noise = 2;
    bs_num_env = 2;
  }
  
  for(i=0;i<3;i++) {
    tQ[i]=tE[i];
    pvc_tQ[i]=pvc_tE[i];
  }
    
  lA = -1;

  /* timeslots from 0..tE[0]-1 are evtl. already done by legacy SBR */
  if(prev_sbr_mode == USE_ORIG_SBR){
    pvc_tE[0]=tE[0];
    pvc_tQ[0]=tE[0];
  }

  pvc_frame_info[0] = bs_num_env;											/* Number of envelopes*/
  memcpy(pvc_frame_info + 1, pvc_tE,(bs_num_env + 1)*sizeof(int));				/* time borders*/
  memcpy(pvc_frame_info + 1 + bs_num_env + 1, f,bs_num_env*sizeof(int));    /* frequency resolution */
  pvc_frame_info[1 + bs_num_env + 1 + bs_num_env] = lA;                     /* transient envelope*/
  pvc_frame_info[1 + bs_num_env + 1 + bs_num_env + 1] = bs_num_noise;       /* Number of noise envelopes */
  memcpy(pvc_frame_info + 1 + bs_num_env + 1 + bs_num_env + 1 + 1,pvc_tQ,(bs_num_noise + 1)*sizeof(int));    /* noise borders */

  v_frame_info[0] = bs_num_env;											/* Number of envelopes*/
  memcpy(v_frame_info + 1, tE,(bs_num_env + 1)*sizeof(int));				/* time borders*/
  memcpy(v_frame_info + 1 + bs_num_env + 1, f,bs_num_env*sizeof(int));    /* frequency resolution */
  v_frame_info[1 + bs_num_env + 1 + bs_num_env] = lA;                     /* transient envelope*/
  v_frame_info[1 + bs_num_env + 1 + bs_num_env + 1] = bs_num_noise;       /* Number of noise envelopes */
  memcpy(v_frame_info + 1 + bs_num_env + 1 + bs_num_env + 1 + 1,tQ,(bs_num_noise + 1)*sizeof(int));    /* noise borders */
}


#endif /* SONY_PVC_DEC */


