/*******************************************************************************
This software module was originally developed by

AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS, and VoiceAge Corp.

Initial author:
Yoshiaki Oikawa     (Sony Corporation)
Mitsuyuki Hatanaka  (Sony Corporation),

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
$Id: usac_decdata.c,v 1.3 2011-02-02 16:04:11 mul Exp $
*******************************************************************************/

#include "allHandles.h"

#include "usac_all.h"                 /* structs */
#include "usac_mainStruct.h"       /* structs */
#include "usac_port.h"


Info		*usac_win_seq_info[NUM_WIN_SEQ];
Info*		usac_winmap[NUM_WIN_SEQ];
short		sfbwidthShort[(1<<LEN_MAX_SFBS)];





/* others are in src_tf/decdata.c */
extern const short sfb_96_1024[];
extern const short sfb_96_128[];   /* 12 scfbands */
extern const short sfb_96_960[];
extern const short sfb_96_120[];   /* 12 scfbands */
extern const short sfb_64_1024[];   /* 41 scfbands 47 */
extern const short sfb_64_128[];   /* 12 scfbands */
extern const short sfb_64_960[];   /* 41 scfbands 47 */
extern const short sfb_64_120[];   /* 12 scfbands */
extern const short sfb_48_1024[];
extern const short sfb_48_960[];
extern const short sfb_48_512[];   /* 36 scfbands */
extern const short sfb_48_480[];   /* 35 scfbands */
extern const short sfb_48_128[];
extern const short sfb_48_120[];
extern const short sfb_32_1024[];
extern const short sfb_32_512[];   /* 37 scfbands */
extern const short sfb_32_480[];   /* 37 scfbands */
extern const short sfb_24_1024[];   /* 47 scfbands */
extern const short sfb_24_960[];   /* 47 scfbands */
extern const short sfb_24_128[];   /* 15 scfbands */
extern const short sfb_24_120[];   /* 15 scfbands */
extern const short sfb_24_512[];   /* 31 scfbands */
extern const short sfb_24_480[];   /* 30 scfbands */
extern const short sfb_16_1024[];   /* 43 scfbands */
extern const short sfb_16_960[];   /* 42 scfbands */
extern const short sfb_16_128[];   /* 15 scfbands */
extern const short sfb_16_120[];   /* 15 scfbands */
extern const short sfb_8_1024[];   /* 40 scfbands */
extern const short sfb_8_128[];   /* 15 scfbands */
extern const short sfb_8_960[];   /* 40 scfbands */
extern const short sfb_8_120[];   /* 15 scfbands */
extern const short sfb_32_960[];
extern const short sfb_32_120[];


USAC_SR_Info usac_samp_rate_info[(1<<LEN_SAMP_IDX)] =
{
  /* sampling_frequency, #long sfb, long sfb, #short sfb, short sfb */
  /* samp_rate, nsfb1024, SFbands1024, nsfb128, SFbands128 */
  {96000, 41, sfb_96_1024, 12, sfb_96_128, 40, sfb_96_960, 12, sfb_96_120},	    /* 96000 */
  {88200, 41, sfb_96_1024, 12, sfb_96_128, 40, sfb_96_960, 12, sfb_96_120},	    /* 88200 */
  {64000, 47, sfb_64_1024, 12, sfb_64_128, 46, sfb_64_960, 12, sfb_64_120},	    /* 64000 */
  {48000, 49, sfb_48_1024, 14, sfb_48_128, 49, sfb_48_960, 14, sfb_48_120},	    /* 48000 */
  {44100, 49, sfb_48_1024, 14, sfb_48_128, 49, sfb_48_960, 14, sfb_48_120},	    /* 44100 */
  {32000, 51, sfb_32_1024, 14, sfb_48_128, 49, sfb_32_960, 14, sfb_48_120},	    /* 32000 */
  {24000, 47, sfb_24_1024, 15, sfb_24_128, 46, sfb_24_960, 15, sfb_24_120},         /* 24000 */
  {22050, 47, sfb_24_1024, 15, sfb_24_128, 46, sfb_24_960, 15, sfb_24_120},	    /* 22050 */
  {16000, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_960, 15, sfb_16_120},	    /* 16000 */
  {12000, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_960, 15, sfb_16_120},	    /* 12000 */
  {11025, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_960, 15, sfb_16_120},	    /* 11025 */
  { 8000, 40,  sfb_8_1024, 15,  sfb_8_128, 40,  sfb_8_960, 15,  sfb_8_120},	    /*  8000 */
  { 7350, 40,  sfb_8_1024, 15,  sfb_8_128, 40,  sfb_8_960, 15,  sfb_8_120},	    /*  8000 */
  {    0,  0,           0,  0,          0,  0,          0,  0,          0}
};






