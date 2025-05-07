/***********************************************************************
MPEG-4 Audio RM Module
Parametric based codec - SSC (SinuSoidal Coding) bit stream Encoder

This software was originally developed by:
* Arno Peters, Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
* Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
* Werner Oomen, Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>

And edited by:
* 

in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
This software module is an implementation of a part of one or more MPEG-4 Audio
tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
MPEG-4 Audio standards free licence to this software module or modifications
thereof for use in hardware or software products claiming conformance to the
MPEG-4 Audio standards. Those intending to use this software module in hardware
or software products are advised that this use may infringe existing patents.
The original developers of this software of this module and their company,
the subsequent editors and their companies, and ISO/EIC have no liability for
use of this software module or modifications thereof in an implementation.
Copyright is not released for non MPEG-4 Audio conforming products. The
original developer retains full right to use this code for his/her own purpose,
assign or donate the code to a third party and to inhibit third party from
using the code for non MPEG-4 Audio conforming products. This copyright notice
must be included in all copies of derivative works.

Copyright  2001.

Source file: SSC_BitStream.h

Required libraries: <none>

Authors:
AP:	Arno Peters,   Philips CE - ASA-labs Eindhoven <arno.peters@philips.com>
JD:	Jan Dasselaar, Philips CE - ASA-labs Eindhoven <jan.dasselaar@philips.com>
WO:	Werner Oomen,  Philips CE - ASA-labs Eindhoven <werner.oomen@philips.com>
AG: Andy Gerrits, Philips Research Eindhoven, <andy.gerrits@philips.com>

Changes:
13-Aug-2001	AP/JD	Initial version
03 Dec 2001 JD      m7725 change (different delta gain coding huffman size)
25 Jul 2002 TM      RM1.5: m8540 improved noise cosing + 8 sf/frame
07 Oct 2002 TM      RM2.0: m8541 parametric stereo coding
21 Nov 2003 AG      RM4.0: ADPCM added
************************************************************************/


/*===========================================================================
 *
 *  Module          : SSC_BITS_BitStream
 *
 *  File            : SSC_BITS_BitStream.h
 *
 *  Description     : This header file defines number of bits used in the bitstream
 *                    per item
 *
 *  Target Platform : ANSI C
 *
 ============================================================================*/

#ifndef _SSC_BITS_BITSTREAM_H
#define _SSC_BITS_BITSTREAM_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*       CONSTANT DEFINITIONS                                                 */
/*============================================================================*/

/* common and file header */
#define SSC_BITS_CHUNK_ID	        32
#define SSC_BITS_CHUNK_LEN	      32
#define SSC_BITS_FORM_TYPE	      32
#define SSC_BITS_FILE_VERSION      8
#define SSC_BITS_ENCODER_VERSION   8
#define SSC_BITS_FORMATTER_VERSION 8
#define SSC_BITS_RESERVED_VERSION  8
#define SSC_BITS_DECODER_LEVEL     2
#define SSC_BITS_SAMPLING_RATE     4
#define SSC_BITS_UPDATE_RATE       4
#define SSC_BITS_SYNTHESIS_METHOD  2
#define SSC_BITS_ENHANCEMENT       1
#define SSC_BITS_MODE              2

#define SSC_BITS_MODE_EXT_MONO     0
#define SSC_BITS_MODE_EXT_STEREO   2
#define SSC_BITS_PS_RESERVED       2

#define SSC_BITS_RESERVED          5

/* data */

/* sound chunk */
#define SSC_BITS_NROF_FRAMES      32

/* audio frame header */
#define SSC_BITS_REFRESH_SINUSOIDS           1
#define SSC_BITS_REFRESH_NOISE               1

#define SSC_BITS_REFRESH_STEREO              1
#define SSC_BITS_STEREO_UPDATE_RATE          2
#define SSC_BITS_STEREO_BINS                 2
#define SSC_BITS_STEREO_ITD_BINS             2

