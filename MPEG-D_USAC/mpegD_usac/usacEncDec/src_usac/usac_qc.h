/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp, University of Erlangen

Initial author:
Bernhard Grill

and edited by

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
$Id: usac_qc.h,v 1.1.1.1 2009-05-29 14:10:21 mul Exp $
*******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

void usac_init_encode_spectrum( void );

int usac_encode_spectrum(
   double      *p_spectrum[MAX_TIME_CHANNELS],
   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
   WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
   int         nr_of_sfb[MAX_TIME_CHANNELS],
   int         average_block_bits,
   int         available_bitreservoir_bits,
   int         padding_limit,
   HANDLE_BSBITSTREAM fixed_stream,
   HANDLE_BSBITSTREAM var_stream,
   int         nr_of_ch
);

void usac_init_decode_spectrum( long sampling_rate );

int usac_decode_spectrum(
   double      *p_spectrum[MAX_TIME_CHANNELS],
   HANDLE_BSBITSTREAM fixed_stream,
   HANDLE_BSBITSTREAM var_stream,
   int         nr_of_chan,
   int         bits_avail,
   WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS]
);

#ifdef __cplusplus
}
#endif




