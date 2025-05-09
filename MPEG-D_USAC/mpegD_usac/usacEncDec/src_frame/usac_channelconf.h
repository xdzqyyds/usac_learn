/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Markus Multrus

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
$Id: usac_channelconf.h,v 1.2 2011-05-04 09:26:28 mul Exp $
*******************************************************************************/
#ifndef __INCLUDED_USAC_CHANNELCONF_H
#define __INCLUDED_USAC_CHANNELCONF_H


typedef enum {

  USAC_OUTPUT_CHANNEL_POS_NA   = -1, /* n/a                                */
  USAC_OUTPUT_CHANNEL_POS_L    =  0, /* Left Front                          */
  USAC_OUTPUT_CHANNEL_POS_R    =  1, /* Right Front                         */
  USAC_OUTPUT_CHANNEL_POS_C    =  2, /* Center Front                        */
  USAC_OUTPUT_CHANNEL_POS_LFE  =  3, /* Low Frequency Enhancement           */
  USAC_OUTPUT_CHANNEL_POS_LS   =  4, /* Left Surround                       */
  USAC_OUTPUT_CHANNEL_POS_RS   =  5, /* Right Surround                      */
  USAC_OUTPUT_CHANNEL_POS_LC   =  6, /* Left Front Center                   */
  USAC_OUTPUT_CHANNEL_POS_RC   =  7, /* Right Front Center                  */
  USAC_OUTPUT_CHANNEL_POS_LSR  =  8, /* Rear Surround Left                  */
  USAC_OUTPUT_CHANNEL_POS_RSR  =  9, /* Rear Surround Right                 */
  USAC_OUTPUT_CHANNEL_POS_CS   = 10, /* Rear Center                         */
  USAC_OUTPUT_CHANNEL_POS_LSD  = 11, /* Left Surround Direct                */
  USAC_OUTPUT_CHANNEL_POS_RSD  = 12, /* Right Surround Direct               */
  USAC_OUTPUT_CHANNEL_POS_LSS  = 13, /* Left Side Surround                  */
  USAC_OUTPUT_CHANNEL_POS_RSS  = 14, /* Right Side Surround                 */
  USAC_OUTPUT_CHANNEL_POS_LW   = 15, /* Left Wide Front                     */
  USAC_OUTPUT_CHANNEL_POS_RW   = 16, /* Right Wide Front                    */
  USAC_OUTPUT_CHANNEL_POS_LV   = 17, /* Left Front Vertical Height          */
  USAC_OUTPUT_CHANNEL_POS_RV   = 18, /* Right Front Vertical Height         */
  USAC_OUTPUT_CHANNEL_POS_CV   = 19, /* Center Front Vertical Height        */
  USAC_OUTPUT_CHANNEL_POS_LVR  = 20, /* Left Surround Vertical Height Rear  */
  USAC_OUTPUT_CHANNEL_POS_RVR  = 21, /* Right Surround Vertical Height Rear */
  USAC_OUTPUT_CHANNEL_POS_CVR  = 22, /* Center Vertical Height Rear         */
  USAC_OUTPUT_CHANNEL_POS_LVSS = 23, /* Left Vertical Height Side Surround  */
  USAC_OUTPUT_CHANNEL_POS_RVSS = 24, /* Right Vertical Height Side Surround */
  USAC_OUTPUT_CHANNEL_POS_TS   = 25, /* Top Center Surround                 */
  USAC_OUTPUT_CHANNEL_POS_LFE2 = 26, /* Low Frequency Enhancement 2         */
  USAC_OUTPUT_CHANNEL_POS_LB   = 27, /* Left Front Vertical Bottom          */
  USAC_OUTPUT_CHANNEL_POS_RB   = 28, /* Right Front Vertical Bottom         */
  USAC_OUTPUT_CHANNEL_POS_CB   = 29, /* Center Front Vertical Bottom        */
  USAC_OUTPUT_CHANNEL_POS_LVS  = 30, /* Left Vertical Height Surround       */
  USAC_OUTPUT_CHANNEL_POS_RVS  = 31  /* Right Vertical Height Surround      */

} USAC_OUTPUT_CHANNEL_POS; 

/* note: USAC_MAX_NUM_OUT_CHANNELS does not correspond to the max. allowed value by the syntax, 
   which is ((1 << 5) - 1) + ((1 << 8) - 1) + ((1 << 16) - 1) */
#define USAC_MAX_NUM_OUT_CHANNELS (255)

typedef struct {

  unsigned int            numOutChannels;
  USAC_OUTPUT_CHANNEL_POS outputChannelPos[USAC_MAX_NUM_OUT_CHANNELS];

} USAC_CHANNEL_CONFIG;


#endif /* __INCLUDED_USAC_CHANNELCONF_H */