#define SSC_BITS_S_NROF_CONTINUATIONS_MIN    5
#define SSC_BITS_S_NROF_CONTINUATIONS_MAX    7

#define SSC_BITS_N_NROF_DEN                  5
#define SSC_BITS_N_NROF_LSF                  4
#define SSC_BITS_FREQ_GRANULARITY            2
#define SSC_BITS_AMP_GRANULARITY             2
#define SSC_BITS_FRAME_RESERVED              2
#define SSC_BITS_PHASE_JITTER_PRESENT        1
#define SSC_BITS_PHASE_JITTER_BAND           2
#define SSC_BITS_PHASE_JITTER_PERCENTAGE     2

/* transient */
#define SSC_BITS_T_TRANSIENT_PRESENT         1
#define SSC_BITS_T_LOC_MIN                   7
#define SSC_BITS_T_LOC_MAX                   11
#define SSC_BITS_T_TYPE                      2
#define SSC_BITS_T_B_PAR                     3
#define SSC_BITS_T_CHI_PAR                   3
#define SSC_BITS_T_NROF_SIN                  3
#define SSC_BITS_T_FREQ                      9
#define SSC_BITS_T_AMP                       5
#define SSC_BITS_T_PHI                       5

/* sinusoids */
#define SSC_BITS_S_CONT_MIN                  2
#define SSC_BITS_S_CONT_MAX                  5
#define SSC_BITS_S_FREQ_MIN                  7
#define SSC_BITS_S_FREQ_MAX                  25
#define SSC_BITS_S_AMP_MIN                   3
#define SSC_BITS_S_AMP_MAX                   16
#define SSC_BITS_S_PHI                       5
#define SSC_BITS_S_DELTA_CONT_FREQ_MIN       1
#define SSC_BITS_S_DELTA_CONT_FREQ_MAX       12
#define SSC_BITS_S_DELTA_CONT_AMP_MIN        1
#define SSC_BITS_S_DELTA_CONT_AMP_MAX        15
#define SSC_BITS_S_NROF_BIRTHS_MIN           3
#define SSC_BITS_S_NROF_BIRTHS_MAX           15
#define SSC_BITS_S_DELTA_BIRTH_FREQ_MIN      5
#define SSC_BITS_S_DELTA_BIRTH_FREQ_MAX      23
#define SSC_BITS_S_DELTA_BIRTH_AMP_MIN       2
#define SSC_BITS_S_DELTA_BIRTH_AMP_MAX       21
#define SSC_BITS_S_FREQ_PHASE_ADPCM          2

/* noise */
#define SSC_BITS_N_LAR_DEN                   7
#define SSC_BITS_N_ENV_GAIN                  7
#define SSC_BITS_N_OVERLAP_LSF               1
#define SSC_BITS_N_POLE_LAGUERRE             2
#define SSC_BITS_N_GRID_LAGUERRE             1

#define SSC_BITS_N_DELTA_LAR_DEN_MIN         1
#define SSC_BITS_N_DELTA_LAR_DEN_MAX         12

#define SSC_BITS_N_DELTA_ENV_GAIN_MIN        1
#define SSC_BITS_N_DELTA_ENV_GAIN_MAX        12

#define SSC_BITS_N_DELTA_LSF_MIN             2
#define SSC_BITS_N_DELTA_LSF_MAX             9

/* stereo */
#define SSC_BITS_ST_WINDOW_TYPE              4
#define SSC_BITS_ST_QLEVEL_IID               1
#define SSC_BITS_ST_QLEVEL_ITD               1
#define SSC_BITS_ST_QLEVEL_RHO               1

/* padding */
#define SSC_BITS_PADDING_MIN                 0
#define SSC_BITS_PADDING_MAX                 7

#ifdef __cplusplus
}
#endif

#endif /* _SSC_BITS_BITSTREAM_H */


